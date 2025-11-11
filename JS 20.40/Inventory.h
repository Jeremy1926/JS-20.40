#include "pch.h"
#include "framework.h"

namespace Inventory
{
	inline bool CompareGuids(FGuid Guid1, FGuid Guid2)
	{
		if (Guid1.A == Guid2.A
			&& Guid1.B == Guid2.B
			&& Guid1.C == Guid2.C
			&& Guid1.D == Guid2.D) {
			return true;
		}
		else {
			return false;
		}
	}

	inline void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{
		UFortWorldItem* Item = Utils::Cast<UFortWorldItem>(Def->CreateTemporaryItemInstanceBP(Count, 0));
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);
		PC->WorldInventory->Inventory.MarkItemDirty(Item->ItemEntry);
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	inline void UpdateStack(AFortPlayerController* PC, bool Update, FFortItemEntry* EntryToUpdate = nullptr)
	{
		PC->WorldInventory->bRequiresLocalUpdate = true;
		PC->WorldInventory->HandleInventoryLocalUpdate();
		PC->HandleWorldInventoryLocalUpdate();
		PC->ClientForceUpdateQuickbar(EFortQuickBars::Primary);
		PC->ClientForceUpdateQuickbar(EFortQuickBars::Secondary);

		if (Update)
		{

			PC->WorldInventory->Inventory.MarkItemDirty(*EntryToUpdate);
		}
		else
		{
			PC->WorldInventory->Inventory.MarkArrayDirty();
		}
	}

	inline FFortItemEntry* GiveStack(AFortPlayerControllerAthena* PC, UFortItemDefinition* Def, int Count = 1, bool GiveLoadedAmmo = false, int LoadedAmmo = 0, bool Toast = false)
	{
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);

		Item->SetOwningControllerForTemporaryItem(PC);
		Item->OwnerInventory = PC->WorldInventory;
		Item->ItemEntry.ItemDefinition = Def;
		Item->ItemEntry.Count = Count;


		if (GiveLoadedAmmo)
		{
			Item->ItemEntry.LoadedAmmo = LoadedAmmo;
		}
		Item->ItemEntry.ReplicationKey++;

		PC->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		PC->WorldInventory->Inventory.ItemInstances.Add(Item);

		UpdateStack(PC, false);
		return &Item->ItemEntry;
	}

	inline void UpdateInventory(AFortPlayerController* PC, FFortItemEntry& Entry)
	{
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (Inventory::CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid, Entry.ItemGuid))
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	inline void GiveItemStack(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo)
	{
		EEvaluateCurveTableResult Result;
		float OutXY = 0;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &OutXY, FString());
		if (!Def->MaxStackSize.Curve.CurveTable || OutXY <= 0)
			OutXY = Def->MaxStackSize.Value;
		FFortItemEntry* Found = nullptr;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == Def)
			{
				Found = &PC->WorldInventory->Inventory.ReplicatedEntries[i];
				PC->WorldInventory->Inventory.ReplicatedEntries[i].Count += Count;
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].Count > OutXY)
				{
					PC->WorldInventory->Inventory.ReplicatedEntries[i].Count = OutXY;
				}
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				UpdateInventory(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->HandleInventoryLocalUpdate();
				return;
			}
		}

		if (!Found)
		{
			GiveItem(PC, Def, Count, LoadedAmmo);
		}
	}

	inline void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count)
	{
		bool Remove = false;
		FGuid guid;
		for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			auto& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
			if (Entry.ItemDefinition == Def)
			{
				Entry.Count -= Count;
				if (Entry.Count <= 0)
				{
					PC->WorldInventory->Inventory.ReplicatedEntries[i].StateValues.Free();
					PC->WorldInventory->Inventory.ReplicatedEntries.RemoveSingle(i);
					Remove = true;
					guid = Entry.ItemGuid;
				}
				else
				{
					PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
					UpdateInventory(PC, Entry);
					PC->WorldInventory->HandleInventoryLocalUpdate();
					return;
				}
				break;
			}
		}

		if (Remove)
		{
			for (size_t i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
			{
				if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->GetItemGuid(), guid))
				{
					PC->WorldInventory->Inventory.ItemInstances.RemoveSingle(i);
					break;
				}
			}
		}

		PC->WorldInventory->Inventory.MarkArrayDirty();
		PC->WorldInventory->HandleInventoryLocalUpdate();
	}

	inline void RemoveItem(AFortPlayerController* PC, FGuid Guid, int Count)
	{
		for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (CompareGuids(Guid, Entry.ItemGuid))
			{
				RemoveItem(PC, Entry.ItemDefinition, Count);
				break;
			}
		}
	}

	inline FFortItemEntry* FindEntry(AFortPlayerController* PC, FGuid Guid)
	{
		for (auto& Entry : PC->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (CompareGuids(Entry.ItemGuid, Guid))
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	inline FFortItemEntry* FindItemEntry(AFortPlayerController* PC, UFortItemDefinition* ItemDef)
	{
		if (!PC || !PC->WorldInventory || !ItemDef)
			return nullptr;
		for (int i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); ++i)
		{
			if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition == ItemDef)
			{
				return &PC->WorldInventory->Inventory.ReplicatedEntries[i];
			}
		}
		return nullptr;
	}

	inline FFortItemEntry* FindItemEntry(AFortPlayerControllerAthena* PC, FGuid Guid) {
		for (FFortItemEntry& ItemEntry : PC->WorldInventory->Inventory.ReplicatedEntries) {
			if (CompareGuids(ItemEntry.ItemGuid, Guid)) {
				return &ItemEntry;
			}
		}

		return nullptr;
	}


	inline UFortWorldItem* FindItemInstance(AFortInventory* inv, UFortItemDefinition* ItemDefinition)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (ItemInstance->ItemEntry.ItemDefinition == ItemDefinition)
				return ItemInstance;
		}

		return nullptr;
	}

	inline UFortWorldItem* FindItemInstance(AFortInventory* inv, const FGuid& Guid)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (CompareGuids(ItemInstance->ItemEntry.ItemGuid, Guid))
				return ItemInstance;
		}

		return nullptr;
	}

	static void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PC, FGuid Guid)
	{
		if (!PC || !PC->MyFortPawn)
			return;

		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Guid))
			{
				UFortWeaponItemDefinition* DefToEquip = (UFortWeaponItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
				FGuid TrackerGuid = PC->WorldInventory->Inventory.ReplicatedEntries[i].TrackerGuid;
				if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortGadgetItemDefinition::StaticClass()))
				{
					DefToEquip = ((UFortGadgetItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition)->GetWeaponItemDefinition();
				}
				else if (PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass())) {
					auto DecoItemDefinition = (UFortDecoItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
					PC->MyFortPawn->PickUpActor(nullptr, DecoItemDefinition);
					PC->MyFortPawn->CurrentWeapon->ItemEntryGuid = Guid;
					static auto FortDecoTool_ContextTrapStaticClass = Utils::FindObject<UClass>(L"/Script/FortniteGame.FortDecoTool_ContextTrap");
					if (PC->MyFortPawn->CurrentWeapon->IsA(FortDecoTool_ContextTrapStaticClass))
					{
						reinterpret_cast<AFortDecoTool_ContextTrap*>(PC->MyFortPawn->CurrentWeapon)->ContextTrapItemDefinition = (UFortContextTrapItemDefinition*)PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition;
					}
					return;
				}
				PC->MyFortPawn->EquipWeaponDefinition(DefToEquip, Guid, TrackerGuid, false);
				break;
			}
		}

		return;
	}

	inline bool IsPrimaryQuickbar(UFortItemDefinition* Def)
	{
		return Def->IsA(UFortConsumableItemDefinition::StaticClass()) || Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) || Def->IsA(UFortGadgetItemDefinition::StaticClass());
	}

	inline EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			return EFortQuickBars::Primary;

		return EFortQuickBars::Secondary;
	}

	inline bool IsInventoryFull(AFortPlayerController* PC)
	{
		int ItemNb = 0;
		auto InstancesPtr = &PC->WorldInventory->Inventory.ItemInstances;
		for (int i = 0; i < InstancesPtr->Num(); i++)
		{
			if (InstancesPtr->operator[](i))
			{
				if (GetQuickBars(InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
				{
					ItemNb++;

					if (ItemNb >= 5)
					{
						break;
					}
				}
			}
		}

		return ItemNb >= 5;
	}

	inline void ModifyEntry(AFortPlayerControllerAthena* PC, FFortItemEntry& Entry)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ItemInstances.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry.ItemGuid, Entry.ItemGuid))
			{
				PC->WorldInventory->Inventory.ItemInstances[i]->ItemEntry = Entry;
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	inline void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Weapon->ItemEntryGuid))
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo = Weapon->AmmoCount;
				UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	inline void UpdateLoadedAmmo(AFortPlayerController* PC, AFortWeapon* Weapon, int AmountToAdd)
	{
		for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
		{
			if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Weapon->ItemEntryGuid))
			{
				PC->WorldInventory->Inventory.ReplicatedEntries[i].LoadedAmmo += AmountToAdd;
				ModifyEntry((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				UpdateInventory((AFortPlayerControllerAthena*)PC, PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				PC->WorldInventory->Inventory.MarkItemDirty(PC->WorldInventory->Inventory.ReplicatedEntries[i]);
				break;
			}
		}
	}

	inline float GetMaxStack(UFortItemDefinition* Def)
	{
		if (!Def->MaxStackSize.Curve.CurveTable)
			return Def->MaxStackSize.Value;
		EEvaluateCurveTableResult Result;
		float Ret;
		((UDataTableFunctionLibrary*)UDataTableFunctionLibrary::StaticClass()->DefaultObject)->EvaluateCurveTableRow(Def->MaxStackSize.Curve.CurveTable, Def->MaxStackSize.Curve.RowName, 0, &Result, &Ret, FString());
		return Ret;
	}

	inline void (*ServerHandlePickupOG)(AFortPlayerPawn* Pawn, AFortPickup* Pickup, float InFlyTime, FVector InStartDirection, bool bPlayPickupSound);
	inline void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound)
	{
		if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
			return;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

		UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
		FFortItemEntry* FoundEntry = nullptr;
		FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;
		float MaxStackSize = GetMaxStack(Def);
		bool Stackable = Def->IsStackable();
		UFortItemDefinition* PickupItemDef = PickupEntry.ItemDefinition;
		bool Found = false;
		FFortItemEntry* GaveEntry = nullptr;

		if (IsInventoryFull(PC))
		{
			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

				Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				if (Stackable)
				{
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

						if (Entry.ItemDefinition == PickupItemDef)
						{
							Found = true;
							if ((MaxStackSize - Entry.Count) > 0)
							{
								Entry.Count += PickupEntry.Count;

								if (Entry.Count > MaxStackSize)
								{
									Utils::SpawnStack((AFortPlayerPawnAthena*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
									Entry.Count = MaxStackSize;
								}

								PC->WorldInventory->Inventory.MarkItemDirty(Entry);
							}
							else
							{
								if (IsPrimaryQuickbar(PickupItemDef))
								{
									GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
								}
							}
							break;
						}
					}
					if (!Found)
					{
						for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
						{
							if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Pawn->CurrentWeapon->GetInventoryGUID()))
							{
								PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
								break;
							}
						}
						GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
					}

					Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
					Pickup->PickupLocationData.FlyTime = 0.3f;
					Pickup->PickupLocationData.ItemOwner = Pawn;
					Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
					Pickup->PickupLocationData.PickupTarget = Pawn;
					Pickup->OnRep_PickupLocationData();

					Pickup->bPickedUp = true;
					Pickup->OnRep_bPickedUp();
					return;
				}

				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Pawn->CurrentWeapon->GetInventoryGUID()))
					{
						PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
						break;
					}
				}
			}
		}

		if (!IsInventoryFull(PC))
		{
			if (Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

					if (Entry.ItemDefinition == PickupItemDef)
					{
						Found = true;
						if ((MaxStackSize - Entry.Count) > 0)
						{
							Entry.Count += PickupEntry.Count;

							if (Entry.Count > MaxStackSize)
							{
								Utils::SpawnStack((AFortPlayerPawnAthena*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
								Entry.Count = MaxStackSize;
							}

							PC->WorldInventory->Inventory.MarkItemDirty(Entry);
						}
						else
						{
							if (IsPrimaryQuickbar(PickupItemDef))
							{
								GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
							}
						}
						break;
					}
				}

				if (!Found)
				{
					GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
				}

				Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
			else {
				GiveItem(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
		}

		Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
		Pickup->PickupLocationData.FlyTime = 0.3f;
		Pickup->PickupLocationData.ItemOwner = Pawn;
		Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
		Pickup->PickupLocationData.PickupTarget = Pawn;
		Pickup->OnRep_PickupLocationData();

		Pickup->bPickedUp = true;
		Pickup->OnRep_bPickedUp();
	}


	/*inline void (*ServerHandlePickupOG)(AFortPlayerPawn* Pawn, AFortPickup* Pickup, float InFlyTime, FVector InStartDirection, bool bPlayPickupSound);
	inline void ServerHandlePickup(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, float InFlyTime, const FVector& InStartDirection, bool bPlayPickupSound)
	{
		if (!Pickup || !Pawn || !Pawn->Controller || Pickup->bPickedUp)
			return;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;

		UFortItemDefinition* Def = Pickup->PrimaryPickupItemEntry.ItemDefinition;
		FFortItemEntry* FoundEntry = nullptr;
		FFortItemEntry& PickupEntry = Pickup->PrimaryPickupItemEntry;
		float MaxStackSize = GetMaxStack(Def);
		bool Stackable = Def->IsStackable();
		UFortItemDefinition* PickupItemDef = PickupEntry.ItemDefinition;
		bool Found = false;
		FFortItemEntry* GaveEntry = nullptr;

		if (IsInventoryFull(PC))
		{
			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);

				Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (!Pawn->CurrentWeapon->WeaponData->IsA(UFortWeaponMeleeItemDefinition::StaticClass()))
			{
				if (Stackable)
				{
					for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
					{
						FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

						if (Entry.ItemDefinition == PickupItemDef)
						{
							Found = true;
							if ((MaxStackSize - Entry.Count) > 0)
							{
								Entry.Count += PickupEntry.Count;

								if (Entry.Count > MaxStackSize)
								{
									Utils::SpawnStack((AFortPlayerPawnAthena*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
									Entry.Count = MaxStackSize;
								}

								PC->WorldInventory->Inventory.MarkItemDirty(Entry);
							}
							else
							{
								if (IsPrimaryQuickbar(PickupItemDef))
								{
									GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
								}
							}
							break;
						}
					}
					if (!Found)
					{
						for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
						{
							if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Pawn->CurrentWeapon->GetInventoryGUID()))
							{
								PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
								break;
							}
						}
						GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
					}

					Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
					Pickup->PickupLocationData.FlyTime = 0.3f;
					Pickup->PickupLocationData.ItemOwner = Pawn;
					Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
					Pickup->PickupLocationData.PickupTarget = Pawn;
					Pickup->OnRep_PickupLocationData();

					Pickup->bPickedUp = true;
					Pickup->OnRep_bPickedUp();
					return;
				}

				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					if (CompareGuids(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid, Pawn->CurrentWeapon->GetInventoryGUID()))
					{
						PC->ServerAttemptInventoryDrop(Pawn->CurrentWeapon->GetInventoryGUID(), PC->WorldInventory->Inventory.ReplicatedEntries[i].Count, false);
						break;
					}
				}
			}
		}

		if (!IsInventoryFull(PC))
		{
			if (Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Stackable && !Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				for (size_t i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
				{
					FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];

					if (Entry.ItemDefinition == PickupItemDef)
					{
						Found = true;
						if ((MaxStackSize - Entry.Count) > 0)
						{
							Entry.Count += PickupEntry.Count;

							if (Entry.Count > MaxStackSize)
							{
								Utils::SpawnStack((AFortPlayerPawnAthena*)PC->Pawn, PickupItemDef, Entry.Count - MaxStackSize);
								Entry.Count = MaxStackSize;
							}

							PC->WorldInventory->Inventory.MarkItemDirty(Entry);
						}
						else
						{
							if (IsPrimaryQuickbar(PickupItemDef))
							{
								GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count);
							}
						}
						break;
					}
				}

				if (!Found)
				{
					GaveEntry = GiveStack(PC, PickupItemDef, PickupEntry.Count, false, 0, true);
				}

				Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
				Pickup->PickupLocationData.FlyTime = 0.3f;
				Pickup->PickupLocationData.ItemOwner = Pawn;
				Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
				Pickup->PickupLocationData.PickupTarget = Pawn;
				Pickup->OnRep_PickupLocationData();

				Pickup->bPickedUp = true;
				Pickup->OnRep_bPickedUp();
				return;
			}

			if (Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) || Pickup->PrimaryPickupItemEntry.ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()))
			{
				GiveItemStack(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
			else {
				GiveItem(PC, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, Pickup->PrimaryPickupItemEntry.LoadedAmmo);
			}
		}

		Pickup->PickupLocationData.bPlayPickupSound = bPlayPickupSound;
		Pickup->PickupLocationData.FlyTime = 0.3f;
		Pickup->PickupLocationData.ItemOwner = Pawn;
		Pickup->PickupLocationData.PickupGuid = Pickup->PrimaryPickupItemEntry.ItemGuid;
		Pickup->PickupLocationData.PickupTarget = Pawn;
		Pickup->OnRep_PickupLocationData();

		Pickup->bPickedUp = true;
		Pickup->OnRep_bPickedUp();
	}*/

	inline int GetLevel(const FDataTableCategoryHandle& CategoryHandle)
	{
		auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		auto GameState = (AFortGameStateAthena*)GameMode->GameState;

		if (!CategoryHandle.DataTable)
			return 0;

		if (!CategoryHandle.RowContents.ComparisonIndex)
			return 0;

		int Level = 0;
		FFortLootLevelData* LootLevelData = nullptr;

		for (auto& LootLevelDataPair : CategoryHandle.DataTable->RowMap)
		{
			FFortLootLevelData* CurrentData = (FFortLootLevelData*)LootLevelDataPair.Value();

			if (CurrentData->Category != CategoryHandle.RowContents ||
				CurrentData->LootLevel > GameState->WorldLevel ||
				CurrentData->LootLevel <= Level)
				continue;

			Level = CurrentData->LootLevel;
			LootLevelData = CurrentData;
		}

		if (LootLevelData)
		{
			auto subbed = LootLevelData->MaxItemLevel - LootLevelData->MinItemLevel;

			if (subbed <= -1)
				subbed = 0;
			else
			{
				auto calc = (int)(((float)rand() / 32767) * (float)(subbed + 1));
				if (calc <= subbed)
					subbed = calc;
			}

			return subbed + LootLevelData->MinItemLevel;
		}

		return 0;
	}

	inline EFortQuickBars GetQuickbar(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition)
			return EFortQuickBars::Max_None;

		return ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>() || ItemDefinition->IsA<UFortResourceItemDefinition>() || ItemDefinition->IsA<UFortAmmoItemDefinition>() || ItemDefinition->IsA<UFortTrapItemDefinition>() || ItemDefinition->IsA<UFortBuildingItemDefinition>() || ItemDefinition->IsA<UFortEditToolItemDefinition>() || ((UFortWorldItemDefinition*)ItemDefinition)->bForceIntoOverflow ? EFortQuickBars::Secondary : EFortQuickBars::Primary;
	}

	inline void (*NetMulticast_Athena_BatchedDamageCuesOG)(AFortPlayerPawnAthena* Pawn, FAthenaBatchedDamageGameplayCues_Shared SharedData, FAthenaBatchedDamageGameplayCues_NonShared NonSharedData);
	inline void NetMulticast_Athena_BatchedDamageCues(AFortPlayerPawnAthena* Pawn, FAthenaBatchedDamageGameplayCues_Shared SharedData, FAthenaBatchedDamageGameplayCues_NonShared NonSharedData)
	{
		if (!Pawn)
			return;

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC) return NetMulticast_Athena_BatchedDamageCuesOG(Pawn, SharedData, NonSharedData);

		if (Pawn->CurrentWeapon) {
			FFortItemEntry* ItemEntry = Inventory::FindItemEntry(PC, Pawn->CurrentWeapon->ItemEntryGuid);
			if (!ItemEntry) return NetMulticast_Athena_BatchedDamageCuesOG(Pawn, SharedData, NonSharedData);
			ItemEntry->LoadedAmmo = Pawn->CurrentWeapon->AmmoCount;
			Inventory::UpdateInventory(PC, *ItemEntry);
		}

		return NetMulticast_Athena_BatchedDamageCuesOG(Pawn, SharedData, NonSharedData);
	}

	inline FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition* Def)
	{
		if (!Def || !Def->WeaponStatHandle.DataTable)
			return nullptr;

		auto Val = Def->WeaponStatHandle.DataTable->RowMap.Search([Def](FName& Key, uint8_t* Value) {
			return Def->WeaponStatHandle.RowName == Key && Value;
			});

		return Val ? *(FFortRangedWeaponStats**)Val : nullptr;
	}

	inline FFortItemEntry* MakeItemEntry(UFortItemDefinition* ItemDefinition, int32 Count, int32 Level)
	{
		FFortItemEntry* ItemEntry = new FFortItemEntry();

		ItemEntry->MostRecentArrayReplicationKey = -1;
		ItemEntry->ReplicationID = -1;
		ItemEntry->ReplicationKey = -1;

		ItemEntry->ItemDefinition = ItemDefinition;
		ItemEntry->Count = Count;
		ItemEntry->Durability = 1.f;
		ItemEntry->GameplayAbilitySpecHandle = FGameplayAbilitySpecHandle(-1);
		ItemEntry->ParentInventory.ObjectIndex = -1;
		ItemEntry->Level = Level;
		if (auto Weapon = ItemDefinition->Cast<UFortWeaponItemDefinition>())
		{
			auto Stats = GetStats(Weapon);
			ItemEntry->LoadedAmmo = Stats->ClipSize;
			if (Weapon->bUsesPhantomReserveAmmo)
				ItemEntry->PhantomReserveAmmo = Stats->InitialClips * Stats->ClipSize;
		}

		return ItemEntry;
	}

	inline static void TriggerInventoryUpdate(AFortPlayerController* PC, FFortItemEntry* Entry)
	{
		AFortInventory* Inventory;
		if (auto Bot = PC->Cast<AFortAthenaAIBotController>())
		{
			Inventory = Bot->Inventory;
		}
		else
		{
			Inventory = PC->WorldInventory;
		}
		if (!PC)
			return;
		Inventory->bRequiresLocalUpdate = true;
		Inventory->HandleInventoryLocalUpdate();

		return Entry ? Inventory->Inventory.MarkItemDirty(*Entry) : Inventory->Inventory.MarkArrayDirty();
	}

	inline AFortPickupAthena* SpawnPickup(FVector Loc, FFortItemEntry& Entry, EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr, int OverrideCount = -1, bool Toss = true, bool RandomRotation = true, bool bCombine = true)
	{
		if (!&Entry)
			return nullptr;
		AFortPickupAthena* NewPickup = Utils::SpawnActor<AFortPickupAthena>(Loc, {});
		if (!NewPickup)
			return nullptr;

		NewPickup->bRandomRotation = RandomRotation;
		NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry.ItemDefinition;
		NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry.LoadedAmmo;
		NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != -1 ? OverrideCount : Entry.Count;
		NewPickup->PrimaryPickupItemEntry.PhantomReserveAmmo = Entry.PhantomReserveAmmo;
		NewPickup->OnRep_PrimaryPickupItemEntry();
		NewPickup->PawnWhoDroppedPickup = Pawn;

		NewPickup->TossPickup(Loc, Pawn, -1, Toss, true, SourceTypeFlag, SpawnSource);
		NewPickup->bTossedFromContainer = SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox;
		if (NewPickup->bTossedFromContainer)
			NewPickup->OnRep_TossedFromContainer();

		NewPickup->SetNetDormancy(ENetDormancy::DORM_DormantAll);

		return NewPickup;
	}

	inline AFortPickupAthena* SpawnPickup(FVector Loc, UFortItemDefinition* ItemDefinition, int Count, int LoadedAmmo = 0, EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr, bool Toss = true, bool bRandomRotation = true)
	{
		return SpawnPickup(Loc, *MakeItemEntry(ItemDefinition, Count, 0), SourceTypeFlag, SpawnSource, Pawn, -1, Toss, true, bRandomRotation);
	}

	inline static void ReplaceEntry(AFortPlayerController* PC, FFortItemEntry& Entry)
	{
		FFortItemList* Inventory;
		if (auto Bot = PC->Cast<AFortAthenaAIBotController>())
		{
			Inventory = &Bot->Inventory->Inventory;
		}
		else
		{
			Inventory = &PC->WorldInventory->Inventory;
		}
		if (!PC)
			return;
		auto ent = Inventory->ItemInstances.Search([Entry](UFortWorldItem* item)
			{ return item->ItemEntry.ItemGuid == Entry.ItemGuid; });
		if (ent)
			(*ent)->ItemEntry = Entry;

		TriggerInventoryUpdate(PC, &Entry);
	}

	inline static void AddMore(AFortPlayerControllerAthena* PC, FFortItemEntry& Entry, int Count, bool ShowPickupNoti = true)
	{
		if (Entry.ItemDefinition->IsA<UFortAmmoItemDefinition>() || Entry.ItemDefinition->IsA<UFortResourceItemDefinition>())
		{
			auto State = Entry.StateValues.Search([](FFortItemEntryStateValue& Value)
				{ return Value.StateType == EFortItemEntryState::ShouldShowItemToast; });
			if (!State) {
				FFortItemEntryStateValue Value{};
				Value.StateType = EFortItemEntryState::ShouldShowItemToast;
				Value.IntValue = ShowPickupNoti;
				Entry.StateValues.Add(Value);
			}
			else State->IntValue = ShowPickupNoti;
		}
		auto MaxStack = (int32)Entry.ItemDefinition->MaxStackSize.Value;
		Entry.Count += Count;
		if (Entry.Count > MaxStack)
		{
			SpawnPickup(PC->Pawn->K2_GetActorLocation(), Entry.ItemDefinition, Entry.Count - MaxStack, 0, EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource::Unset, PC->MyFortPawn);
			Entry.Count = MaxStack;
		}
		ReplaceEntry(PC, Entry);
	}

	inline void Remove(AFortPlayerController* PlayerController, FGuid Guid)
	{
		if (!PlayerController)
			return;

		auto ItemEntryIdx = PlayerController->WorldInventory->Inventory.ReplicatedEntries.SearchIndex([&](FFortItemEntry& entry) { return entry.ItemGuid == Guid; });
		if (ItemEntryIdx != -1)
			PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(ItemEntryIdx);

		auto ItemInstanceIdx = PlayerController->WorldInventory->Inventory.ItemInstances.SearchIndex([&](UFortWorldItem* entry) { return entry->ItemEntry.ItemGuid == Guid; });
		auto ItemInstance = PlayerController->WorldInventory->Inventory.ItemInstances.Search([&](UFortWorldItem* entry)
			{ return entry->ItemEntry.ItemGuid == Guid; });

		auto Instance = ItemInstance ? *ItemInstance : nullptr;
		if (ItemInstanceIdx != -1)
			PlayerController->WorldInventory->Inventory.ItemInstances.Remove(ItemInstanceIdx);

		TriggerInventoryUpdate(PlayerController, nullptr);
	}

	inline void ServerAttemptInventoryDrop(UObject* Context, FFrame& Stack)
	{
		FGuid Guid;
		int32 Count;
		bool bTrash;
		Stack.StepCompiledIn(&Guid);
		Stack.StepCompiledIn(&Count);
		Stack.StepCompiledIn(&bTrash);
		Stack.IncrementCode();
		auto PlayerController = (AFortPlayerControllerAthena*)Context;

		if (!PlayerController || !PlayerController->Pawn)
			return;
		auto ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry)
			{ return entry.ItemGuid == Guid; });
		if (!ItemEntry || (ItemEntry->Count - Count) < 0)
			return;
		ItemEntry->Count -= Count;
		Inventory::SpawnPickup(PlayerController->Pawn->K2_GetActorLocation() + PlayerController->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *ItemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn, Count);
		if (ItemEntry->Count == 0)
			Inventory::Remove(PlayerController, Guid);
		else
			Inventory::ReplaceEntry(PlayerController, *ItemEntry);
	}

	inline void (*OnReloadOG)(AFortWeapon* a1, int RemoveCount);
	inline void OnReload(AFortWeapon* a1, int RemoveCount)
	{
		if (!a1) return OnReloadOG(a1, RemoveCount);

		AFortPlayerPawn* Pawn = (AFortPlayerPawn*)a1->GetOwner();
		if (!Pawn || !Pawn->Controller) return OnReloadOG(a1, RemoveCount);

		AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PC) return OnReloadOG(a1, RemoveCount);
		AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)Pawn->PlayerState;
		if (!PlayerState || PlayerState->bIsABot) return OnReloadOG(a1, RemoveCount);

		FFortItemEntry* WeaponItemEntry = Inventory::FindItemEntry(PC, a1->ItemEntryGuid);
		if (!WeaponItemEntry || !WeaponItemEntry->ItemDefinition) return OnReloadOG(a1, RemoveCount);

		UFortWorldItemDefinition* AmmoItemDef = a1->WeaponData ? a1->WeaponData->GetAmmoWorldItemDefinition_BP() : nullptr;
		if (!AmmoItemDef) return OnReloadOG(a1, RemoveCount);

		FFortItemEntry* AmmoItemEntry = Inventory::FindItemEntry(PC, AmmoItemDef);
		if (AmmoItemEntry)
		{
			Inventory::RemoveItem(PC, AmmoItemEntry->ItemDefinition, RemoveCount);
		}
		else
		{
			int MaxStackSize = WeaponItemEntry->ItemDefinition->MaxStackSize.Value;
			if (MaxStackSize > 1) Inventory::RemoveItem(PC, WeaponItemEntry->ItemDefinition, RemoveCount);
		}

		WeaponItemEntry->LoadedAmmo = a1->AmmoCount;

		Inventory::UpdateInventory(PC, *WeaponItemEntry);

		return OnReloadOG(a1, RemoveCount);
	}

	inline void RemoveAllItems(AFortPlayerController* PlayerController)
	{
		if (!PlayerController || !PlayerController->WorldInventory)
			return;

		auto& Inventory = PlayerController->WorldInventory->Inventory;

		TArray<FFortItemEntry> EntriesToRemove = Inventory.ReplicatedEntries;

		Inventory.ReplicatedEntries.Free();
		Inventory.ItemInstances.Free();

		TriggerInventoryUpdate(PlayerController, nullptr);
	}

	inline void InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry PickupEntry)
	{
		auto MaxStack = (int32)Utils::EvaluateScalableFloat(PickupEntry.ItemDefinition->MaxStackSize);
		int ItemCount = 0;
		for (auto& Item : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
		{
			if (Inventory::GetQuickbar(Item.ItemDefinition) == EFortQuickBars::Primary)
				ItemCount += ((UFortWorldItemDefinition*)Item.ItemDefinition)->NumberOfSlotsToTake;
		}
		auto GiveOrSwap = [&]() {
			if (ItemCount == 5 && Inventory::GetQuickbar(PickupEntry.ItemDefinition) == EFortQuickBars::Primary) {
				if (Inventory::GetQuickbar(PlayerController->MyFortPawn->CurrentWeapon->WeaponData) == EFortQuickBars::Primary) {
					auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search1([PlayerController](FFortItemEntry& entry)
						{ return entry.ItemGuid == PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid; });
					Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *itemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
					Inventory::Remove(PlayerController, PlayerController->MyFortPawn->CurrentWeapon->ItemEntryGuid);
					Inventory::GiveItem(PlayerController, PickupEntry.ItemDefinition, PickupEntry.Count, true);
				}
				else {
					Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
				}
			}
			else
				Inventory::GiveItem(PlayerController, PickupEntry.ItemDefinition, PickupEntry.Count, true);
			};
		auto GiveOrSwapStack = [&](int32 OriginalCount) {
			if (PickupEntry.ItemDefinition->bAllowMultipleStacks && ItemCount < 5)
				Inventory::GiveItem(PlayerController, PickupEntry.ItemDefinition, OriginalCount - MaxStack, true);
			else
				Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), (FFortItemEntry&)PickupEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn, OriginalCount - MaxStack);
			};
		if (PickupEntry.ItemDefinition->IsStackable()) {
			auto itemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Search([PickupEntry, MaxStack](FFortItemEntry& entry)
				{ return entry.ItemDefinition == PickupEntry.ItemDefinition && entry.Count < MaxStack; });
			if (itemEntry) {
				auto State = itemEntry->StateValues.Search([](FFortItemEntryStateValue& Value)
					{ return Value.StateType == EFortItemEntryState::ShouldShowItemToast; });
				if (!State) {
					FFortItemEntryStateValue Value{};
					Value.StateType = EFortItemEntryState::ShouldShowItemToast;
					Value.IntValue = true;
					itemEntry->StateValues.Add(Value);
				}
				else State->IntValue = true;

				if ((itemEntry->Count += PickupEntry.Count) > MaxStack) {
					auto OriginalCount = itemEntry->Count;
					itemEntry->Count = MaxStack;

					GiveOrSwapStack(OriginalCount);
				}
				Inventory::ReplaceEntry(PlayerController, *itemEntry);
			}
			else {
				if (PickupEntry.Count > MaxStack) {
					auto OriginalCount = PickupEntry.Count;
					PickupEntry.Count = MaxStack;

					GiveOrSwapStack(OriginalCount);
				}
				GiveOrSwap();
			}
		}
		else {
			GiveOrSwap();
		}
	}

	inline bool (*CompletePickupAnimationOG)(AFortPickup* Pickup);
	inline bool CompletePickupAnimation(AFortPickup* Pickup) {
		auto Pawn = (AFortPlayerPawnAthena*)Pickup->PickupLocationData.PickupTarget;
		if (!Pawn)
			return CompletePickupAnimationOG(Pickup);
		auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
		if (!PlayerController)
			return CompletePickupAnimationOG(Pickup);
		if (auto entry = (FFortItemEntry*)PlayerController->SwappingItemDefinition)
		{
			Inventory::SpawnPickup(PlayerController->GetViewTarget()->K2_GetActorLocation(), *entry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PlayerController->MyFortPawn);
			// SwapEntry(PC, *entry, Pickup->PrimaryPickupItemEntry);
			Inventory::Remove(PlayerController, entry->ItemGuid);
			Inventory::GiveItem(PlayerController, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count, -1);
			PlayerController->SwappingItemDefinition = nullptr;
		}
		else
		{
			InternalPickup(PlayerController, Pickup->PrimaryPickupItemEntry);
		}
		return CompletePickupAnimationOG(Pickup);
	}

	inline void Hook()
	{
		Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x231, ServerExecuteInventoryItem, nullptr);

		//Utils::HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x22B, ServerHandlePickup, (LPVOID*)&ServerHandlePickupOG);

		Utils::ExecHook(L"/Script/FortniteGame.FortPlayerController.ServerAttemptInventoryDrop", ServerAttemptInventoryDrop);

		Utils::HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x22B, ServerHandlePickup, (LPVOID*)&ServerHandlePickupOG);

		//Utils::Hook(Jeremy::ImageBase + 0x234E47C, CompletePickupAnimation, CompletePickupAnimationOG);

		Utils::HookVTable(AFortPlayerPawnAthena::GetDefaultObj(), 0x12F, NetMulticast_Athena_BatchedDamageCues, (LPVOID*)&NetMulticast_Athena_BatchedDamageCuesOG);
	}
}