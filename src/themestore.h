#pragma once

#include <QDebug>
#include <QColor>
#include <QPalette>

enum ColorSchemes {
    COLORS_LIGHT,
    COLORS_DARK
};

struct BaseColorScheme {
    int tid;
    QColor background;
    QColor background_fullscreen;
    QColor text;
    QColor icons;
    QColor folder_icons;
    QColor thumbnail_folder_icons;
    QColor widget;
    QColor widget_border;
    QColor accent;
    QColor folderview;
    QColor folderview_topbar;
    QColor thumbpanel;
    QColor scrollbar;
    QColor overlay_text;
    QColor overlay;
    QColor status_pending;
    QColor status_error;
    QColor status_processing;
    QColor status_success;
    QColor danger;
    QColor trash;
};

class ColorScheme {
public:
    ColorScheme();
    ColorScheme(BaseColorScheme base);
    void setBaseColors(BaseColorScheme base);
    // index of theme name
    int tid;
    // base
    QColor background;
    QColor background_fullscreen;
    QColor text;
    QColor icons;
    QColor folder_icons;
    QColor thumbnail_folder_icons;
    QColor widget;
    QColor widget_border;
    QColor accent;
    QColor folderview;
    QColor folderview_topbar;
    QColor thumbpanel;
    QColor scrollbar;
    QColor scrollbar_hover;
    QColor overlay_text;
    QColor overlay;
    QColor status_pending;
    QColor status_error;
    QColor status_processing;
    QColor status_success;
    QColor danger;
    QColor trash;
    // extended
    QColor text_hc2;
    QColor text_hc;
    QColor text_lc;
    QColor folderview_hc;
    QColor folderview_hc2;
    QColor thumbpanel_hc;
    QColor thumbpanel_hc2;
    QColor thumbpanel_text;
    // combobox panel colors, kept in the theme files. Tuned to stay
    // visible against both the app's own surfaces and the OS-tinted
    // native dialog backgrounds.
    QColor combo_field;
    QColor combo_field_hover;
    QColor combo_field_pressed;
    QColor combo_field_border;


private:
    void createColorVariants();
};

class ThemeStore {
public:
    static ColorScheme colorScheme(ColorSchemes name);
};
