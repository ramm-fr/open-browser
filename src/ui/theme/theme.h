#pragma once

#include "colors.h"
#include "metrics.h"
#include "typography.h"

namespace open_browser::ui::theme {

// Theme mode
enum class ThemeMode { Light, Dark };

// Get color based on current theme mode
inline const char *get_color(ThemeMode mode, const char *light_color,
                             const char *dark_color) {
  return mode == ThemeMode::Light ? light_color : dark_color;
}

// Helper functions to get themed colors
inline const char *window_bg(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_WINDOW_BG, COLOR_DARK_WINDOW_BG);
}

inline const char *header_bg(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_HEADER_BG, COLOR_DARK_HEADER_BG);
}

inline const char *surface(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_SURFACE, COLOR_DARK_SURFACE);
}

inline const char *border(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_BORDER, COLOR_DARK_BORDER);
}

inline const char *text(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_TEXT, COLOR_DARK_TEXT);
}

inline const char *text_muted(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_TEXT_MUTED, COLOR_DARK_TEXT_MUTED);
}

inline const char *accent(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ACCENT, COLOR_DARK_ACCENT);
}

inline const char *accent_hover(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ACCENT_HOVER, COLOR_DARK_ACCENT_HOVER);
}

inline const char *hover(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_HOVER, COLOR_DARK_HOVER);
}

inline const char *active(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ACTIVE, COLOR_DARK_ACTIVE);
}

inline const char *tab_bg(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_TAB_BG, COLOR_DARK_TAB_BG);
}

inline const char *tab_active(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_TAB_ACTIVE, COLOR_DARK_TAB_ACTIVE);
}

inline const char *address_bar(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ADDRESS_BAR, COLOR_DARK_ADDRESS_BAR);
}

inline const char *address_bar_focus(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ADDRESS_BAR_FOCUS,
                   COLOR_DARK_ADDRESS_BAR_FOCUS);
}

inline const char *address_bar_border(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ADDRESS_BAR_BORDER,
                   COLOR_DARK_ADDRESS_BAR_BORDER);
}

inline const char *address_bar_border_focus(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_ADDRESS_BAR_BORDER_FOCUS,
                   COLOR_DARK_ADDRESS_BAR_BORDER_FOCUS);
}

inline const char *shadow(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_SHADOW, COLOR_DARK_SHADOW);
}

inline const char *focus_ring(ThemeMode mode) {
  return get_color(mode, COLOR_LIGHT_FOCUS_RING, COLOR_DARK_FOCUS_RING);
}

} // namespace open_browser::ui::theme
