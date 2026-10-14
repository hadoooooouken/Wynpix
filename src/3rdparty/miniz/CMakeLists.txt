add_library(wynpix_miniz STATIC
    miniz.c
    miniz_zip.c
    miniz_tdef.c
    miniz_tinfl.c
)

target_include_directories(wynpix_miniz PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

wynpix_apply_msvc_profile(wynpix_miniz)
wynpix_suppress_crt_deprecation(wynpix_miniz)
