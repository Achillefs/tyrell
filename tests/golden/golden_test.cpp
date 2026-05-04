#include <catch2/catch_test_macros.hpp>

TEST_CASE("test harness boots", "[golden][smoke]") {
    REQUIRE(2 + 2 == 4);
}
