#pragma once
#include "framework.h"

using namespace std;

namespace Misc
{
    inline void NullFunc() {}

    inline int True()
    {
        return 1;
    }

    inline int False()
    {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*)
    {
        return;
    }

    inline void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    inline void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    inline void Hook()
    {
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x19686f0), KickPlayer, (LPVOID*)&KickPlayerOG);
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x16bbfe0), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        //Utils::Patch<uint8_t>(Jeremy::ImageBase + 0x605F9CE, 0xEB); //thanks ploosh

        //MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x66032DC), True, nullptr);

        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x4834b50), NullFunc, nullptr);
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x6d007f0), NullFunc, nullptr);
    }
}