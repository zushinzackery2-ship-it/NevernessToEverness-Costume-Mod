#pragma once

namespace CurrentFashionSetter
{
    void RunFashionSwapOnGameThread(int direction);
    void ApplyFashionById(const SDK::FName& targetFashionId);
}
