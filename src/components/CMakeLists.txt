## SOURCES FOR WYNPIX COMPONENTS

target_sources(wynpix PRIVATE
    directorymodel.cpp
    directorypresenter.cpp
    directoryexpandworker.cpp
    foldercoverresolver.cpp

    scriptmanager/scriptmanager.cpp
    actionmanager/actionmanager.cpp
    actionmanager/shortcutmap.cpp

    cache/cache.cpp
    cache/decodedthumbnailcache.cpp
    cache/thumbnailcache.cpp
    cache/thumbnailcachemaintenance.cpp
    cache/thumbnailsourcestamp.cpp
    cache/thumbnailcachewriter.cpp

    loader/loader.cpp
    loader/loaderrunnable.cpp

    thumbnailer/thumbnailer.cpp
    thumbnailer/thumbnailgeometry.cpp
    thumbnailer/thumbnailerrunnable.cpp

    directorymanager/directorymanager.cpp

    directorymanager/watchers/directorywatcher.cpp
    directorymanager/watchers/watcherevent.cpp
    directorymanager/watchers/watcherworker.cpp
    directorymanager/watchers/windows/windowswatcher.cpp
    directorymanager/watchers/windows/windowsworker.cpp

    fileoptask/fileopcontroller.cpp
    fileoptask/fileoptaskrunnable.cpp

    batchconverter/batchconverter.cpp

    wallpaper/wallpapercontroller.cpp

    viewmode/viewmodecontroller.cpp

    shellinfo/windowtitle.cpp
    singleinstance/singleinstancechannel.cpp
    windowstate/windowstatecontroller.cpp
    viewertoggles/appviewertogglestore.cpp
    viewertoggles/appviewertogglestore.h

    mimepayload/mimepayloadmanager.cpp

    imagepreparer/imagepreparer.cpp

    comfymetadata/comfyksamplerparser.cpp
    comfymetadata/pngtextchunkreader.cpp
)

# UPSCAYL SUPPORT
target_sources(wynpix PRIVATE
    upscaler/upscaledecision.cpp
    upscaler/upscaler.cpp
    upscaler/upscalerrunnable.cpp
    upscaler/upscaylresizerunnable.cpp
    upscaler/upscaylresizerunnable.h
)
