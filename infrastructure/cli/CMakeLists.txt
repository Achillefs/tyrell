add_library(vp330_cli_smf STATIC
    SmfReader.cpp
)

target_include_directories(vp330_cli_smf
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(vp330_cli_smf PUBLIC
    vp330_domain
)

target_compile_features(vp330_cli_smf PUBLIC cxx_std_20)

if(VP330_ENABLE_RENDER)
    add_executable(vp330_render render_main.cpp)
    target_link_libraries(vp330_render PRIVATE vp330_cli_smf PkgConfig::SNDFILE)
    target_compile_features(vp330_render PRIVATE cxx_std_20)
endif()
