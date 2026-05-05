#pragma once

#include "SDK/HTGame_classes.hpp"

namespace CurrentFashionSetter
{
    struct FashionCandidate
    {
        std::string Name;
        SDK::FName RowName;
        const SDK::FStaticAppearanceData* StaticData = nullptr;
        const SDK::FFashionData* FashionData = nullptr;
        const SDK::UObject* ScriptStruct = nullptr;
        bool IsDefault = false;
        SDK::TSoftObjectPtr<SDK::UHTPlayerAppearance> DefaultPlayerAppearanceAsset = {};
    };

    bool IsValidObject(const SDK::UObject* object);
    std::string ObjectName(const SDK::UObject* object);
    SDK::AHTPlayerCharacter* GetCurrentPlayerCharacter(std::ofstream& log);
    SDK::UDataTable* FindAppearanceDataTable(const SDK::FName& characterId, std::ofstream& log);
    bool SelectFashionByDirection(SDK::UDataTable* table, const SDK::FName& characterId,
        const SDK::FName& currentFashionId, int direction, FashionCandidate& target, std::ofstream& log);
    bool SelectFashionByDirection(SDK::UDataTable* table, const SDK::FName& characterId,
        const SDK::FName& currentFashionId, const FashionCandidate* defaultCandidate,
        int direction, FashionCandidate& target, std::ofstream& log);
}
