//
// Created by Matthew Pallan on 4/8/24.
//

#ifndef ORDERBOOK_PRICE_HPP
#define ORDERBOOK_PRICE_HPP

#include <compare>
#include <cstdint>
#include <string>

class Price {
    static constexpr auto SCALAR = 10000;
    std::uint64_t integral{};
    std::uint64_t fractional{};

public:
    static constexpr auto INVALID_PRICE = std::numeric_limits<std::uint64_t>::quiet_NaN();

    constexpr Price()
        : integral(INVALID_PRICE)
        , fractional(INVALID_PRICE) {}

    constexpr Price(float price)
        : integral(static_cast<std::uint64_t>(price))
        , fractional(static_cast<std::uint64_t>((price - integral) * SCALAR)) {}

    constexpr Price(const std::uint64_t integral, const std::uint64_t fractional)
        : integral(integral)
        , fractional(fractional) {}

    [[nodiscard]] constexpr auto getIntegral() const -> uint64_t { return integral; }
    [[nodiscard]] constexpr auto getFractional() const -> uint64_t { return fractional; }

    constexpr explicit operator float() const { return integral + fractional / SCALAR; }

    constexpr auto operator<=>(const Price& rhs) const {
        if (integral != rhs.integral) {
            return integral <=> rhs.integral;
        }
        return fractional <=> rhs.fractional;
    }

    explicit operator std::string() const {
        // rvo'd, should use fmt or std::format instead but compiler is not u2d
        std::string str = std::to_string(integral) + "." + std::to_string(fractional);
        return str;
    }
};

#endif    // ORDERBOOK_PRICE_HPP
