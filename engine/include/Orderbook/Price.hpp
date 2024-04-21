//
// Created by Matthew Pallan on 4/8/24.
//

#ifndef ORDERBOOK_PRICE_HPP
#define ORDERBOOK_PRICE_HPP

#include <compare>
#include <cstdint>
#include <string>
#include <cmath>
#include <fmt/format.h>

class Price {
    static constexpr auto SCALAR = 10000;
    std::int64_t integral{};
    std::uint64_t fractional{};

public:
    static constexpr auto INVALID_PRICE = std::numeric_limits<std::uint64_t>::quiet_NaN();

    Price()
        : integral(INVALID_PRICE)
        , fractional(INVALID_PRICE) {}

    constexpr Price(float price)
        : integral(static_cast<std::int64_t>(price))
        , fractional(static_cast<std::uint64_t>((price - integral) * SCALAR)) {}

    constexpr Price(const std::uint64_t integral, const std::uint64_t fractional)
        : integral(integral)
        , fractional(fractional) {}

    [[nodiscard]] constexpr auto getIntegral() const -> uint64_t { return integral; }
    [[nodiscard]] constexpr auto getFractional() const -> uint64_t { return fractional; }

    constexpr explicit operator double() const { return integral + fractional / SCALAR; }

    constexpr auto operator<=>(const Price& rhs) const {
        if (integral != rhs.integral) {
            return integral <=> rhs.integral;
        }
        return fractional <=> rhs.fractional;
    }

    explicit operator std::string() const { return fmt::format("{}.{}", integral, fractional); }
};

template<>
struct fmt::formatter<Price> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Price& price, FormatContext& ctx) {
        return format_to(ctx.out(), "{}.{}", price.getIntegral(), price.getFractional());
    }
};

namespace {
    static_assert(Price{10.1} > Price{10.0});
    static_assert(Price{10.0} < Price{10.1});
    static_assert(Price{9, 120} < Price{10, 0});
    static_assert(Price{10, 0} > Price{9, 120});
}    // namespace

#endif    // ORDERBOOK_PRICE_HPP
