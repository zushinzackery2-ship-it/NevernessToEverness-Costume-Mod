#include "Pch.h"
#include "CameraPatch.hpp"
#include "SdkHelpers.hpp"

namespace AntiFadeMod
{
namespace
{
constexpr float RestoredOpacityMask = 1.0f;

bool QueryOpacity(SDK::AHTAbilityCharacter* Character, float* OutValue, FPatchStats& Stats)
{
    if (Character == nullptr || OutValue == nullptr)
    {
        return false;
    }

    static SDK::UFunction* Function = nullptr;
    if (Function == nullptr)
    {
        Function = FindFunctionByName(Character->Class, "HTAbilityCharacter", "GetOpacityMaskVal");
    }

    if (Function == nullptr)
    {
        ++Stats.FunctionLookupFailures;
        return false;
    }

    SDK::Params::HTAbilityCharacter_GetOpacityMaskVal Params{};
    if (!CallProcessEvent(Character, Function, &Params))
    {
        ++Stats.FunctionLookupFailures;
        return false;
    }

    *OutValue = Params.ReturnValue;
    ++Stats.OpacityQueryCalls;
    return true;
}
}

bool ApplySelfSettingCamera(SDK::AHTPlayerCameraManager* CameraManager, FPatchStats& Stats)
{
    if (CameraManager == nullptr)
    {
        return false;
    }

    static SDK::UFunction* Function = nullptr;
    if (Function == nullptr)
    {
        Function = FindFunctionByName(CameraManager->Class, "HTPlayerCameraManager", "SetSelfSettingCamera");
    }

    if (Function == nullptr)
    {
        ++Stats.FunctionLookupFailures;
        return false;
    }

    SDK::Params::HTPlayerCameraManager_SetSelfSettingCamera Params{};
    Params.bUse = true;
    ConfigureDisabledPlayerFade(Params.CameraSetting);

    if (!CallProcessEvent(CameraManager, Function, &Params))
    {
        ++Stats.FunctionLookupFailures;
        return false;
    }

    ++Stats.SelfSettingCalls;
    return true;
}

void RestoreOpacityOnce(SDK::AHTAbilityCharacter* Character, FPatchStats& Stats)
{
    if (Character == nullptr)
    {
        return;
    }

    QueryOpacity(Character, &Stats.OpacityBefore, Stats);

    static SDK::UFunction* Function = nullptr;
    if (Function == nullptr)
    {
        Function = FindFunctionByName(Character->Class, "HTAbilityCharacter", "UpdateOpacityMaskVal");
    }

    if (Function == nullptr)
    {
        ++Stats.FunctionLookupFailures;
        return;
    }

    SDK::Params::HTAbilityCharacter_UpdateOpacityMaskVal Params{};
    Params.fValue = RestoredOpacityMask;

    if (CallProcessEvent(Character, Function, &Params))
    {
        ++Stats.OpacityRestoreCalls;
    }
    else
    {
        ++Stats.FunctionLookupFailures;
    }

    QueryOpacity(Character, &Stats.OpacityAfter, Stats);
}

void PrintStats(SDK::AHTPlayerCameraManager* CameraManager, SDK::AHTAbilityCharacter* Character, const FPatchStats& Stats)
{
    std::printf("[AntiFadeMod] patched cam=%p character=%p\n", static_cast<void*>(CameraManager), static_cast<void*>(Character));
    std::printf("[AntiFadeMod] source managers=%d settings=%d indoor=%d aiming=%d\n",
        Stats.CameraManagers,
        Stats.CameraSettings,
        Stats.IndoorMapSettings,
        Stats.AimingMapSettings);
    std::printf("[AntiFadeMod] curves pitch=%d actor=%d collisionDuration=%d collisionEntries=%d\n",
        Stats.PlayerPitchCurvesCleared,
        Stats.ActorFadeCurvesCleared,
        Stats.CollisionDurationsCleared,
        Stats.CollisionParamEntriesChanged);
    std::printf("[AntiFadeMod] unableFade added=%d present=%d noSlack=%d selfSettingCalls=%d\n",
        Stats.UnableFadeClassesAdded,
        Stats.UnableFadeClassesAlreadyPresent,
        Stats.UnableFadeAddNoSlack,
        Stats.SelfSettingCalls);
    std::printf("[AntiFadeMod] opacity restore=%d queries=%d before=%.3f after=%.3f lookupFailures=%d\n",
        Stats.OpacityRestoreCalls,
        Stats.OpacityQueryCalls,
        Stats.OpacityBefore,
        Stats.OpacityAfter,
        Stats.FunctionLookupFailures);
}
}

