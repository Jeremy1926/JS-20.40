#pragma once
#include "pch.h"
#include "framework.h"
#include "Offsets.h"
#include "Inventory.h"
#include "Abilities.h"

static void* (*ApplyCharacterCustomization)(AFortPlayerStateAthena* a1, APawn* a2) = decltype(ApplyCharacterCustomization)(Jeremy::ImageBase + 0x6EEC570);

namespace GameMode
{
	uint8 NextIdx = 3;
	int CurrentPlayersOnTeam = 0;
	int MaxPlayersOnTeam = 1;

	inline bool (*ReadyToStartMatchOG)(AFortGameModeAthena* GameMode);
	inline bool ReadyToStartMatch(AFortGameModeAthena* GameMode)
	{
		UFortPlaylistAthena* Playlist;

		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		static bool SetupPlaylist = false;
		if (!SetupPlaylist)
		{
			SetupPlaylist = true;

			Playlist = Utils::FindObject<UFortPlaylistAthena>(L"/Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo"); // /Game/Athena/Playlists/Playlist_DefaultSolo.Playlist_DefaultSolo

			if (Options::bForceRespawns)
			{
				Playlist->bRespawnInAir = true;
				Playlist->RespawnHeight.Curve.CurveTable = nullptr;
				Playlist->RespawnHeight.Curve.RowName = FName();
				Playlist->RespawnHeight.Value = 20000;
				Playlist->RespawnTime.Curve.CurveTable = nullptr;
				Playlist->RespawnTime.Curve.RowName = FName();
				Playlist->RespawnTime.Value = 3;
				Playlist->RespawnType = EAthenaRespawnType::InfiniteRespawn;
				Playlist->bAllowJoinInProgress = true;
				Playlist->bForceRespawnLocationInsideOfVolume = true;
			}

			GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
			GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
			GameState->CurrentPlaylistInfo.MarkArrayDirty();
			GameState->OnRep_CurrentPlaylistInfo();

			GameMode->CurrentPlaylistName = Playlist->PlaylistName;
			GameMode->WarmupRequiredPlayerCount = Options::iMinPlayersForEarlyStart;

			GameMode->bDBNOEnabled = Playlist->MaxTeamSize > 1;
			GameMode->bAlwaysDBNO = Playlist->MaxTeamSize > 1;
			GameState->bDBNODeathEnabled = Playlist->MaxTeamSize > 1;
			GameState->SetIsDBNODeathEnabled(Playlist->MaxTeamSize > 1);

			NextIdx = Playlist->DefaultFirstTeam;
			MaxPlayersOnTeam = Playlist->MaxSquadSize;

			GameMode->GameSession->MaxPlayers = Playlist->MaxPlayers;
			GameMode->GameSession->MaxSpectators = 0;
			GameMode->GameSession->MaxPartySize = Playlist->MaxSquadSize;
			GameMode->GameSession->MaxSplitscreensPerConnection = 2;
			GameMode->GameSession->bRequiresPushToTalk = false;
			GameMode->GameSession->SessionName = UKismetStringLibrary::Conv_StringToName(FString(L"GameSession"));

			GameState->CurrentPlaylistId = Playlist->PlaylistId;
			GameState->OnRep_CurrentPlaylistId();

			GameState->bGameModeWillSkipAircraft = Playlist->bSkipAircraft;
			GameState->CachedSafeZoneStartUp = Playlist->SafeZoneStartUp;
			GameState->AirCraftBehavior = Playlist->AirCraftBehavior;
			GameState->OnRep_Aircraft();

			GameState->DefaultParachuteDeployTraceForGroundDistance = 10000;
		}

		if (!GameState->MapInfo)
		{
			return false;
		}

		static bool Listening = false;
		if (!Listening) {
			Listening = true;

			auto Beacon = Utils::SpawnActor<AFortOnlineBeaconHost>({});
			Beacon->ListenPort = 7777;
			Jeremy::Funcs::InitHost(Beacon);
			Jeremy::Funcs::PauseBeaconRequests(Beacon, false);

			UWorld::GetWorld()->NetDriver = Beacon->NetDriver;
			UWorld::GetWorld()->NetDriver->World = UWorld::GetWorld();
			UWorld::GetWorld()->NetDriver->NetDriverName = UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"GameNetDriver");

			FString Error;
			FURL url = FURL();
			url.Port = 7777;

			if (!Jeremy::Funcs::InitListen(UWorld::GetWorld()->NetDriver, UWorld::GetWorld(), url, true, Error))
			{
				
			}
			else {
				
			}

			Jeremy::Funcs::SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

			for (size_t i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++) {
				UWorld::GetWorld()->LevelCollections[i].NetDriver = UWorld::GetWorld()->NetDriver;
			}

			Jeremy::Funcs::SetWorld(UWorld::GetWorld()->NetDriver, UWorld::GetWorld());

			for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels.Num(); i++)
			{
				FVector Loc{};
				FRotator Rot{};
				bool bSuccess = false;
				((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i], Loc, Rot, &bSuccess, FString(), nullptr);
				FAdditionalLevelStreamed NewLevel{};
				NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevels[i].ObjectID.AssetPathName;
				NewLevel.bIsServerOnly = false;
				GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
			}

			for (int i = 0; i < GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly.Num(); i++)
			{
				FVector Loc{};
				FRotator Rot{};
				bool bSuccess = false;
				((ULevelStreamingDynamic*)ULevelStreamingDynamic::StaticClass()->DefaultObject)->LoadLevelInstanceBySoftObjectPtr(UWorld::GetWorld(), GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i], Loc, Rot, &bSuccess, FString(), nullptr);
				FAdditionalLevelStreamed NewLevel{};
				NewLevel.LevelName = GameState->CurrentPlaylistInfo.BasePlaylist->AdditionalLevelsServerOnly[i].ObjectID.AssetPathName;
				NewLevel.bIsServerOnly = true;
				GameState->AdditionalPlaylistLevelsStreamed.Add(NewLevel);
			}

			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"));

			auto TacticalSprintAbility = Utils::FindObject<UFortAbilitySet>(L"/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint");

			if (!TacticalSprintAbility)
				TacticalSprintAbility = Utils::FindObject<UFortAbilitySet>(L"/TacticalSprint/Gameplay/AS_TacticalSprint.AS_TacticalSprint");

			AbilitySets.Add(TacticalSprintAbility);
			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/Ascender/Gameplay/Ascender/AS_Ascender.AS_Ascender"));
			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/DoorBashContent/Gameplay/AS_DoorBash.AS_DoorBash"));
			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/HillScramble/Gameplay/AS_HillScramble.AS_HillScramble"));
			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/SlideImpulse/Gameplay/AS_SlideImpulse.AS_SlideImpulse"));
			AbilitySets.Add(Utils::FindObject<UFortAbilitySet>(L"/RealitySeedGameplay/Environment/Foliage/GAS_Athena_RealitySapling.GAS_Athena_RealitySapling"));

			for (int i = 0; i < UObject::GObjects->Num(); i++)
			{
				auto Object = UObject::GObjects->GetByIndex(i);

				if (!Object || !Object->Class || Object->IsDefaultObject())
					continue;

				if (auto GameFeatureData = Object->Cast<UFortGameFeatureData>())
				{
					auto LootTableData = GameFeatureData->DefaultLootTableData;
					auto AbilitySet = Utils::FindObject<UFortAbilitySet>(GameFeatureData->PlayerAbilitySet.ObjectID.AssetPathName.GetRawWString().c_str());

					if (AbilitySet)
					{
						AbilitySet->AddToRoot();
						AbilitySets.Add(AbilitySet);
					}

				}
			}

			GameState->OnRep_AdditionalPlaylistLevelsStreamed();
			GameState->OnFinishedStreamingAdditionalPlaylistLevel();

			GameMode->bWorldIsReady = true;

			UCurveTable* AthenaGameDataTable = GameState->AthenaGameDataTable;

			if (AthenaGameDataTable)
			{
				static FName DefaultSafeZoneDamageName = FName(UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.Damage")));

				for (const auto& [RowName, RowPtr] : ((UDataTable*)AthenaGameDataTable)->RowMap)
				{
					if (RowName != DefaultSafeZoneDamageName)
						continue;

					FSimpleCurve* Row = (FSimpleCurve*)RowPtr;

					if (!Row)
						continue;

					for (auto& Key : Row->Keys)
					{
						FSimpleCurveKey* KeyPtr = &Key;

						if (KeyPtr->Time == 0.f)
						{
							KeyPtr->Value = 0.f;
						}
					}

					Row->Keys.Add(FSimpleCurveKey(1.f, 0.01f));
				}
			}
			;
			SetConsoleTitleA("JS 20.40 | Listening: 7777");
		}

		if (GameState->PlayersLeft > 0)
		{
			return true;
		}
		else
		{
			auto TS = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			auto DR = Options::iWarmupTime;

			GameState->WarmupCountdownEndTime = TS + DR;
			GameMode->WarmupCountdownDuration = DR;
			GameState->WarmupCountdownStartTime = TS;
			GameMode->WarmupEarlyCountdownDuration = DR;
		}

		return false;
	}

	APawn* (*SpawnDefaultPawnForOG)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer, AActor* StartSpot);
	APawn* SpawnDefaultPawnFor(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer, AActor* StartSpot) {
		
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;

		NewPlayer->XPComponent->bRegisteredWithQuestManager = true;
		NewPlayer->XPComponent->OnRep_bRegisteredWithQuestManager();

		PlayerState->SeasonLevelUIDisplay = NewPlayer->XPComponent->CurrentLevel;
		PlayerState->OnRep_SeasonLevelUIDisplay();

		NewPlayer->GetQuestManager(ESubGame::Athena)->InitializeQuestAbilities(NewPlayer->Pawn);

		UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
		PlayerState->OnRep_CharacterData();

		UAthenaPickaxeItemDefinition* PickDef;
		FFortAthenaLoadout& CosmecticLoadoutPC = NewPlayer->CosmeticLoadoutPC;
		PickDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : Utils::FindObject<UAthenaPickaxeItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
		if (PickDef) {
			Inventory::GiveItem(NewPlayer, PickDef->WeaponDefinition, 1, 0);
		}
		else {
		}

		for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
		{
			if (GameMode->StartingItems[i].Count > 0)
			{
				Inventory::GiveItem(NewPlayer, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
			}
		}

		Abilities::GiveAbilitySet(Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"), PlayerState);

		ApplyCharacterCustomization(PlayerState, Pawn);

		return Pawn;
	}

	inline __int64 PickTeam(__int64 a1, unsigned __int8 a2, __int64 a3)
	{
		uint8 Ret = NextIdx;
		CurrentPlayersOnTeam++;

		if (CurrentPlayersOnTeam == MaxPlayersOnTeam)
		{
			NextIdx++;
			CurrentPlayersOnTeam = 0;
		}
		return Ret;
	};

	static bool (*StartAircraftPhaseOG)(AFortGameModeAthena* GameMode, char a2);
	bool StartAircraftPhase(AFortGameModeAthena* GameMode, char a2)
	{
		StartAircraftPhaseOG(GameMode, a2);
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		if (Options::bLateGame)
		{
			GameState->GamePhase = EAthenaGamePhase::SafeZones;
			GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
			GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);

			auto Aircraft = GameState->Aircrafts[0];
			Aircraft->FlightInfo.FlightSpeed = 0.f;
			FVector Loc = GameMode->SafeZoneLocations[3];
			Loc.Z = 17500.f;

			Aircraft->FlightInfo.FlightStartLocation = (FVector_NetQuantize100)Loc;

			Aircraft->FlightInfo.TimeTillFlightEnd = 7.f;
			Aircraft->FlightInfo.TimeTillDropEnd = 0.f;
			Aircraft->FlightInfo.TimeTillDropStart = 0.f;
			Aircraft->FlightStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
			Aircraft->FlightEndTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
			GameState->SafeZonesStartTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld()) + 7.f;
		}
	}

	void OnAircraftEnteredDropZone(UObject* Context, FFrame& Stack) {
		AFortAthenaAircraft* Aircraft;
		Stack.StepCompiledIn(&Aircraft);
		Stack.IncrementCode();

		auto GameMode = (AFortGameModeAthena*)Context;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		callOG(GameMode, Stack.CurrentNativeFunction, OnAircraftEnteredDropZone, Aircraft);
	}

	void OnAircraftExitedDropZone(UObject* Context, FFrame& Stack)
	{
		AFortAthenaAircraft* Aircraft;
		Stack.StepCompiledIn(&Aircraft);
		Stack.IncrementCode();

		auto GameMode = (AFortGameModeAthena*)Context;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		for (auto& Player : GameMode->AlivePlayers)
		{
			if (Player->IsInAircraft())
			{
				Player->GetAircraftComponent()->ServerAttemptAircraftJump({});
			}
		}

		callOG(GameMode, Stack.CurrentNativeFunction, OnAircraftExitedDropZone, Aircraft);
	}

	void (*StartNewSafeZonePhaseOG)(AFortGameModeAthena*, int);
	void StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int a2)
	{
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;
		float TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(GameState);

		FFortSafeZoneDefinition* SafeZoneDefinition = &GameState->MapInfo->SafeZoneDefinition;
		TArray<float>& Durations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x248);
		TArray<float>& HoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + 0x238);

		auto NewSafeZonePhase = a2 >= 0 ? a2 : ((GameMode->SafeZonePhase + 1));

		constexpr static std::array<float, 8> LateGameDurations
		{
			0.f,
			65.f,
			60.f,
			50.f,
			45.f,
			35.f,
			30.f,
			40.f,
		};

		constexpr static std::array<float, 8> LateGameHoldDurations
		{
			0.f,
			60.f,
			55.f,
			50.f,
			45.f,
			30.f,
			0.f,
			0.f,
		};

		static bool bSetDurations = false;

        if (!bSetDurations)
        {
            bSetDurations = true;

            auto GameData = GameState->AthenaGameDataTable ? GameState->AthenaGameDataTable : GameState->AthenaGameDataTable;

            auto ShrinkTime = UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.ShrinkTime"));
            auto HoldTime = UKismetStringLibrary::Conv_StringToName(FString(L"Default.SafeZone.WaitTime"));

            for (int i = 0; i < Durations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, ShrinkTime, (float)i, nullptr, &Durations[i], FString());
            }
            for (int i = 0; i < HoldDurations.Num(); i++)
            {
                UDataTableFunctionLibrary::EvaluateCurveTableRow(GameData, HoldTime, (float)i, nullptr, &HoldDurations[i], FString());
            }
        }

        if (!Options::bLateGame)
        {
            auto Duration = Durations[NewSafeZonePhase];
            auto HoldDuration = HoldDurations[NewSafeZonePhase];

            GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
            GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
        }

		if (Options::bLateGame && GameMode->SafeZonePhase > 3)
		{
			auto Duration = LateGameDurations[GameMode->SafeZonePhase - 2];
			auto HoldDuration = LateGameHoldDurations[GameMode->SafeZonePhase - 2];

			GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + HoldDuration;
			GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
		}

		if (Options::bLateGame && GameMode->SafeZonePhase < 3)
		{
			GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds;
			GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + 0.15f;
		}
		else if (Options::bLateGame && GameMode->SafeZonePhase == 3)
		{

			if (Options::bInfiniteLateGame)
			{
				GameMode->SafeZonePhase = 3;
				GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = 77777;
				GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = 77777;
			}
			else
			{
				auto Duration = LateGameDurations[GameMode->SafeZonePhase - 2];

				GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = TimeSeconds + 30.f;
				GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + Duration;
			}
		}

		return StartNewSafeZonePhaseOG(GameMode, a2);
	}

	void Hook()
	{
		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x660b124), ReadyToStartMatch, (LPVOID*)&ReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x6610804), SpawnDefaultPawnFor, (LPVOID*)&SpawnDefaultPawnForOG);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x66151A8), StartNewSafeZonePhase, (LPVOID*)&StartNewSafeZonePhaseOG);

	    MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x6609F0C), PickTeam, nullptr);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x6612f34), StartAircraftPhase, (LPVOID*)&StartAircraftPhaseOG);

		//Utils::ExecHook(L"/Script/FortniteGame.FortGameModeAthena.OnAircraftExitedDropZone", OnAircraftExitedDropZone, OnAircraftExitedDropZoneOG);
		//Utils::ExecHook(L"/Script/FortniteGame.FortGameModeAthena.OnAircraftEnteredDropZone", OnAircraftEnteredDropZone, OnAircraftEnteredDropZoneOG);
	}
}