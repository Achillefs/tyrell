# Characterization (L2) tests. See SPEC.md §7 — L2 verifies frequency response,
# time-domain behavior, stability, and property-based invariants. Each new DSP
# component lands its L2 contract here in the same patch as the component.
target_include_directories(vp330_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_sources(vp330_tests PRIVATE
    # populated by Task 5
)
