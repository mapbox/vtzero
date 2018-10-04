
#include <test.hpp>

#include <vtzero/scaling.hpp>

TEST_CASE("Default scaling") {
    const vtzero::scaling s;

    REQUIRE(s.encode(5.3) == 5);
    REQUIRE(s.decode(5) == Approx(5.0));
}

TEST_CASE("Some scaling") {
    const vtzero::scaling s{2, 0.1, 3.5};

    REQUIRE(s.encode(5.0) == 13);
    REQUIRE(s.decode(13) == Approx(5.0));
}

