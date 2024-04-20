//
// Created by Matthew Pallan on 4/8/24.
//

#ifndef ORDERBOOK_ORDER_HPP
#define ORDERBOOK_ORDER_HPP

#include "Price.hpp"
#include <cstdint>

using OrderId_ty = std::uint64_t;
using Quantity_ty = std::uint64_t;

enum class OrderSide : uint8_t { BUY, SELL, INVALID };
enum class OrderType : uint8_t { LIMIT, MARKET, INVALID };

class Order {
    Price m_price;
    Quantity_ty m_initQuantity{};
    Quantity_ty m_remainingQuantity{};
    OrderId_ty m_id{};
    OrderSide m_side{OrderSide::INVALID};
    OrderType m_type{OrderType::INVALID};

public:
    Order() = default;

    Order(
        const OrderId_ty id,
        const Price price,
        const Quantity_ty quantity,
        const OrderSide side,
        const OrderType type = OrderType::LIMIT)
        : m_price(price)
        , m_initQuantity(quantity)
        , m_remainingQuantity(quantity)
        , m_id(id)
        , m_side(side)
        , m_type(type) {}

    Order(const OrderId_ty orderId, const OrderSide side, const Quantity_ty quantity)
        : Order(orderId, Price::INVALID_PRICE, quantity, side, OrderType::MARKET) {}

    [[nodiscard]] auto getPrice() const -> Price { return m_price; }
    [[nodiscard]] auto getType() const -> OrderType { return m_type; }
    [[nodiscard]] auto getId() const -> OrderId_ty { return m_id; }
    [[nodiscard]] auto getSide() const -> OrderSide { return m_side; }
    [[nodiscard]] auto getInitialQuantity() const -> Quantity_ty { return m_initQuantity; }
    [[nodiscard]] auto getRemainingQuantity() const -> Quantity_ty { return m_remainingQuantity; }
    [[nodiscard]] auto getFilledQuantity() const -> Quantity_ty { return m_initQuantity - m_remainingQuantity; }
    [[nodiscard]] auto isFilled() const -> bool { return m_remainingQuantity == 0; }

    auto fill(const Quantity_ty quantity) -> void {
        if (quantity > m_remainingQuantity) {
            throw std::runtime_error("Cannot fill more than remaining quantity");
        }

        m_remainingQuantity -= quantity;
    }

    auto fill(Order& restingOrder) -> void {
        const auto quantity = std::min(m_remainingQuantity, restingOrder.getRemainingQuantity());
        fill(quantity);
        restingOrder.fill(quantity);
    }
};

template<>
struct fmt::formatter<Order> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template<typename FormatContext>
    auto format(const Order& p, FormatContext& ctx) const {
        static const char* sides[] = {"BUY", "SELL", "INVALID"};
        static const char* types[] = {"LIMIT", "MARKET", "INVALID"};

        if (p.getType() == OrderType::MARKET) {
            return format_to(
                ctx.out(),
                "{}:{}:(id:{}) => quantity: {}, remaining: {}",
                types[1],
                sides[static_cast<int>(p.getSide())],
                p.getId(),
                p.getInitialQuantity(),
                p.getRemainingQuantity());
        }

        return format_to(
            ctx.out(),
            "{}:{}:(id:{}) => price: {}, quantity: {}, remaining: {}",
            types[static_cast<int>(p.getType())],
            sides[static_cast<int>(p.getSide())],
            p.getId(),
            static_cast<std::string>(p.getPrice()),
            p.getInitialQuantity(),
            p.getRemainingQuantity());
    }
};
#endif    // ORDERBOOK_ORDER_HPP
