#pragma once

namespace AntiFadeMod
{
struct FPatchStats
{
    int CameraManagers = 0;
    int CameraSettings = 0;
    int IndoorMapSettings = 0;
    int AimingMapSettings = 0;
    int PlayerPitchCurvesCleared = 0;
    int ActorFadeCurvesCleared = 0;
    int CollisionDurationsCleared = 0;
    int CollisionParamEntriesChanged = 0;
    int UnableFadeClassesAdded = 0;
    int UnableFadeClassesAlreadyPresent = 0;
    int UnableFadeAddNoSlack = 0;
    int CachedDistanceValuesCleared = 0;
    int FunctionLookupFailures = 0;
    int OpacityRestoreCalls = 0;
    int OpacityQueryCalls = 0;
    float OpacityBefore = -1.0f;
    float OpacityAfter = -1.0f;
};

void ConfigureDisabledPlayerFade(SDK::FCameraSettings& Settings);
bool IsCameraManagerAntiFadeIntact(SDK::AHTPlayerCameraManager* CameraManager);
FPatchStats PatchCurrentCameraManager(SDK::AHTPlayerCameraManager* CurrentCameraManager);
void RestoreOpacityOnce(SDK::AHTAbilityCharacter* Character, FPatchStats& Stats);
void PrintStats(SDK::AHTPlayerCameraManager* CameraManager, SDK::AHTAbilityCharacter* Character, const FPatchStats& Stats);
}

