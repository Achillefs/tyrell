add_executable(vp330_render
    render_main.cpp
)

target_link_libraries(vp330_render PRIVATE
    vp330_domain
    PkgConfig::SNDFILE
)

target_compile_features(vp330_render PRIVATE cxx_std_20)
