#include "framework.h"

namespace Vehicles
{
	template<typename T>
	T* SpawnActor(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		FTransform Transform{};
		Transform.Scale3D = FVector(1, 1, 1);
		Transform.Translation = Loc;

		return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
	}

	template<typename T>
	T* SpawnActor(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		FTransform Transform{};
		Transform.Scale3D = FVector(1, 1, 1);
		Transform.Translation = Loc;
		Transform.Rotation = Utils::FRotToQuat(Rot);

		return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
	}

	void SpawnVehicles()
	{
		TArray<AActor*> Spawners;
		UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaVehicleSpawner::StaticClass(), &Spawners);
		static bool spawnsharkchoppa = false;

		for (size_t i = 0; i < Spawners.Num(); i++)
		{
			AFortAthenaVehicleSpawner* Spawner = (AFortAthenaVehicleSpawner*)Spawners[i];
			SpawnActor<AFortAthenaVehicle>(Spawner->GetVehicleClass(), Spawner->K2_GetActorLocation());
			auto Name = Spawner->GetName();

			if (Name.starts_with("Apollo_Hoagie_Spawner") && !spawnsharkchoppa)
			{
				SpawnActor<AFortAthenaVehicle>(Spawner->GetVehicleClass(), {
					113665,
					-91120,
					-2985 - 80
					});
				spawnsharkchoppa = true;
			}
		}

		Spawners.Free();
	}

	void ServerMove(AFortPhysicsPawn* Pawn, FReplicatedPhysicsPawnState InState)
	{
		UPrimitiveComponent* Mesh = (UPrimitiveComponent*)Pawn->RootComponent;

		InState.Rotation.X -= 2.5;
		InState.Rotation.Y /= 0.3;
		InState.Rotation.Z -= -2.0;
		InState.Rotation.W /= -1.2;

		FTransform Transform{};
		Transform.Translation = InState.Translation;
		Transform.Rotation = InState.Rotation;
		Transform.Scale3D = FVector{ 1, 1, 1 };

		Mesh->K2_SetWorldTransform(Transform, false, nullptr, true);
		Mesh->bComponentToWorldUpdated = true;
		Mesh->SetPhysicsLinearVelocity(InState.LinearVelocity, 0, FName());
		Mesh->SetPhysicsAngularVelocityInDegrees(InState.AngularVelocity, 0, FName(0));
	}

	void Hook()
	{
		Utils::ExecHook(L"/Script/FortniteGame.FortPhysicsPawn.ServerMove", ServerMove);
		Utils::ExecHook(L"/Script/FortniteGame.FortPhysicsPawn.ServerMoveReliable", ServerMove);
	}
}