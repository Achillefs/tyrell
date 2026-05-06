# Each subsequent task adds its test source via target_sources(vp330_tests PRIVATE …)
# from a sub-CMakeLists or directly here. Keep this file as the single hook.
target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/values/HertzTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/note/MidiNoteTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/values/PitchTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/MidiEventTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/engine/SynthesisEngineTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cli/SmfReaderTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tod/TopOctaveDividerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tod/OctaveDividerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keygate/KeyGateTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keyboard/MkIIKeyboardTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keyboard/MkIIKeyboardTuningTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/section/BpCascadeTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirFilterBankTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/section/ChoirSectionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modulation/LfoTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modulation/VibratoTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/BbdLineTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ensemble/EnsembleTest.cpp
)
