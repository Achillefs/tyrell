add_executable(vp330_tests)

target_link_libraries(vp330_tests PRIVATE
    vp330_domain
    Catch2::Catch2WithMain
    rapidcheck
    rapidcheck_catch
)

target_compile_features(vp330_tests PRIVATE cxx_std_20)

add_subdirectory(golden)

catch_discover_tests(vp330_tests)
