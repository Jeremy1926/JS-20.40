#pragma once
#include "framework.h"

namespace Tick {
	void (*ServerReplicateActors)(void*) = decltype(ServerReplicateActors)(Jeremy::ImageBase + 0x0);

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
		AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;

		ServerReplicateActors(Driver->ReplicationDriver);

		return TickFlushOG(Driver, DeltaTime);
	}


	inline float GetMaxTickRate()
	{
		return 120.f;
	}

	void Hook()
	{
		MH_CreateHook((LPVOID)(Jeremy::ImageBase + Jeremy::Offsets::GetMaxTickRate), GetMaxTickRate, nullptr);
		MH_CreateHook((LPVOID)(Jeremy::ImageBase + Jeremy::Offsets::TickFlush), TickFlush, (LPVOID*)&TickFlushOG);
	}
}