#ifndef ORDERBOOK_ORDERBOOK_HPP
#define ORDERBOOK_ORDERBOOK_HPP

#include "Order.hpp"

#include <map>
#include <thread>
#include <queue>

class Orderbook {
    std::map<Price, std::deque<Order>, std::less<>> m_asks;
    std::map<Price, std::deque<Order>, std::greater<>> m_bids;
    std::queue<Order> m_orders;
    std::mutex m_lock;
    std::condition_variable m_condVar;
    std::atomic<bool> m_stopFlag{false};
    std::thread m_matchingThread;

    std::function<void(Order&)> onOrderAddCallback;
    std::function<void(Order&, Order&)> onOrderFillCallback;
    std::function<void(Order&)> limitOrderAddCallback;
    std::function<void(Order&)> marketOrderFailCallback;

public:
    Orderbook(decltype(onOrderAddCallback) onOrderAddCallback, decltype(onOrderFillCallback) onOrderFillCallback)
        : onOrderAddCallback(std::move(onOrderAddCallback))
        , onOrderFillCallback(std::move(onOrderFillCallback))
        , m_matchingThread(&Orderbook::matchingThread, this) {}

    void addOrder(Order& order) {
        {
            std::lock_guard<std::mutex> lock(m_lock);
            m_orders.push(order);
        }
        m_condVar.notify_one();
        onOrderAddCallback(order);
    }

    void matchingThread() {
        while (true) {
            std::unique_lock<std::mutex> lock(m_lock);
            m_condVar.wait(lock, [this] { return !m_orders.empty() || m_stopFlag; });
            if (m_stopFlag && m_orders.empty())
                return;

            Order order = std::move(m_orders.front());
            m_orders.pop();
            lock.unlock();

            switch (order.getType()) {
                case OrderType::MARKET: fillMarketOrder(order); break;
                case OrderType::LIMIT: fillLimitOrder(order); break;
                case OrderType::INVALID: throw std::runtime_error("Invalid order type");
            }
        }
    }

    void fillMarketOrder(Order& order) {
        switch (order.getSide()) {
            case OrderSide::BUY: fillShit(order, m_asks); break;
            case OrderSide::SELL: fillShit(order, m_bids); break;
            case OrderSide::INVALID: break;
        }
    }

    void fillLimitOrder(Order& order) {
        switch (order.getSide()) {
            case OrderSide::BUY: fillLimitShit(order, m_asks); break;
            case OrderSide::SELL: fillLimitShit(order, m_bids); break;
            case OrderSide::INVALID: break;
        }
    }

    template<typename Comp>
    auto fillShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void {
        for (auto it = priceMap.begin(); it != priceMap.end() && !order.isFilled(); ++it) {
            processOrder(order, it->second);
            if (it->second.empty())
                priceMap.erase(it);
        }
        if (!order.isFilled()) {
            marketOrderFailCallback(order);
        }
    }

    template<typename Comp>
    auto fillLimitShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void {
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
            limitOrderAddCallback(order);
        }
    }

    bool processOrder(Order& order, std::deque<Order>& orders) const {
        while (!order.isFilled() && !orders.empty()) {
            Order& matchOrder = orders.front();
            order.fill(matchOrder);
            onOrderFillCallback(order, matchOrder);
            if (matchOrder.isFilled())
                orders.pop_front();
        }
        return order.isFilled();
    }

    void requestStop() {
        m_stopFlag = true;
        m_condVar.notify_one();
        if (m_matchingThread.joinable())
            m_matchingThread.join();
    }

    ~Orderbook() { requestStop(); }
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
