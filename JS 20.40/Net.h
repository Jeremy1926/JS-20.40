#pragma once
#include "framework.h"

namespace Net
{
	int GetNetMode()
	{
		return 1;
	}

	void Hook()
	{
		MH_CreateHook((LPVOID)(Jeremy::ImageBase + Jeremy::Offsets::GetNetMode), GetNetMode, nullptr);
	}
}