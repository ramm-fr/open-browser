#pragma once

namespace open_browser::ui::theme {

// Spacing constants (pixels)
constexpr int SPACING_XS = 4;
constexpr int SPACING_SM = 6;
constexpr int SPACING_MD = 8;
constexpr int SPACING_LG = 12;
constexpr int SPACING_XL = 16;
constexpr int SPACING_XXL = 20;
constexpr int SPACING_XXXL = 24;

// Corner radius (pixels)
constexpr int RADIUS_SM = 4;
constexpr int RADIUS_MD = 8;
constexpr int RADIUS_LG = 12;
constexpr int RADIUS_XL = 16;
constexpr int RADIUS_XXL = 24;
constexpr int RADIUS_BUTTON = RADIUS_MD;
constexpr int RADIUS_ADDRESS_BAR = RADIUS_XXL;
constexpr int RADIUS_TAB = RADIUS_MD;
constexpr int RADIUS_WINDOW_CONTROL = RADIUS_SM;
constexpr int RADIUS_CLOSE_BUTTON = 50; // Circular

// Heights (pixels)
constexpr int HEIGHT_WINDOW_CONTROL = 24;
constexpr int HEIGHT_TAB = 38;
constexpr int HEIGHT_TOOLBAR = 40;
constexpr int HEIGHT_ADDRESS_BAR = 36;
constexpr int HEIGHT_HEADER = 52;

// Widths (pixels)
constexpr int WIDTH_WINDOW_CONTROL = 24;
constexpr int WIDTH_TAB_MIN = 80;
constexpr int WIDTH_TAB_MAX = 220;
constexpr int WIDTH_ADDRESS_BAR_MIN = 260;
constexpr int WIDTH_ICON = 16;
constexpr int WIDTH_ICON_LARGE = 20;

// Padding (pixels)
constexpr int PADDING_BUTTON = 7;
constexpr int PADDING_BUTTON_HORIZONTAL = 9;
constexpr int PADDING_ADDRESS_BAR = 7;
constexpr int PADDING_ADDRESS_BAR_HORIZONTAL = 18;
constexpr int PADDING_TAB = 5;
constexpr int PADDING_TAB_HORIZONTAL = 14;
constexpr int PADDING_HEADER = 4;
constexpr int PADDING_HEADER_HORIZONTAL = 12;

// Margins (pixels)
constexpr int MARGIN_TAB = 2;
constexpr int MARGIN_TAB_RIGHT = 2;
constexpr int MARGIN_NEW_TAB = 8;

// Border widths (pixels)
constexpr int BORDER_WIDTH_THIN = 1;
constexpr int BORDER_WIDTH_MEDIUM = 1.5;
constexpr int BORDER_WIDTH_THICK = 2;

// Icon sizes (pixels)
constexpr int ICON_SIZE_XS = 12;
constexpr int ICON_SIZE_SM = 14;
constexpr int ICON_SIZE_MD = 16;
constexpr int ICON_SIZE_LG = 18;
constexpr int ICON_SIZE_XL = 20;
constexpr int ICON_SIZE_XXL = 24;

// Animation durations (milliseconds)
constexpr int ANIMATION_DURATION_FAST = 100;
constexpr int ANIMATION_DURATION_NORMAL = 120;
constexpr int ANIMATION_DURATION_SLOW = 140;
constexpr int ANIMATION_DURATION_SLOWER = 160;

// Shadow values
constexpr int SHADOW_SMALL = 1;
constexpr int SHADOW_MEDIUM = 4;
constexpr int SHADOW_LARGE = 8;
constexpr int SHADOW_XLARGE = 20;

// Focus ring size (pixels)
constexpr int FOCUS_RING_SIZE = 3;

// Scrollbar dimensions (pixels)
constexpr int SCROLLBAR_WIDTH = 6;
constexpr int SCROLLBAR_MARGIN = 2;

} // namespace open_browser::ui::theme
