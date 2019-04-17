
#include <test.hpp>

#include <vtzero/scaling.hpp>

#include <type_traits>

static_assert(std::is_nothrow_move_constructible<vtzero::scaling>::value, "scaling is not nothrow move constructible");
static_assert(std::is_nothrow_move_assignable<vtzero::scaling>::value, "scaling is not nothrow move assignable");

TEST_CASE("Default scaling") {
    const vtzero::scaling s1{};
    const vtzero::scaling s2{};

    REQUIRE(s1.is_default());
    REQUIRE(s1.offset() == 0);
    REQUIRE(s1.multiplier() == Approx(1.0));
    REQUIRE(s1.base() == Approx(0.0));
    REQUIRE(s1.encode32(5.3) == 5);
    REQUIRE(s1.encode64(5.3) == 5);
    REQUIRE(s1.decode(5) == Approx(5.0));
    REQUIRE(s1 == s2);
}

TEST_CASE("Some scaling") {
    const vtzero::scaling s{2, 0.0000001, 3.5};

    REQUIRE_FALSE(s.is_default());
    REQUIRE(s.encode32(5.0) == 14999998);
    REQUIRE(s.encode64(5.0) == 14999998);
    REQUIRE(s.decode(14999998) == Approx(5.0));
}

