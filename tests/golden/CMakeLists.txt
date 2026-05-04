target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/golden_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/render.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers/wav_io.cpp
)

target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/helpers
)

target_link_libraries(vp330_tests PRIVATE
    PkgConfig::SNDFILE
)

target_compile_definitions(vp330_tests PRIVATE
    VP330_RENDER_BINARY="$<TARGET_FILE:vp330_render>"
    GOLDEN_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    GOLDEN_BASELINES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/baselines"
)

add_dependencies(vp330_tests vp330_render)
