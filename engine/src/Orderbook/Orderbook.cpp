//
// Created by Matthew Pallan on 4/20/24.
//

#include "Orderbook/Orderbook.hpp"

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
        m_orders.push(order);
    }
    m_condVar.notify_one();
    onOrderQueued(order);
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
            m_orders.pop();
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
        case OrderSide::BUY: fillShit(order, m_asks); break;
        case OrderSide::SELL: fillShit(order, m_bids); break;
        case OrderSide::INVALID: break;
    }
}

void Orderbook::fillLimitOrder(Order& order) {
    switch (order.getSide()) {
        case OrderSide::BUY: fillLimitShit(order, m_asks); break;
        case OrderSide::SELL: fillLimitShit(order, m_bids); break;
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
auto Orderbook::fillShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void {
    for (auto it = priceMap.begin(); it != priceMap.end() && !order.isFilled(); ++it) {
        processOrder(order, it->second);
        if (it->second.empty())
            priceMap.erase(it);
    }
    if (!order.isFilled()) {
        onOrderKill(order);
    }
}

template<typename Comp>
auto Orderbook::fillLimitShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void {
    auto it = priceMap.lower_bound(order.getPrice());
    while (it != priceMap.end() && !order.isFilled() && Comp()(order.getPrice(), it->first)) {
        processOrder(order, it->second);
        if (it->second.empty())
            priceMap.erase(it++);
        else
            ++it;
    }
    if (!order.isFilled()) {
        priceMap[order.getPrice()].push_back(order);
        onOrderAddedToBook(order);
    }
}

void Orderbook::requestStop() {
    m_stopFlag = true;
    m_condVar.notify_one();
    if (m_matchingThread.joinable())
        m_matchingThread.join();
}
