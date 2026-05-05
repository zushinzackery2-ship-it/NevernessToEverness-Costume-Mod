#include <cstddef>

#include "SDK/UMG_classes.hpp"
#include "SDK/Slate_classes.hpp"
#include "SDK/SlateCore_structs.hpp"

static_assert(alignof(SDK::FSlateBrush) == 0x10);
static_assert(sizeof(SDK::FSlateBrush) == 0xB0);
static_assert(alignof(SDK::FTextBlockStyle) == 0x10);
static_assert(sizeof(SDK::FTextBlockStyle) == 0x2E0);
static_assert(alignof(SDK::FEditableTextBoxStyle) == 0x10);
static_assert(sizeof(SDK::FEditableTextBoxStyle) == 0xC80);
static_assert(offsetof(SDK::UEditableTextBox, WidgetStyle) == 0x190);
static_assert(offsetof(SDK::UTextBlock, StrikeBrush) == 0x230);
static_assert(alignof(SDK::UEditableTextBox) == 0x10);
static_assert(sizeof(SDK::UEditableTextBox) == 0xE80);
static_assert(alignof(SDK::UTextBlock) == 0x10);
static_assert(sizeof(SDK::UTextBlock) == 0x330);
static_assert(alignof(SDK::UEditableTextBoxWidgetStyle) == 0x10);
static_assert(sizeof(SDK::UEditableTextBoxWidgetStyle) == 0xCB0);
static_assert(alignof(SDK::UTextBlockWidgetStyle) == 0x10);
static_assert(sizeof(SDK::UTextBlockWidgetStyle) == 0x310);
