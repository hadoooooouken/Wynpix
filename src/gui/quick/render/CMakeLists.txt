## QML MODULE wynpix.render
# GPU image rendering for the Qt Quick UI (Phase 1 of
# docs/QML_MIGRATION_PLAN.md): ImageRenderItem (QQuickRhiItem) and its
# render-thread ImageRenderer (with RenderDevice and ImageLayer), drawing
# through QRhi with the RHI shaders in res/shaders/rhi (tile drawing with
# sharpening and colour adjustments, the exact-ratio box downsample, the
# Magic Kernel Sharp 2021 resampling, the source conversion: HDR tone mapping
# and display colour management, and the equirectangular panorama), and
# ThumbnailItem, the scene graph item of one thumbnail (thumbnail.vert / .frag).

qt_add_library(wynpix_render STATIC)

qt_add_qml_module(wynpix_render
    URI wynpix.render
    DEPENDENCIES QtQuick
    SOURCES
        colorlutbuilder.cpp
        colorlutbuilder.h
        colortransformplan.cpp
        colortransformplan.h
        imagefiltermode.cpp
        imagefiltermode.h
        imagelayer.cpp
        imagelayer.h
        imagerenderer.cpp
        imagerenderer.h
        imagerenderitem.cpp
        imagerenderitem.h
        renderframe.h
        rendererrorchannel.cpp
        rendererrorchannel.h
        rendererrorreporter.cpp
        rendererrorreporter.h
        renderdevice.cpp
        renderdevice.h
        renderstatistics.h
        resamplegrid.cpp
        resamplegrid.h
        rhipassuniforms.h
        thumbnailitem.cpp
        thumbnailitem.h
        thumbnailmaterial.cpp
        thumbnailmaterial.h
        textureuploadformat.cpp
        textureuploadformat.h
        tilegrid.cpp
        tilegrid.h
)

# GLSL 440 sources compiled to .qsb (SPIR-V, HLSL and GLSL variants) for
# every QRhi backend; ImageRenderer loads them from :/wynpix/render/shaders.
set(WYNPIX_RHI_SHADER_DIR "${PROJECT_SOURCE_DIR}/src/res/shaders/rhi")
qt_add_shaders(wynpix_render "wynpix_render_shaders"
    PREFIX "/wynpix/render/shaders"
    BASE "${WYNPIX_RHI_SHADER_DIR}"
    FILES
        "${WYNPIX_RHI_SHADER_DIR}/image.vert"
        "${WYNPIX_RHI_SHADER_DIR}/image.frag"
        "${WYNPIX_RHI_SHADER_DIR}/boxreduce.vert"
        "${WYNPIX_RHI_SHADER_DIR}/boxreduce.frag"
)

# ThumbnailItem draws through a scene graph material, whose vertex shader
# the batching renderer needs in its batchable variant as well.
qt_add_shaders(wynpix_render "wynpix_render_thumbnail_shaders"
    PREFIX "/wynpix/render/shaders"
    BASE "${WYNPIX_RHI_SHADER_DIR}"
    BATCHABLE
    FILES
        "${WYNPIX_RHI_SHADER_DIR}/thumbnail.vert"
        "${WYNPIX_RHI_SHADER_DIR}/thumbnail.frag"
)

# The resampling passes read exact texels with texelFetch() and loop over a
# scale-dependent number of taps, the conversion pass also samples a 3D
# lookup table, and the panorama samples with explicit gradients
# (textureGrad); GLSL ES 1.00 can express none of them, so their OpenGL
# variants
# start at GLSL ES 3.00 / GLSL 3.30. The Quick UI renders on Direct3D 11 by
# default and never on an OpenGL ES 2 context.
qt_add_shaders(wynpix_render "wynpix_render_resample_shaders"
    PREFIX "/wynpix/render/shaders"
    BASE "${WYNPIX_RHI_SHADER_DIR}"
    GLSL "300es,330"
    FILES
        "${WYNPIX_RHI_SHADER_DIR}/resample.vert"
        "${WYNPIX_RHI_SHADER_DIR}/resample.frag"
        "${WYNPIX_RHI_SHADER_DIR}/convert.vert"
        "${WYNPIX_RHI_SHADER_DIR}/convert.frag"
        "${WYNPIX_RHI_SHADER_DIR}/panorama.vert"
        "${WYNPIX_RHI_SHADER_DIR}/panorama.frag"
)

# Headers include each other relative to the application root.
target_include_directories(wynpix_render PUBLIC "${PROJECT_SOURCE_DIR}/src")

target_compile_features(wynpix_render PUBLIC cxx_std_23)
if(MSVC)
    # Same C++26 working-draft switch as wynpix.
    target_compile_options(wynpix_render PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>")
endif()

# QRhi (<rhi/qrhi.h>) is published through GuiPrivate, with limited
# compatibility guarantees across Qt minor versions.
target_link_libraries(wynpix_render
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::GuiPrivate
        Qt6::Qml
        Qt6::Quick
)
