#pragma once
#include "framework.h"

using namespace std;

namespace Misc
{
    void NullFunc() {}

    int True()
    {
        return 1;
    }

    int False()
    {
        return 0;
    }

    static inline void (*KickPlayerOG)(AGameSession*, AController*);
    static void KickPlayer(AGameSession*, AController*)
    {
        return;
    }

    void (*DispatchRequestOG)(__int64 a1, unsigned __int64* a2, int a3);
    void DispatchRequest(__int64 a1, unsigned __int64* a2, int a3)
    {
        return DispatchRequestOG(a1, a2, 3);
    }

    void Hook()
    {
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x65F9A50), KickPlayer, (LPVOID*)&KickPlayerOG);
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0xF54B24), DispatchRequest, (LPVOID*)&DispatchRequestOG);

        Utils::Patch<uint8_t>(Jeremy::ImageBase + 0x605F9CE, 0xEB); //thanks ploosh

        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x66032DC), True, nullptr);

        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x45e0cc0), NullFunc, nullptr);
        MH_CreateHook((LPVOID)(Jeremy::ImageBase + 0x65f9a50), NullFunc, nullptr);
    }
}