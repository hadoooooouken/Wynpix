## QT QUICK UI MODULES
# Each QML module of the Qt Quick UI is a static library linked
# into wynpix through its static plugin; see quickuihost.cpp.

# Module output directories follow the module URIs (wynpix/ui, wynpix/style,
# wynpix/render) below one shared root, so that qmllint, qmlcachegen and
# qmlls resolve the wynpix.* imports of each module from the build tree.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")
if(NOT "${CMAKE_BINARY_DIR}/qml" IN_LIST QML_IMPORT_PATH)
    list(APPEND QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml")
    set(QML_IMPORT_PATH "${QML_IMPORT_PATH}" CACHE STRING "Qt Creator QML import path" FORCE)
endif()

# Qt generates helper targets for every module (static plugin, plugin init,
# resource and cache object libraries) that are linked into wynpix and
# need the same compile profile as the explicitly created module targets.
wynpix_apply_msvc_directory_profile()

add_subdirectory(bridges)
add_subdirectory(style)
add_subdirectory(render)
add_subdirectory(ui)
