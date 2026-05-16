#include "Pch.h"

#include "SdkRuntimeHelpers.h"

#include "Log.h"

namespace CurrentFashionSetter
{
    std::string Hex32(DWORD value)
    {
        char buffer[16] = {};
        sprintf_s(buffer, "0x%08X", value);
        return buffer;
    }

    bool IsPlayerAppearanceObject(const SDK::UObject* object)
    {
        if (!IsValidObject(object))
        {
            return false;
        }

        const SDK::UClass* klass = object->Class;
        for (int depth = 0; IsValidObject(klass) && depth < 32; ++depth)
        {
            if (klass->GetName() == "HTPlayerAppearance")
            {
                return true;
            }

            const SDK::UStruct* klassStruct = reinterpret_cast<const SDK::UStruct*>(klass);
            klass = reinterpret_cast<const SDK::UClass*>(klassStruct->Super);
        }

        return false;
    }

    SDK::UHTPlayerAppearance* AsPlayerAppearance(SDK::UObject* object, std::ofstream& log)
    {
        if (!IsPlayerAppearanceObject(object))
        {
            LogLine(log, "object is not UHTPlayerAppearance object=" + ObjectName(object));
            return nullptr;
        }

        return reinterpret_cast<SDK::UHTPlayerAppearance*>(object);
    }
}
