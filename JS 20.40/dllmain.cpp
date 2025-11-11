// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Offsets.h"
#include "GameMode.h"
#include "PC.h"
#include "Misc.h"
#include "Net.h"
#include "Tick.h"
#include "Vehicles.h"
#include "Looting.h"

void LoadWorld()
{
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Artemis_Terrain", nullptr);
}

void Main()
{
    AllocConsole();
    FILE* fptr;
    freopen_s(&fptr, "CONOUT$", "w+", stdout);

    MH_Initialize();
    SetConsoleTitleA("JS 20.40 | Setting up");
    Sleep(5000);

    GameMode::Hook();
    PC::Hook();
    Abilities::Hook();
    Inventory::Hook();
    Looting::Hook();
    Vehicles::Hook();
    Misc::Hook();
    Net::Hook();
    Tick::Hook();

    Sleep(5000);

    MH_EnableHook(MH_ALL_HOOKS);
    srand((uint32_t)time(0));

    for (auto& RetTrueFunc : Jeremy::Offsets::RetTrueFuncs)
    {
        if (RetTrueFunc == 0)
            continue;

        Utils::Patch<uint32_t>(Jeremy::ImageBase + RetTrueFunc, 0xc0ffc031);
        Utils::Patch<uint8_t>(Jeremy::ImageBase + RetTrueFunc + 4, 0xc3);
    }

    *(bool*)(InSDKUtils::GetImageBase() + Jeremy::Offsets::GIsClient) = false; //GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + Jeremy::Offsets::GIsServer) = true; //GIsServer

    Sleep(1000);
    LoadWorld();

    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Artemis_Terrain", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogFortUIDirector None", nullptr);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::thread(Main).detach();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

