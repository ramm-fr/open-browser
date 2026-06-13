#pragma once

namespace open_browser::ui::theme {

// Font family (system fonts with fallbacks)
constexpr const char *FONT_FAMILY_DEFAULT =
    "Inter, -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif";
constexpr const char *FONT_FAMILY_MONOSPACE =
    "'JetBrains Mono', 'Fira Code', 'Consolas', monospace";

// Font sizes (pixels)
constexpr int FONT_SIZE_XS = 11;
constexpr int FONT_SIZE_SM = 12;
constexpr int FONT_SIZE_MD = 13;
constexpr int FONT_SIZE_LG = 14;
constexpr int FONT_SIZE_XL = 16;
constexpr int FONT_SIZE_XXL = 18;

// Font weights
constexpr const char *FONT_WEIGHT_LIGHT = "300";
constexpr const char *FONT_WEIGHT_NORMAL = "400";
constexpr const char *FONT_WEIGHT_MEDIUM = "500";
constexpr const char *FONT_WEIGHT_SEMIBOLD = "600";
constexpr const char *FONT_WEIGHT_BOLD = "700";

// Line heights (relative to font size)
constexpr float LINE_HEIGHT_TIGHT = 1.2f;
constexpr float LINE_HEIGHT_NORMAL = 1.4f;
constexpr float LINE_HEIGHT_RELAXED = 1.6f;

// Letter spacing (pixels)
constexpr int LETTER_SPACING_TIGHT = -0.5;
constexpr int LETTER_SPACING_NORMAL = 0;
constexpr int LETTER_SPACING_WIDE = 0.5;

// Component-specific typography
constexpr int FONT_SIZE_ADDRESS_BAR = FONT_SIZE_LG;
constexpr int FONT_SIZE_TAB = FONT_SIZE_MD;
constexpr int FONT_SIZE_BUTTON = FONT_SIZE_SM;
constexpr int FONT_SIZE_WINDOW_TITLE = FONT_SIZE_LG;
constexpr const char *FONT_WEIGHT_TAB_ACTIVE = FONT_WEIGHT_MEDIUM;
constexpr const char *FONT_WEIGHT_WINDOW_TITLE = FONT_WEIGHT_SEMIBOLD;

} // namespace open_browser::ui::theme
