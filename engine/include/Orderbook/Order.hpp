//
// Created by Matthew Pallan on 4/8/24.
//

#ifndef ORDERBOOK_ORDER_HPP
#define ORDERBOOK_ORDER_HPP

#include "Price.hpp"
#include <cstdint>

using OrderId_ty = std::uint64_t;
using Quantity_ty = std::uint64_t;

enum class OrderSide { BUY, SELL, INVALID };
enum class OrderType { LIMIT, MARKET, INVALID };

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
        const OrderId_ty id, const Price price, const Quantity_ty quantity, const OrderSide side, const OrderType type)
        : m_price(price)
        , m_initQuantity(quantity)
        , m_remainingQuantity(quantity)
        , m_id(id)
        , m_side(side)
        , m_type(type) {}

    Order(const OrderId_ty orderId, const OrderSide side, const Quantity_ty quantity)
        : Order(orderId, Price::INVALID_PRICE, quantity, side, OrderType::MARKET) {}

    [[nodiscard]] auto getPrice() -> Price& { return m_price; }
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
#endif    // ORDERBOOK_ORDER_HPP
