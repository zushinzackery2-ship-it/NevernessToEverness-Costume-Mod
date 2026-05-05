#pragma once

#include "AppearanceData.h"

namespace CurrentFashionSetter
{
    bool CallProcessEvent(SDK::UObject* object, SDK::UFunction* function, void* params, std::ofstream& log);
    bool IsPlayerAppearanceObject(const SDK::UObject* object);
    SDK::UHTPlayerAppearance* AsPlayerAppearance(SDK::UObject* object, std::ofstream& log);

    template<typename SoftType>
    SDK::TSoftObjectPtr<SDK::UObject> CopySoftObjectPtr(const SoftType& source)
    {
        SDK::TSoftObjectPtr<SDK::UObject> result = {};
        static_assert(sizeof(result) == sizeof(source));
        memcpy(&result, &source, sizeof(result));
        return result;
    }

    template<typename SoftType>
    std::string SoftObjectPathToString(const SoftType& source)
    {
        return source.ObjectID.AssetPath.PackageName.ToString()
            + "."
            + source.ObjectID.AssetPath.AssetName.ToString();
    }
}
