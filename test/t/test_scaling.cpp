
#include <test.hpp>

#include <vtzero/scaling.hpp>

TEST_CASE("Default scaling") {
    const vtzero::scaling s{};

    REQUIRE(s.encode32(5.3) == 5);
    REQUIRE(s.encode64(5.3) == 5);
    REQUIRE(s.decode(5) == Approx(5.0));
}

TEST_CASE("Some scaling") {
    const vtzero::scaling s{2, 0.0000001, 3.5};

    REQUIRE(s.encode32(5.0) == 14999998);
    REQUIRE(s.encode64(5.0) == 14999998);
    REQUIRE(s.decode(14999998) == Approx(5.0));
}

