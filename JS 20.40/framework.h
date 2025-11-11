#pragma once
#include "pch.h"
#include "Offsets.h"

static inline void* _NpFH = nullptr;

inline std::map<AFortPickup*, float> PickupLifetimes;
static inline TArray<UFortAbilitySet*> AbilitySets;

namespace Utils
{
	template<typename T>
	inline T* GetDefaultObject()
	{
		return (T*)T::StaticClass()->DefaultObject;
	}

	template <typename _Is>
	static __forceinline void Patch(uintptr_t ptr, _Is byte)
	{
		DWORD og;
		VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
		*(_Is*)ptr = byte;
		VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
	}

	inline void HookVTable(void* Base, int Idx, void* Detour, void** OG)
	{
		DWORD oldProtection;

		void** VTable = *(void***)Base;

		if (OG)
		{
			*OG = VTable[Idx];
		}

		VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

		VTable[Idx] = Detour;

		VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
	}

	template<typename T>
	inline T* Cast(UObject* Object)
	{
		if (!Object || !Object->IsA(T::StaticClass()))
			return nullptr;
		return (T*)Object;
	}

	inline __forceinline static UObject* InternalLoadObject(const wchar_t* ObjectPath, UClass* InClass, UObject* Outer = nullptr)
	{
		return Jeremy::Funcs::StaticLoadObject(InClass, Outer, ObjectPath, nullptr, 0, nullptr, false);
	}
	inline __forceinline static UObject* InternalFindObject(const wchar_t* ObjectPath, UClass* Class)
	{
		return Jeremy::Funcs::StaticFindObject(Class, nullptr, ObjectPath, false);
	}

	inline UObject* FindObject(const wchar_t* ObjectPath, UClass* Class)
	{
		auto Object = InternalFindObject(ObjectPath, Class);
		return Object ? Object : InternalLoadObject(ObjectPath, Class);
	}

	template <typename _Ot>
	inline static _Ot* FindObject(const wchar_t* ObjectPath, UClass* Class = _Ot::StaticClass())
	{
		return (_Ot*)FindObject(ObjectPath, Class);
	}

	__forceinline static void _HookVT(void** _Vt, uint32_t _Ind, void* _Detour)
	{
		DWORD _Vo;
		VirtualProtect(_Vt + _Ind, 8, PAGE_EXECUTE_READWRITE, &_Vo);
		_Vt[_Ind] = _Detour;
		VirtualProtect(_Vt + _Ind, 8, _Vo, &_Vo);
	}

	template <typename _Ct>
	__forceinline static void HookEvery(uint32_t _Ind, void* _Detour)
	{
		for (int i = 0; i < UObject::GObjects->Num(); i++) {
			auto Obj = UObject::GObjects->GetByIndex(i);
			if (Obj && Obj->IsA<_Ct>()) {
				_HookVT(Obj->VTable, _Ind, _Detour);
			}
		}
	}

	template <typename _Ot>
	inline static _Ot* FindObject(std::wstring ObjectPath, UClass* Class = _Ot::StaticClass())
	{
		return (_Ot*)FindObject(ObjectPath.c_str(), Class);
	}

	template <typename _Ot = void*>
	__forceinline static void ExecHook(UFunction* _Fn, void* _Detour, _Ot& _Orig = _NpFH)
	{
		if (!_Fn)
			return;
		if (!std::is_same_v<_Ot, void*>)
			_Orig = (_Ot)_Fn->ExecFunction;

		_Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
	}

	template <typename _Ot = void*>
	__forceinline static void ExecHook(const wchar_t* _Name, void* _Detour, _Ot& _Orig = _NpFH)
	{
		UFunction* _Fn = FindObject<UFunction>(_Name);
		ExecHook(_Fn, _Detour, _Orig);
	}

	template <class _Ot = void*>
	static void Hook(uint64_t _Ptr, void* _Detour, _Ot& _Orig = _NpFH) {
		MH_CreateHook((LPVOID)_Ptr, _Detour, (LPVOID*)(std::is_same_v<_Ot, void*> ? nullptr : &_Orig));
	}

	inline TArray<AActor*> GetAll(UClass* Class)
	{
		TArray<AActor*> ret;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), Class, &ret);
		return ret;
	}

	template <typename _At = AActor>
	__forceinline static TArray<_At*> GetAll(UClass* Class)
	{
		TArray<_At*> ret;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), Class, (TArray<AActor*>*) & ret);
		return ret;
	}

	template <typename _At = AActor>
	__forceinline static TArray<_At*> GetAll()
	{
		return GetAll<_At>(_At::StaticClass());
	}

	inline float EvaluateScalableFloat(FScalableFloat& Float)
	{
		if (!Float.Curve.CurveTable)
			return Float.Value;

		float Out;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Float.Curve.CurveTable, Float.Curve.RowName, (float)0, nullptr, &Out, FString());
		return Out;
	}

	template <typename _It>
	static _It* GetInterface(UObject* Object)
	{
		return ((_It * (*)(UObject*, UClass*)) (Jeremy::ImageBase + 0xD80568))(Object, _It::StaticClass());
	}

	enum ESpawnActorNameMode : uint8
	{
		Required_Fatal,
		Required_ErrorAndReturnNull,
		Required_ReturnNull,
		Requested
	};

	struct FActorSpawnParameters
	{
	public:
		FName Name;

		AActor* Template;
		AActor* Owner;
		APawn* Instigator;
		ULevel* OverrideLevel;
		UChildActorComponent* OverrideParentComponent;
		ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

	private:
		uint8	bRemoteOwned : 1;
	public:
		uint8	bNoFail : 1;
		uint8	bDeferConstruction : 1;
		uint8	bAllowDuringConstructionScript : 1;
		ESpawnActorNameMode NameMode;
		EObjectFlags ObjectFlags;
	};

	template <typename T = AActor>
	inline static T* SpawnActorUnfinished(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		FTransform Transform(Loc, Rot);

		return (T*)UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AlwaysSpawn, Owner);
	}

	template <typename T = AActor>
	inline static T* FinishSpawnActor(AActor* Actor, FVector Loc, FRotator Rot)
	{
		FTransform Transform(Loc, Rot);

		return (T*)UGameplayStatics::FinishSpawningActor(Actor, Transform);
	};

	inline AActor* SpawnActor(UClass* Class, FTransform& Transform, AActor* Owner)
	{
		FActorSpawnParameters addr{};

		addr.Owner = Owner;
		addr.bDeferConstruction = false;
		addr.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		addr.NameMode = ESpawnActorNameMode::Requested;

		return ((AActor * (*)(UWorld*, UClass*, FTransform const*, FActorSpawnParameters*))(Jeremy::ImageBase + 0x11846C4))(UWorld::GetWorld(), Class, &Transform, &addr);
	}

	inline AActor* SpawnActor(UClass* Class, FVector& Loc, FRotator& Rot, AActor* Owner)
	{
		FTransform Transform(Loc, Rot);
		return SpawnActor(Class, Transform, Owner);
	}

	template<typename T>
	inline T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		return SpawnActor<T>(Loc, Rot, Owner, Class);
	}

	inline AFortPickupAthena* SpawnStack(AFortPlayerPawnAthena* Pawn, UFortItemDefinition* Def, int Count, bool GiveAmmo = false, int Ammo = 0)
	{
		auto Statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		FVector Loc = Pawn->K2_GetActorLocation();
		AFortPickupAthena* Pickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
		Pickup->bReplicates = true;
		PickupLifetimes[Pickup] = Statics->GetTimeSeconds(UWorld::GetWorld());
		Pickup->PawnWhoDroppedPickup = Pawn;
		Pickup->PrimaryPickupItemEntry.Count = Count;
		Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;

		if (GiveAmmo)
		{
			Pickup->PrimaryPickupItemEntry.LoadedAmmo = Ammo;
		}
		Pickup->PrimaryPickupItemEntry.ReplicationKey++;

		Pickup->OnRep_PrimaryPickupItemEntry();
		Pickup->TossPickup(Loc, Pawn, 6, true, true, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);

		Pickup->MovementComponent = (UProjectileMovementComponent*)Statics->SpawnObject(UProjectileMovementComponent::StaticClass(), Pickup);
		Pickup->MovementComponent->bReplicates = true;
		((UProjectileMovementComponent*)Pickup->MovementComponent)->SetComponentTickEnabled(true);

		return Pickup;
	}

	inline AFortPickupAthena* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
	{
		auto SpawnedPickup = Actors<AFortPickupAthena>(AFortPickupAthena::StaticClass(), Loc);
		SpawnedPickup->bRandomRotation = true;

		auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
		PickupEntry.ItemDefinition = ItemDef;
		PickupEntry.Count = OverrideCount;
		PickupEntry.LoadedAmmo = LoadedAmmo;
		PickupEntry.ReplicationKey++;
		SpawnedPickup->OnRep_PrimaryPickupItemEntry();
		SpawnedPickup->PawnWhoDroppedPickup = Pawn;

		SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

		SpawnedPickup->SetReplicateMovement(true);
		SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)Utils::GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

		if (SourceType == EFortPickupSourceTypeFlag::Container)
		{
			SpawnedPickup->bTossedFromContainer = true;
			SpawnedPickup->OnRep_TossedFromContainer();
		}

		return SpawnedPickup;
	}

	static inline FQuat FRotToQuat(FRotator Rotation)
	{
		FQuat Quat;
		const float DEG_TO_RAD = 3.14159f / 180.0f;
		const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

		float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
		float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
		float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
		float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
		float SR = sin(Rotation.Roll * DIVIDE_BY_2);
		float CR = cos(Rotation.Roll * DIVIDE_BY_2);

		Quat.X = CR * SP * SY - SR * CP * CY;
		Quat.Y = -CR * SP * CY - SR * CP * SY;
		Quat.Z = CR * CP * SY - SR * SP * CY;
		Quat.W = CR * CP * CY + SR * SP * SY;

		return Quat;
	}

	template<typename T>
	inline T* SpawnActor(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform Transform{};
		Transform.Scale3D = FVector{ 1,1,1 };
		Transform.Translation = Loc;
		Transform.Rotation = Utils::FRotToQuat(Rot);

		return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
	}
}

#define DefHookOg(_Rt, _Name, ...) static inline _Rt (*_Name##OG)(##__VA_ARGS__); static _Rt _Name(##__VA_ARGS__); 
#define DefUHookOg(_Name) static inline void (*_Name##OG)(UObject*, FFrame&); static void _Name(UObject*, FFrame&); 
#define DefUHookOgRet(_Rt, _Name) static inline void (*_Name##OG)(UObject*, FFrame&, _Rt*); static void _Name(UObject *, FFrame&, _Rt*);
#define InitHooks static void Hook(); static int _AddHook() { _HookFuncs.push_back(Hook); return 0; }; static inline auto _HookAdder = _AddHook();
#define callOGWithRet(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th##OG; auto _Rt = _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th; return _Rt; })()

DefUHookOg(OnAircraftExitedDropZone);
DefUHookOg(OnAircraftEnteredDropZone);

#define callOG(_Tr, _Fn, _Th, ...) ([&](){ _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th##OG; _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th; })()

struct FOutParmRec
{
	UProperty* Property;
	uint8* PropAddr;
	FOutParmRec* NextOutParm;
};

inline FFortRangedWeaponStats* GetStats(UFortWeaponItemDefinition* Def)
{
	if (!Def || !Def->WeaponStatHandle.DataTable)
		return nullptr;

	auto Val = Def->WeaponStatHandle.DataTable->RowMap.Search([Def](FName& Key, uint8_t* Value) {
		return Def->WeaponStatHandle.RowName == Key && Value;
		});

	return Val ? *(FFortRangedWeaponStats**)Val : nullptr;
}

class FOutputDevice
{
public:
	bool bSuppressEventTag;
	bool bAutoEmitLineTerminator;
	uint8_t _Padding1[0x6];
};

class FFrame : public FOutputDevice
{
public:
	void** VTable;
	UFunction* Node;
	UObject* Object;
	uint8* Code;
	uint8* Locals;
	FProperty* MostRecentProperty;
	uint8_t* MostRecentPropertyAddress;
	uint8_t _Padding1[0x48];
	FField* PropertyChainForCompiledIn;

public:
	inline void StepCompiledIn(void* const Result, bool ForceExplicitProp = false);

	template <typename T>
	inline T& StepCompiledInRef() {
		T TempVal{};
		MostRecentPropertyAddress = nullptr;

		if (Code)
		{
			((void (*)(FFrame*, UObject*, void* const)) (Jeremy::ImageBase + Jeremy::Offsets::Step))(this, Object, &TempVal);
		}
		else
		{
			FField* _Prop = PropertyChainForCompiledIn;
			PropertyChainForCompiledIn = _Prop->Next;
			((void (*)(FFrame*, void* const, FField*)) (Jeremy::ImageBase + Jeremy::Offsets::StepExplicitProperty))(this, &TempVal, _Prop);
		}

		return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
	}

	inline void IncrementCode();
};


enum ESpawnActorNameMode : uint8
{
	Required_Fatal,
	Required_ErrorAndReturnNull,
	Required_ReturnNull,
	Requested
};

struct FActorSpawnParameters
{
public:
	FName Name;

	AActor* Template;
	AActor* Owner;
	APawn* Instigator;
	ULevel* OverrideLevel;
	UChildActorComponent* OverrideParentComponent;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

private:
	uint8	bRemoteOwned : 1;
public:
	uint8	bNoFail : 1;
	uint8	bDeferConstruction : 1;
	uint8	bAllowDuringConstructionScript : 1;
	ESpawnActorNameMode NameMode;
	EObjectFlags ObjectFlags;
};

inline void FFrame::StepCompiledIn(void* const Result, bool ForceExplicitProp)
{
	if (Code && !ForceExplicitProp)
	{
		((void (*)(FFrame*, UObject*, void* const)) (Jeremy::ImageBase + Jeremy::Offsets::Step))(this, Object, Result);
	}
	else
	{
		FField* _Prop = PropertyChainForCompiledIn;
		PropertyChainForCompiledIn = _Prop->Next;
		((void (*)(FFrame*, void* const, FField*)) (Jeremy::ImageBase + Jeremy::Offsets::StepExplicitProperty))(this, Result, _Prop);
	}
}


inline void FFrame::IncrementCode() {
	Code = (uint8_t*)(__int64(Code) + (bool)Code);
}

inline UObject* SDK::InternalGet(FSoftObjectPtr* Ptr, UClass* Class)
{
	if (!Ptr)
		return nullptr;

	auto Ret = Ptr->WeakPtr.ObjectIndex && Ptr->WeakPtr.ObjectSerialNumber ? Ptr->Get() : nullptr;
	if ((!Ret || !Ret->IsA(Class)) && Ptr->ObjectID.AssetPathName.ComparisonIndex > 0) {
		Ptr->WeakPtr = Ret = Utils::FindObject(Ptr->ObjectID.AssetPathName.GetRawWString().c_str(), Class);
	}
	return Ret;
}