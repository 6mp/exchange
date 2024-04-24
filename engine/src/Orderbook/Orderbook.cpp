//
// Created by Matthew Pallan on 4/20/24.
//

#include "Orderbook/Orderbook.hpp"

#include <iostream>
#include <mutex>

Orderbook::Orderbook(
    decltype(onOrderQueued) onOrderAdd,
    decltype(onOrderFill) onOrderFill,
    decltype(onOrderAddedToBook) onOrderAddedToBook,
    decltype(onOrderKill) onOrderKill)
    : onOrderQueued(std::move(onOrderAdd))
    , onOrderFill(std::move(onOrderFill))
    , onOrderAddedToBook(std::move(onOrderAddedToBook))
    , onOrderKill(std::move(onOrderKill))
    , m_matchingThread(&Orderbook::matchingThread, this) {}

Orderbook::~Orderbook() { requestStop(); }

auto Orderbook::addOrder(const Order& order) -> void {
    {
        std::scoped_lock lock{m_lock};
        m_orders.push_back(order);
    }
    onOrderQueued(order);
    m_condVar.notify_one();
}

auto Orderbook::removeOrder(const Order& order) -> void {
    std::scoped_lock lock{m_lock};

    static auto orderMatchingPredicate = [&order](const Order& o) { return o.getId() == order.getId(); };

    auto curriedDeleteThing = [this](const Order& order, auto& collection) {
        if (const auto it = collection.find(order.getPrice()); it != collection.end()) {
            if (const auto erasedCount = std::erase_if(it->second, orderMatchingPredicate); erasedCount == 1) {
                onOrderDeleted(order);
                return true;
            }
        }

        return false;
    };

    // try and delete from the order queue first
    if (curriedDeleteThing(order, m_orders)) {
        return;
    }

    // if its not in there get it from the maps
    switch (order.getSide()) {
        case OrderSide::BUY: curriedDeleteThing(order, m_bids); break;
        case OrderSide::SELL: curriedDeleteThing(order, m_asks); break;
        case OrderSide::INVALID: break;
    }
}

void Orderbook::matchingThread() {
    while (true) {

        Order order;
        {
            std::unique_lock lock{m_lock};
            m_condVar.wait(lock, [this] { return !m_orders.empty() || m_stopFlag; });
            if (m_stopFlag && m_orders.empty())
                return;

            order = m_orders.front();

            // delete item at front
            m_orders.erase(m_orders.begin());
        }

        switch (order.getType()) {
            case OrderType::MARKET: fillMarketOrder(order); break;
            case OrderType::LIMIT: fillLimitOrder(order); break;
            case OrderType::INVALID: throw std::runtime_error("Invalid order type");
        }
    }
}

void Orderbook::fillMarketOrder(Order& order) {
    switch (order.getSide()) {
        case OrderSide::BUY: matchMarketOrder(order, m_asks); break;
        case OrderSide::SELL: matchMarketOrder(order, m_bids); break;
        case OrderSide::INVALID: break;
    }
}

void Orderbook::fillLimitOrder(Order& order) {
    switch (order.getSide()) {
        case OrderSide::BUY: matchLimitOrder(order, m_asks); break;
        case OrderSide::SELL: matchLimitOrder(order, m_bids); break;
        case OrderSide::INVALID: break;
    }
}

bool Orderbook::processOrder(Order& order, std::deque<Order>& orders) const {
    while (!order.isFilled() && !orders.empty()) {
        Order& matchOrder = orders.front();
        order.fill(matchOrder);
        onOrderFill(order, matchOrder);
        if (matchOrder.isFilled())
            orders.pop_front();
    }
    return order.isFilled();
}

template<typename Comp>
auto Orderbook::matchMarketOrder(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void {
    for (auto it = priceMap.begin(); it != priceMap.end() && !order.isFilled(); ++it) {
        processOrder(order, it->second);
    }
    if (!order.isFilled()) {
        onOrderKill(order);
    }
}

template<typename Comp>
void Orderbook::matchLimitOrder(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) {
    if constexpr (std::is_same_v<Comp, std::less<>>) {    // Handling buy orders
                                                          // Finding the lowest price at or below the order price
        auto it = priceMap.lower_bound(order.getPrice());

        if (it != priceMap.end() && it->first > order.getPrice()) {
            // If the found price is higher than the order price, no matching order, so step back
            if (it != priceMap.begin()) {
                --it;
            } else {
                // All existing prices are higher, no potential matches
                m_bids[order.getPrice()].push_back(order);
                onOrderAddedToBook(order);
                return;
            }
        }

        while (it != priceMap.end() && !order.isFilled()) {
            processOrder(order, it->second);
            if (it->second.empty()) {
                // Remove the price level if no orders are left
                it = priceMap.erase(it);
            } else {
                ++it;
            }
        }

        if (!order.isFilled()) {
            m_bids[order.getPrice()].push_back(order);
            onOrderAddedToBook(order);
        }
    } else if constexpr (std::is_same_v<Comp, std::greater<>>) {    // Handling sell orders
        // Finding the highest price at or above the order price
        auto it = priceMap.lower_bound(order.getPrice());

        if (it == priceMap.begin() || (it != priceMap.end() && it->first < order.getPrice())) {
            // If the lowest price in the map is still below the order price, go to the end
            m_asks[order.getPrice()].push_back(order);
            onOrderAddedToBook(order);
            return;
        }

        // Correct initial position if pointing beyond the first eligible price
        if (it != priceMap.begin() && (it == priceMap.end() || it->first > order.getPrice())) {
            --it;
        }

        while (it != priceMap.begin() && !order.isFilled()) {
            processOrder(order, it->second);
            if (it->second.empty()) {
                it = priceMap.erase(it);
                if (it != priceMap.begin()) {
                    --it;    // Ensure iterator does not become invalid after erasure
                }
            } else {
                --it;    // Move to the next lower price
            }
        }

        if (!order.isFilled()) {
            m_asks[order.getPrice()].push_back(order);
            onOrderAddedToBook(order);
        }
    }
}

void Orderbook::requestStop() {
    m_stopFlag = true;
    m_condVar.notify_one();
    if (m_matchingThread.joinable())
        m_matchingThread.join();
}
