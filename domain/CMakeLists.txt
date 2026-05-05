add_library(vp330_domain STATIC
    src/engine/SynthesisEngine.cpp
    src/tod/TopOctaveDivider.cpp
    src/tod/OctaveDivider.cpp
    src/keygate/KeyGate.cpp
    src/keyboard/MkIIKeyboard.cpp
    src/section/BpCascade.cpp
    src/section/ChoirFilterBank.cpp
    src/section/ChoirSection.cpp
)

target_include_directories(vp330_domain
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(vp330_domain PUBLIC cxx_std_20)
