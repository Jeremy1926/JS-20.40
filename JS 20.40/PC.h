#pragma once
#include "pch.h"
#include "framework.h"
#include "Lategame.h"

static __int64 (*CantBuild)(UWorld*, UObject*, FVector, FRotator, char, void*, char*) = decltype(CantBuild)(Jeremy::ImageBase + 0x6ac00c4);

namespace PC
{
	void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
	inline void ServerAcknowledgePossession(AFortPlayerControllerAthena* PC, APawn* Pawn)
	{
		PC->AcknowledgedPawn = Pawn;

		return ServerAcknowledgePossessionOG(PC, Pawn);
	}

	inline void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);
	inline void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto Pawn = (AFortPlayerPawn*)PC->Pawn;

		return ServerLoadingScreenDroppedOG(PC);
	}

	void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
	void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
	{
		if (!PC)
		{
			return;
		}

		return ServerReadyToStartMatchOG(PC);
	}

	void (*GetPlayerViewPointOG)(AFortPlayerControllerAthena*, FVector&, FRotator&);
	void GetPlayerViewPoint(AFortPlayerControllerAthena* PlayerController, FVector& Loc, FRotator& Rot)
	{
		static auto SFName = UKismetStringLibrary::Conv_StringToName(FString(L"Spectating"));
		if (PlayerController->StateName == SFName)
		{
			Loc = PlayerController->LastSpectatorSyncLocation;
			Rot = PlayerController->LastSpectatorSyncRotation;
		}
		else
		{
			auto ViewTarget = PlayerController->GetViewTarget();

			if (ViewTarget)
			{
				Loc = ViewTarget->K2_GetActorLocation();
				if (auto TargetPawn = ViewTarget->Cast<AFortPlayerPawnAthena>())
					Loc.Z += TargetPawn->BaseEyeHeight;
				Rot = PlayerController->GetControlRotation();
			}
			else
				PlayerController->GetActorEyesViewPoint(&Loc, &Rot);
		}
	}

	inline void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* Comp, FRotator Rotation)
	{
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		auto PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
		UWorld::GetWorld()->AuthorityGameMode->RestartPlayer(PC);

		if (PC->MyFortPawn)
		{
			PC->ClientSetRotation(Rotation, true);
			PC->MyFortPawn->BeginSkydiving(true);
			PC->MyFortPawn->SetHealth(100);
			PC->MyFortPawn->SetShield(0);
		}

		if (PC && PC->WorldInventory)
		{
			for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
			{
				if (((UFortWorldItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->bCanBeDropped)
				{
					int Count = PC->WorldInventory->Inventory.ReplicatedEntries[i].Count;
					Inventory::RemoveItem(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Count);
				}
			}
		}

		if (Options::bLateGame && PC->MyFortPawn)
		{
			FVector AircraftLocation = GameState->Aircrafts[0]->K2_GetActorLocation();

			float Angle = UKismetMathLibrary::RandomFloatInRange(0.0f, 6.2831853f);
			float Radius = (float)(rand() % 1000);

			float OffsetX = cosf(Angle) * Radius;
			float OffsetY = sinf(Angle) * Radius;

			FVector Offset;
			Offset.X = OffsetX;
			Offset.Y = OffsetY;
			Offset.Z = 0.0f;

			FVector NewLoc = AircraftLocation + Offset;

			PC->MyFortPawn->K2_SetActorLocation(NewLoc, false, nullptr, false);
		}

		if (Options::bLateGame)
		{
			PC->MyFortPawn->SetShield(100);
		}

		if (Options::bInfiniteLateGame)
		{
			auto Shotgun = Lategame::GetShotguns();
			auto AssaultRifle = Lategame::GetAssaultRifles();
			auto Sniper = Lategame::GetSnipers();
			auto Heal = Lategame::GetHeals();
			auto HealSlot2 = Lategame::GetHeals();

			int ShotgunClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Shotgun.Item)->ClipSize;
			int AssaultRifleClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)AssaultRifle.Item)->ClipSize;
			int SniperClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Sniper.Item)->ClipSize;
			int HealClipSize = Heal.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)Heal.Item)->ClipSize : 0;
			int HealSlot2ClipSize = HealSlot2.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)HealSlot2.Item)->ClipSize : 0;

			Inventory::GiveItem(PC, Lategame::GetResource(EFortResourceType::Wood), 500, 0);
			Inventory::GiveItem(PC, Lategame::GetResource(EFortResourceType::Stone), 500, 0);
			Inventory::GiveItem(PC, Lategame::GetResource(EFortResourceType::Metal), 500, 0);

			Inventory::GiveItem(PC, Lategame::GetAmmo(EAmmoType::Assault), 250, 0);
			Inventory::GiveItem(PC, Lategame::GetAmmo(EAmmoType::Shotgun), 50, 0);
			Inventory::GiveItem(PC, Lategame::GetAmmo(EAmmoType::Submachine), 400, 0);
			Inventory::GiveItem(PC, Lategame::GetAmmo(EAmmoType::Rocket), 6, 0);
			Inventory::GiveItem(PC, Lategame::GetAmmo(EAmmoType::Sniper), 20, 0);

			Inventory::GiveItem(PC, AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize);
			Inventory::GiveItem(PC, Shotgun.Item, Shotgun.Count, ShotgunClipSize);
			Inventory::GiveItem(PC, Sniper.Item, Sniper.Count, SniperClipSize);
			Inventory::GiveItem(PC, Heal.Item, Heal.Count, HealClipSize);
			Inventory::GiveItem(PC, HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize);
		}
	}

	DefHookOg(void, ClientOnPawnDied, AFortPlayerControllerAthena*, FFortPlayerDeathReport&);

	void ClientOnPawnDied(AFortPlayerControllerAthena* PlayerController, FFortPlayerDeathReport& DeathReport)
	{
		if (!PlayerController)
			return ClientOnPawnDiedOG(PlayerController, DeathReport);
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;
		auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;


		if (!GameState->IsRespawningAllowed(PlayerState) && PlayerController->WorldInventory && PlayerController->MyFortPawn)
		{
			bool bHasMats = false;
			for (auto& Entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
			{
				if (!Entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>() && (Entry.ItemDefinition->IsA<UFortResourceItemDefinition>() || Entry.ItemDefinition->IsA<UFortWeaponRangedItemDefinition>() || Entry.ItemDefinition->IsA<UFortConsumableItemDefinition>() || Entry.ItemDefinition->IsA<UFortAmmoItemDefinition>()))
				{
					Inventory::SpawnPickup(PlayerController->MyFortPawn->K2_GetActorLocation(), Entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::PlayerElimination, PlayerController->MyFortPawn);
				}
			}
		}

		auto KillerPlayerState = (AFortPlayerStateAthena*)DeathReport.KillerPlayerState;
		auto KillerPawn = (AFortPlayerPawnAthena*)DeathReport.KillerPawn;

		PlayerState->PawnDeathLocation = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->K2_GetActorLocation() : FVector();
		PlayerState->DeathInfo.bDBNO = PlayerController->MyFortPawn ? PlayerController->MyFortPawn->IsDBNO() : false;
		PlayerState->DeathInfo.DeathLocation = PlayerState->PawnDeathLocation;
		PlayerState->DeathInfo.DeathTags = DeathReport.Tags;
		PlayerState->DeathInfo.DeathCause = AFortPlayerStateAthena::ToDeathCause(PlayerState->DeathInfo.DeathTags, PlayerState->DeathInfo.bDBNO);
		PlayerState->DeathInfo.Downer = KillerPlayerState;
		PlayerState->DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerState;
		PlayerState->DeathInfo.Distance = PlayerController->MyFortPawn ? (PlayerState->DeathInfo.DeathCause != EDeathCause::FallDamage ? (KillerPawn ? KillerPawn->GetDistanceTo(PlayerController->MyFortPawn) : 0) : PlayerController->MyFortPawn->Cast<AFortPlayerPawnAthena>()->LastFallDistance) : 0;
		PlayerState->DeathInfo.bInitialized = true;
		PlayerState->OnRep_DeathInfo();

		if (KillerPlayerState && KillerPawn && KillerPawn->Controller && KillerPawn->Controller->IsA<AFortPlayerControllerAthena>() && KillerPawn->Controller != PlayerController)
		{
			KillerPlayerState->KillScore++;
			KillerPlayerState->OnRep_Kills();
			KillerPlayerState->TeamKillScore++;
			KillerPlayerState->OnRep_TeamKillScore();

			KillerPlayerState->ClientReportKill(PlayerState);
			KillerPlayerState->ClientReportTeamKill(KillerPlayerState->TeamKillScore);

			auto KillerPC = (AFortPlayerControllerAthena*)KillerPlayerState->Owner;
			auto PlayerCount = GameMode->AlivePlayers.Num() - 1;

			if (PlayerCount == 5 || PlayerCount == 10 || PlayerCount == 25)
			{
				int Points = 10;
				if (PlayerCount == 10)
					Points = 15;

				for (int i = 0; i < GameMode->AlivePlayers.Num(); i++)
				{
					GameMode->AlivePlayers[i]->ClientReportTournamentPlacementPointsScored(PlayerCount, Points);
					auto PlayerState = (AFortPlayerStateAthena*)GameMode->AlivePlayers[i]->PlayerState;
					auto PlayerName = PlayerState->GetPlayerName().ToString();
				}
			}
		}

		if (!GameState->IsRespawningAllowed(PlayerState) && (PlayerController->MyFortPawn ? !PlayerController->MyFortPawn->IsDBNO() : true))
		{
			PlayerState->Place = GameState->PlayersLeft;
			PlayerState->OnRep_Place();
			FAthenaMatchStats& Stats = PlayerController->MatchReport->MatchStats;
			FAthenaMatchTeamStats& TeamStats = PlayerController->MatchReport->TeamStats;

			Stats.Stats[3] = PlayerState->KillScore;
			Stats.Stats[8] = PlayerState->SquadId;
			PlayerController->ClientSendMatchStatsForPlayer(Stats);

			TeamStats.Place = PlayerState->Place;
			TeamStats.TotalPlayers = GameState->TotalPlayers;
			PlayerController->ClientSendTeamStatsForPlayer(TeamStats);


			AFortWeapon* DamageCauser = nullptr;
			if (auto Projectile = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortProjectileBase>() : nullptr)
				DamageCauser = Projectile->GetOwnerWeapon();
			else if (auto Weapon = DeathReport.DamageCauser ? DeathReport.DamageCauser->Cast<AFortWeapon>() : nullptr)
				DamageCauser = Weapon;

			((void (*)(AFortGameModeAthena*, AFortPlayerController*, APlayerState*, AFortPawn*, UFortWeaponItemDefinition*, EDeathCause, char))(Jeremy::ImageBase + 0x606F83C))(GameMode, PlayerController, KillerPlayerState == PlayerState ? nullptr : KillerPlayerState, KillerPawn, DamageCauser ? DamageCauser->WeaponData : nullptr, PlayerState->DeathInfo.DeathCause, 0);

			PlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, PlayerController->MatchReport->EndOfMatchResults);

			PlayerController->StateName = UKismetStringLibrary::Conv_StringToName(L"Spectating");

			if (PlayerController->MyFortPawn && KillerPlayerState && KillerPawn && KillerPawn->Controller != PlayerController)
			{
				auto Handle = KillerPlayerState->AbilitySystemComponent->MakeEffectContext();
				FGameplayTag Tag;
				static auto Cue = UKismetStringLibrary::Conv_StringToName(L"GameplayCue.Shield.PotionConsumed");
				Tag.TagName = Cue;
				KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(Tag, FPredictionKey(), Handle);
				KillerPlayerState->AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(Tag, FPredictionKey(), Handle);

				auto Health = KillerPawn->GetHealth();
				auto Shield = KillerPawn->GetShield();

				if (Health == 100)
				{
					Shield += Shield + 50;
				}
				else if (Health + 50 > 100)
				{
					Health = 100;
					Shield += (Health + 50) - 100;
				}
				else if (Health + 50 <= 100)
				{
					Health += 50;
				}

				KillerPawn->SetHealth(Health);
				KillerPawn->SetShield(Shield);

			}
			if (PlayerController->MyFortPawn && ((KillerPlayerState && KillerPlayerState->Place == 1) || PlayerState->Place == 1))
			{
				if (PlayerState->Place == 1)
				{
					KillerPlayerState = PlayerState;
					KillerPawn = (AFortPlayerPawnAthena*)PlayerController->MyFortPawn;
				}

				auto KillerPlayerController = (AFortPlayerControllerAthena*)KillerPlayerState->Owner;
				auto KillerWeapon = DamageCauser ? DamageCauser->WeaponData : nullptr;

				KillerPlayerController->PlayWinEffects(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause, false);
				KillerPlayerController->ClientNotifyWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);
				KillerPlayerController->ClientNotifyTeamWon(KillerPawn, KillerWeapon, PlayerState->DeathInfo.DeathCause);

				if (KillerPlayerState != PlayerState)
				{
					auto Crown = Utils::FindObject<UFortItemDefinition>(L"/VictoryCrownsGameplay/Items/AGID_VictoryCrown.AGID_VictoryCrown");
					Inventory::GiveItem(KillerPlayerController, Crown, 1, 0);

					KillerPlayerController->ClientSendEndBattleRoyaleMatchForPlayer(true, KillerPlayerController->MatchReport->EndOfMatchResults);

					FAthenaMatchStats& KillerStats = KillerPlayerController->MatchReport->MatchStats;
					FAthenaMatchTeamStats& KillerTeamStats = KillerPlayerController->MatchReport->TeamStats;


					KillerStats.Stats[3] = KillerPlayerState->KillScore;
					KillerStats.Stats[8] = KillerPlayerState->SquadId;
					KillerPlayerController->ClientSendMatchStatsForPlayer(KillerStats);

					KillerTeamStats.Place = KillerPlayerState->Place;
					KillerTeamStats.TotalPlayers = GameState->TotalPlayers;
					KillerPlayerController->ClientSendTeamStatsForPlayer(KillerTeamStats);
				}

				GameState->WinningTeam = KillerPlayerState->TeamIndex;
				GameState->OnRep_WinningTeam();
				GameState->WinningPlayerState = KillerPlayerState;
				GameState->OnRep_WinningPlayerState();
			}
		}
	}

	void ServerSendZiplineState(UObject* Context, FFrame& Stack)
	{
		FZiplinePawnState State;

		Stack.StepCompiledIn(&State);
		Stack.IncrementCode();

		auto Pawn = (AFortPlayerPawn*)Context;

		if (!Pawn)
			return;

		Pawn->ZiplineState = State;

		((void (*)(AFortPlayerPawn*))(Jeremy::ImageBase + 0x6e63fb8))(Pawn);

		if (State.bJumped)
		{
			auto Velocity = Pawn->CharacterMovement->Velocity;
			auto VelocityX = Velocity.X * -0.5f;
			auto VelocityY = Velocity.Y * -0.5f;
			Pawn->LaunchCharacterJump({ VelocityX >= -750 ? min(VelocityX, 750) : -750, VelocityY >= -750 ? min(VelocityY, 750) : -750, 1200 }, false, false, true, true);
		}
	}

	static void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* Building)
	{
		if (!PC || !PC->MyFortPawn || !Building || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PC->PlayerState)->TeamIndex)
			return;

		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;
		if (!PlayerState)
			return;

		auto itemEntry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([](FFortItemEntry& entry)
			{ return entry.ItemDefinition->IsA<UFortEditToolItemDefinition>(); });
		if (!itemEntry)
			return;

		PC->ServerExecuteInventoryItem(itemEntry->ItemGuid);
	}

	static bool CanBePlacedByPlayer(UClass* BuildClass)
	{
		return AFortGameStateAthena::Get()->AllPlayerBuildableClasses.Search([BuildClass](UClass* Class) { return Class == BuildClass; });
	}

	static void ServerEditBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* Building, TSubclassOf<ABuildingSMActor> NewClass, uint8 RotationIterations, bool bMirrored)
	{
		if (!PC || !Building || !NewClass || !Building->IsA<ABuildingSMActor>() || !CanBePlacedByPlayer(NewClass) || Building->TeamIndex != static_cast<AFortPlayerStateAthena*>(PC->PlayerState)->TeamIndex)
			return;

		Building->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		Building->EditingPlayer = nullptr;

		static auto ReplaceBuildingActor = (ABuildingSMActor * (*)(ABuildingSMActor*, unsigned int, UObject*, unsigned int, int, bool, AFortPlayerControllerAthena*))(Jeremy::ImageBase + 0x684a4ec);

		ABuildingSMActor* NewBuild = ReplaceBuildingActor(Building, 1, NewClass, Building->CurrentBuildingLevel, RotationIterations, bMirrored, PC);

		if (NewBuild)
			NewBuild->bPlayerPlaced = true;
	}

	inline void ServerCreateBuildingActor(AFortPlayerControllerAthena* PC, FCreateBuildingActorData CreateBuildingData)
	{
		if (!PC || PC->IsInAircraft())
			return;

		UClass* BuildingClass = PC->BroadcastRemoteClientInfo->RemoteBuildableClass.Get();
		char a7;
		TArray<AActor*> BuildingsToRemove;

		if (!CantBuild(UWorld::GetWorld(), BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, &BuildingsToRemove, &a7))
		{
			auto ResDef = UFortKismetLibrary::GetDefaultObj()->K2_GetResourceItemDefinition(((ABuildingSMActor*)BuildingClass->DefaultObject)->ResourceType);

			if (!Options::bInfiniteMats)
			Inventory::RemoveItem(PC, ResDef, 10);

			ABuildingSMActor* NewBuilding = Utils::SpawnActor<ABuildingSMActor>(CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, PC, BuildingClass);

			NewBuilding->bPlayerPlaced = true;
			NewBuilding->InitializeKismetSpawnedBuildingActor(NewBuilding, PC, true, nullptr);
			NewBuilding->TeamIndex = ((AFortPlayerStateAthena*)PC->PlayerState)->TeamIndex;
			NewBuilding->Team = EFortTeam(NewBuilding->TeamIndex);

			for (size_t i = 0; i < BuildingsToRemove.Num(); i++)
			{
				BuildingsToRemove[i]->K2_DestroyActor();
			}
			UKismetArrayLibrary::Array_Clear((TArray<int32>&)BuildingsToRemove);
		}
	}

	void ServerPlayEmoteItem(AFortPlayerController* PC, UFortItemDefinition* EmoteAsset, float RandomEmoteNumber)
	{
		if (!PC || !EmoteAsset || !PC->MyFortPawn)
			return;

		static UClass* DanceAbility = Utils::FindObject<UClass>(L"/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
		static UClass* SprayAbility = Utils::FindObject<UClass>(L"/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");

		if (!DanceAbility || !SprayAbility)
			return;

		auto EmoteDef = (UAthenaDanceItemDefinition*)EmoteAsset;
		if (!EmoteDef)
			return;

		PC->MyFortPawn->bMovingEmote = EmoteDef->bMovingEmote;
		PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;
		PC->MyFortPawn->bMovingEmoteForwardOnly = EmoteDef->bMoveForwardOnly;
		PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;

		FGameplayAbilitySpec Spec{};
		UGameplayAbility* Ability = nullptr;
		if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
		{
			Ability = (UGameplayAbility*)SprayAbility->DefaultObject;
		}

		if (Ability == nullptr)
		{
			Ability = (UGameplayAbility*)DanceAbility->DefaultObject;
		}
		if (Ability)
		{
			FGameplayAbilitySpec Spec{};
			AbilitySpecConstructor(&Spec, Ability, 1, -1, EmoteAsset);
			GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec, 0);
		}
	}

	void ServerPlaySquadQuickChatMessage(AFortPlayerControllerAthena* PC, FAthenaQuickChatActiveEntry ChatEntry, __int64)
	{
		static ETeamMemberState MemberStates[10] =
		{
			ETeamMemberState::ChatBubble,
			ETeamMemberState::EnemySpotted,
			ETeamMemberState::NeedMaterials,
			ETeamMemberState::NeedBandages,
			ETeamMemberState::NeedShields,
			ETeamMemberState::NeedAmmoHeavy,
			ETeamMemberState::NeedAmmoLight,
			ETeamMemberState::FIRST_CHAT_MESSAGE,
			ETeamMemberState::NeedAmmoMedium,
			ETeamMemberState::NeedAmmoShells
		};

		auto PlayerState = reinterpret_cast<AFortPlayerStateAthena*>(PC->PlayerState);

		PlayerState->ReplicatedTeamMemberState = MemberStates[ChatEntry.Index];

		PlayerState->OnRep_ReplicatedTeamMemberState();

		static auto EmojiComm = Utils::FindObject<UAthenaEmojiItemDefinition>(L"/Game/Athena/Items/Cosmetics/Dances/Emoji/Emoji_Comm.Emoji_Comm");

		if (EmojiComm)
		{
			PC->ServerPlayEmoteItem(EmojiComm, 0);
		}
	}

	FFortItemEntry* FindEntry(AFortPlayerController* PC, UFortItemDefinition* Def)
	{

		auto Inventory = PC->WorldInventory;
		if (!Inventory && PC->IsA(AFortAthenaAIBotController::StaticClass()))
		{
			Inventory = ((AFortAthenaAIBotController*)PC)->Inventory;
		}


		for (size_t i = 0; i < Inventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (!Inventory->Inventory.ReplicatedEntries.IsValidIndex(i))
				continue;
			if (Inventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
				return &Inventory->Inventory.ReplicatedEntries[i];
		}
		return nullptr;
	}


	void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToStopEditing)
	{
		if (!PlayerController || !BuildingActorToStopEditing || BuildingActorToStopEditing->EditingPlayer != (AFortPlayerStateAthena*)PlayerController->PlayerState || !PlayerController->MyFortPawn)
			return;

		BuildingActorToStopEditing->SetNetDormancy(ENetDormancy::DORM_DormantAll);
		BuildingActorToStopEditing->EditingPlayer = nullptr;

		static auto EditToolDefinition = Utils::FindObject<UFortItemDefinition>(L"/Game/Items/Weapons/BuildingTools/EditTool.EditTool");

		if (!PlayerController->MyFortPawn->CurrentWeapon || PlayerController->MyFortPawn->CurrentWeapon->WeaponData != EditToolDefinition)
		{
			FFortItemEntry* ItemEntry = FindEntry(PlayerController, EditToolDefinition);

			if (!ItemEntry)
				return;

			PlayerController->ServerExecuteInventoryItem(ItemEntry->ItemGuid);
		}

		AFortWeap_EditingTool* EditTool = Utils::Cast<AFortWeap_EditingTool>(PlayerController->MyFortPawn->CurrentWeapon);

		if (EditTool)
		{
			EditTool->EditActor = nullptr;
			EditTool->OnRep_EditActor();
		}
	}

	static void ServerRepairBuildingActor(AFortPlayerControllerAthena* PC, ABuildingSMActor* Building)
	{
		if (!PC)
			return;

		auto Price = (int32)std::floor((10.f * (1.f - Building->GetHealthPercent())) * 0.75f);
		auto res = UFortKismetLibrary::K2_GetResourceItemDefinition(Building->ResourceType);
		auto itemEntry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([res](FFortItemEntry& entry)
			{ return entry.ItemDefinition == res; });

		Inventory::RemoveItem(PC, res, Price);

		Building->RepairBuilding(PC, Price);
	}

	static inline void (*OnDamageServerOG)(ABuildingSMActor*, float, FGameplayTagContainer, FVector, FHitResult, AFortPlayerControllerAthena*, AActor*, FGameplayEffectContextHandle);
	void OnDamageServer(ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext) {
		auto GameState = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState);
		if (!InstigatedBy || Actor->bPlayerPlaced || Actor->GetHealth() == 1 || Actor->IsA(UObject::FindClassFast("B_Athena_VendingMachine_C")) || Actor->IsA(GameState->MapInfo->LlamaClass)) return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		if (!DamageCauser || !DamageCauser->IsA<AFortWeapon>() || !((AFortWeapon*)DamageCauser)->WeaponData->IsA<UFortWeaponMeleeItemDefinition>()) return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		static auto PickaxeTag = UKismetStringLibrary::Conv_StringToName(L"Weapon.Melee.Impact.Pickaxe");
		auto entry = DamageTags.GameplayTags.Search([](FGameplayTag& entry) {
			return entry.TagName.ComparisonIndex == PickaxeTag.ComparisonIndex;
			});
		if (!entry)
			return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		auto Resource = UFortKismetLibrary::K2_GetResourceItemDefinition(Actor->ResourceType);
		if (!Resource)
			return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
		auto MaxMat = (int32)Utils::EvaluateScalableFloat(Resource->MaxStackSize);
		FCurveTableRowHandle& BuildingResourceAmountOverride = Actor->BuildingResourceAmountOverride;
		int ResCount = 0;
		if (Actor->BuildingResourceAmountOverride.RowName.ComparisonIndex > 0)
		{
			float Out;
			UDataTableFunctionLibrary::EvaluateCurveTableRow(Actor->BuildingResourceAmountOverride.CurveTable, Actor->BuildingResourceAmountOverride.RowName, 0.f, nullptr, &Out, FString());
			float RC = Out / (Actor->GetMaxHealth() / Damage);
			ResCount = (int)round(RC);
		}
		if (ResCount > 0)
		{
			auto itemEntry = InstigatedBy->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
				return entry.ItemDefinition == Resource;
				});
			if (itemEntry)
			{
				itemEntry->Count += ResCount;
				if (itemEntry->Count > MaxMat)
				{
					Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), itemEntry->ItemDefinition, itemEntry->Count - MaxMat, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, InstigatedBy->MyFortPawn);
					itemEntry->Count = MaxMat;
				}
				Inventory::ReplaceEntry(InstigatedBy, *itemEntry);
			}
			else
			{
				if (ResCount > MaxMat)
				{
					Inventory::SpawnPickup(InstigatedBy->Pawn->K2_GetActorLocation(), Resource, ResCount - MaxMat, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, InstigatedBy->MyFortPawn);
					ResCount = MaxMat;
				}
				Inventory::GiveItem(InstigatedBy, Resource, ResCount, 0);
			}
		}

		InstigatedBy->ClientReportDamagedResourceBuilding(Actor, ResCount == 0 ? EFortResourceType::None : Actor->ResourceType, ResCount, false, Damage == 100.f);
		return OnDamageServerOG(Actor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	void ServerClientIsReadyToRespawn(UObject* Context, FFrame& Stack)
	{
		Stack.IncrementCode();

		auto PlayerController = (AFortPlayerControllerAthena*)Context;

		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;
		auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;

		if (PlayerState->RespawnData.bRespawnDataAvailable && PlayerState->RespawnData.bServerIsReady)
		{
			auto RespawnData = PlayerState->RespawnData;
			FTransform SpawnTransform{};

			FQuat Rotation = PlayerState->RespawnData.RespawnRotation;
			SpawnTransform.Translation = PlayerState->RespawnData.RespawnLocation;
			SpawnTransform.Rotation = Rotation;

			auto Scale = FVector(1, 1, 1);
			SpawnTransform.Scale3D = Scale;

			auto NewPawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, SpawnTransform);
			auto Pawn = (AFortPlayerPawnAthena*)NewPawn;

			PlayerController->Possess(NewPawn);
			PlayerController->RespawnPlayerAfterDeath(true);

			Pawn->SetHealth(100.f);

			if (Options::bInfiniteLateGame)
			{
				Inventory::RemoveAllItems(PlayerController);

				UAthenaPickaxeItemDefinition* PickDef;
				FFortAthenaLoadout& CosmecticLoadoutPC = PlayerController->CosmeticLoadoutPC;
				PickDef = CosmecticLoadoutPC.Pickaxe != nullptr ? CosmecticLoadoutPC.Pickaxe : Utils::FindObject<UAthenaPickaxeItemDefinition>(L"/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
				if (PickDef) {
					Inventory::GiveItem(PlayerController, PickDef->WeaponDefinition, 1, 0);
				}
				else {
				}

				for (size_t i = 0; i < GameMode->StartingItems.Num(); i++)
				{
					if (GameMode->StartingItems[i].Count > 0)
					{
						Inventory::GiveItem(PlayerController, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count, 0);
					}
				}

				Abilities::GiveAbilitySet(Utils::FindObject<UFortAbilitySet>(L"/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer"), PlayerState);

				auto Shotgun = Lategame::GetShotguns();
				auto AssaultRifle = Lategame::GetAssaultRifles();
				auto Sniper = Lategame::GetSnipers();
				auto Heal = Lategame::GetHeals();
				auto HealSlot2 = Lategame::GetHeals();

				int ShotgunClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Shotgun.Item)->ClipSize;
				int AssaultRifleClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)AssaultRifle.Item)->ClipSize;
				int SniperClipSize = Inventory::GetStats((UFortWeaponItemDefinition*)Sniper.Item)->ClipSize;
				int HealClipSize = Heal.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)Heal.Item)->ClipSize : 0;
				int HealSlot2ClipSize = HealSlot2.Item->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)HealSlot2.Item)->ClipSize : 0;

				Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Wood), 500, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Stone), 500, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Metal), 500, 0);

				Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Assault), 250, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Shotgun), 50, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Submachine), 400, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Rocket), 6, 0);
				Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Sniper), 20, 0);

				Inventory::GiveItem(PlayerController, AssaultRifle.Item, AssaultRifle.Count, AssaultRifleClipSize);
				Inventory::GiveItem(PlayerController, Shotgun.Item, Shotgun.Count, ShotgunClipSize);
				Inventory::GiveItem(PlayerController, Sniper.Item, Sniper.Count, SniperClipSize);
				Inventory::GiveItem(PlayerController, Heal.Item, Heal.Count, HealClipSize);
				Inventory::GiveItem(PlayerController, HealSlot2.Item, HealSlot2.Count, HealSlot2ClipSize);
			}

			if (Options::bInfiniteLateGame)
			{
				Pawn->SetShield(100.f);
			}

			PlayerState->RespawnData.bClientIsReady = true;
		}
	}

	void (*OnReloadOG)(AFortWeapon* Weapon, int RemoveCount);
	void OnReload(AFortWeapon* Weapon, int RemoveCount)
	{
		if (Options::bInfiniteAmmo)
			return;

		OnReloadOG(Weapon, RemoveCount);
		auto WeaponDef = Weapon->WeaponData;
		if (!WeaponDef)
			return;

		auto AmmoDef = WeaponDef->GetAmmoWorldItemDefinition_BP();
		if (!AmmoDef)
			return;
		AFortPlayerPawnAthena* Pawn = (AFortPlayerPawnAthena*)Weapon->GetOwner();
		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC->IsA(AFortPlayerControllerAthena::StaticClass()))
			return;

		Inventory::RemoveItem(PC, AmmoDef, RemoveCount);
		Inventory::UpdateLoadedAmmo(PC, Weapon, RemoveCount);

		return;
	}

	void TeleportPlayerPawn(UObject* Context, FFrame& Stack, bool* Ret)
	{
		UObject* WorldContextObject;
		AFortPlayerPawn* PlayerPawn;
		FVector DestLocation;
		FRotator DestRotation;
		bool bIgnoreCollision;
		bool bIgnoreSupplementalKillVolumeSweep;

		Stack.StepCompiledIn(&WorldContextObject);
		Stack.StepCompiledIn(&PlayerPawn);
		Stack.StepCompiledIn(&DestLocation);
		Stack.StepCompiledIn(&DestRotation);
		Stack.StepCompiledIn(&bIgnoreCollision);
		Stack.StepCompiledIn(&bIgnoreSupplementalKillVolumeSweep);
		Stack.IncrementCode();

		PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
		*Ret = true;
	}

	void Hook()
	{
		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x125, ServerAcknowledgePossession, (LPVOID*)&ServerAcknowledgePossessionOG);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x21c01dc), ServerLoadingScreenDropped, (LPVOID*)&ServerLoadingScreenDroppedOG);

	    MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x6eb2278), ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x7115D20), OnReload, (LPVOID*)&OnReloadOG);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0xd741bc), GetPlayerViewPoint, (LPVOID*)&GetPlayerViewPointOG);

		Utils::HookVTable(UFortControllerComponent_Aircraft::GetDefaultObj(), 0x9F, ServerAttemptAircraftJump, nullptr);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0xaa4c9d), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);

		Utils::ExecHook(L"/Script/FortniteGame.FortPlayerPawn.ServerSendZiplineState", ServerSendZiplineState);

		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x254, ServerCreateBuildingActor, nullptr);

		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x25B, ServerBeginEditingBuildingActor, nullptr);
		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x256, ServerEditBuildingActor, nullptr);
		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x259, ServerEndEditingBuildingActor, nullptr);
		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x250, ServerRepairBuildingActor, nullptr);

		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x56A, ServerPlaySquadQuickChatMessage, nullptr);

		if (Options::bInfiniteLateGame)
		{
			Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x561, ServerClientIsReadyToRespawn, nullptr);
		}

		//MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x93B2488), ServerClientIsReadyToRespawn, nullptr);

		MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x714bd34), OnDamageServer, (LPVOID*)&OnDamageServerOG);

		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x1ED, ServerPlayEmoteItem, nullptr);

		Utils::ExecHook(L"/Script/FortniteGame.FortMissionLibrary.TeleportPlayerPawn", TeleportPlayerPawn);
	}
}