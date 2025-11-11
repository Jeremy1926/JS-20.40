#pragma once

struct Options
{
public:

    static inline auto bLateGame = false;
    static inline auto bInfiniteLateGame = false;
    static inline auto bForceRespawns = false;
    static inline auto bInfiniteMats = false;
    static inline auto bInfiniteAmmo = false;
    static inline auto iMinPlayersForEarlyStart = 1;
    static inline auto iWarmupTime = 30.f;
    static inline auto iMaxTickRate = 30.f;
};