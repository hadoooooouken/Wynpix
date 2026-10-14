## QML MODULE wynpix.bridges
# Typed, notifying QML views of the global services: AppSettings, Theme,
# Actions and the SettingsEnums / FluentIcons enumerations. The bridges hold
# snapshots or talk to IActionDispatcher only; they are fed by QuickUiHost
# (gui/quick/adapters) and never read the global singletons, so the QML test
# runner links them without the application services.

qt_add_library(wynpix_bridges STATIC)

qt_add_qml_module(wynpix_bridges
    URI wynpix.bridges
    DEPENDENCIES QtQuick
    SOURCES
        actionbridge.cpp
        actionbridge.h
        actiondispatcher.h
        publish.h
        qmlinputevents.cpp
        qmlinputevents.h
        settingsbridge.cpp
        settingsbridge.h
        settingsenums.h
        themebridge.cpp
        themebridge.h
        themedata.h
        uisettings.h
)

# Bridge headers include application headers (settings_types.h,
# gui/uimetrics.h, utils/fluenticon.h) relative to the application root.
target_include_directories(wynpix_bridges PUBLIC "${PROJECT_SOURCE_DIR}/src")

target_compile_features(wynpix_bridges PUBLIC cxx_std_23)
if(MSVC)
    # Same C++26 working-draft switch as wynpix.
    target_compile_options(wynpix_bridges PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
endif()

target_link_libraries(wynpix_bridges
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Qml
        wynpix_fluenticons
    PRIVATE
        Qt6::Quick
)
