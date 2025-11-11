#pragma once
#include "framework.h"
#include "Replication.h"

namespace Tick {

	inline void (*TickFlushOG)(UNetDriver*, float);
	void TickFlush(UNetDriver* Driver, float DeltaTime)
	{
		if (!Driver)
			return;

		Replication::ServerReplicateActors(Driver, DeltaTime);

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