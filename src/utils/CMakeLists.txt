## SOURCES FOR WYNPIX UTILITY CLASSES

# ZSTD_INSTALL_DIR comes from src/CMakeLists.txt.
set(WYNPIX_ZSTD_INCLUDE_DIR "${ZSTD_INSTALL_DIR}/include")
set(WYNPIX_ZSTD_LIBRARY "${ZSTD_INSTALL_DIR}/lib/zstd.lib")
set(WYNPIX_ZSTD_RUNTIME "${ZSTD_INSTALL_DIR}/bin/zstd.dll")

if(NOT EXISTS "${WYNPIX_ZSTD_INCLUDE_DIR}/zstd.h"
   OR NOT EXISTS "${WYNPIX_ZSTD_LIBRARY}"
   OR NOT EXISTS "${WYNPIX_ZSTD_RUNTIME}")
    message(FATAL_ERROR "zstd shared build not found under ${ZSTD_INSTALL_DIR}")
endif()

target_include_directories(wynpix PRIVATE "${WYNPIX_ZSTD_INCLUDE_DIR}")
target_link_libraries(wynpix PRIVATE "${WYNPIX_ZSTD_LIBRARY}")

target_sources(wynpix PRIVATE
    actions.cpp
    blendreader.cpp
    coloradjustments.h
    decodecontext.h
    djvureader.cpp
    fontpreview.cpp
    cmdoptionsrunner.cpp
    formatregistry.cpp
    imagefactory.cpp
    imagelib.cpp
    inputmap.cpp
    randomizer.cpp
    script.cpp
    startuptiming.cpp
    startuptiming.h
    stuff.cpp
    fileoperations.cpp
    colormanager.cpp
    pngwriter.cpp
    iconfontmanager.cpp
    hdrtonemapper.cpp
    hdrtonemapper.h
    hdrsource.h
)
