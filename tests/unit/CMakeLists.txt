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
)
