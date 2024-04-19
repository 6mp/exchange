//
// Created by Matthew Pallan on 4/9/24.
//

#ifndef ORDERBOOK_ORDERBOOK_HPP
#define ORDERBOOK_ORDERBOOK_HPP

// broadcast through grpc to all clients which orderids get filled

#include "Order.hpp"

#include <map>
#include <vector>
#include <queue>
#include <deque>
#include <thread>
#include <mutex>

class Orderbook {

    /*
     * ASKS
     * 115 |  10
     * 114 |  20
     * 113 |  30
     * ---------
     * 113 |  30
     * 114 |  20
     * 115 |  10
     * BIDS
     */

    // price levels
    std::map<Price, std::deque<Order>, std::less<>> m_asks;
    std::map<Price, std::deque<Order>, std::greater<>> m_bids;

    // queue to get orders in asap, orders matched off queue
    std::queue<Order> m_orders;

    // order created is param
    std::function<void(Order&)> onOrderAddCallback;

    // first order is the new order that got filled, second is the resting order used to fill
    std::function<void(Order&, Order&)> onOrderFillCallback;

    // thread stuff for matching and adding orders
    std::mutex m_lock;
    std::atomic<bool> m_stopFlag;
    std::thread m_matchingThread;

public:
    Orderbook(decltype(onOrderAddCallback) onOrderAddCallback, decltype(onOrderFillCallback) onOrderFillCallback)
        : onOrderAddCallback(std::move(onOrderAddCallback))
        , onOrderFillCallback(std::move(onOrderFillCallback))
        , m_matchingThread(&Orderbook::matchingThread, this) {}

    auto addOrder(Order& order) -> void {
        {
            std::scoped_lock lock{m_lock};
            m_orders.push(order);
        }
        onOrderAddCallback(order);
    }

    auto matchingThread() -> void {
        while (!m_stopFlag) {
            // take latest order from orders
            Order order;
            {
                std::scoped_lock lock{m_lock};
                order = m_orders.back();
                m_orders.pop();
            }

            switch (order.getType()) {
                case OrderType::MARKET: {
                    handleMarketOrder(order);
                    break;
                }
                case OrderType::LIMIT: {
                    // limit orders can be done on a separate thread
                    std::thread j{[this, &order] { handleLimitOrder(order); }};
                    j.detach();
                    break;
                }
                case OrderType::INVALID: throw std::runtime_error("invalid order type");
            }
        }
    }

    auto fillMarketOrder(Order& order) -> void {
        switch (order.getSide()) {
            case OrderSide::BUY: {
                // execute best price so top of asks
                auto& top = m_asks.begin()->second;
                // start filling
                if (!processOrders(order, top)) {
                    // no match, notify that client of that
                }

                break;
            }
            case OrderSide::SELL: break;
            case OrderSide::INVALID: break;
        }
    }

    auto fillLimitOrder() -> void {}

    auto fillAndNotify(Order& order) -> bool {
        if (order.getSide() == OrderSide::BUY) {
            if (m_asks.empty())
                return false;
            auto& askOrders = m_asks.begin()->second;
            auto askPrice = m_asks.begin()->first;

            // Only fill if the buy order price is >= lowest ask
            if (order.getPrice() >= askPrice) {
                return processOrders(order, askOrders);
            }
        } else if (order.getSide() == OrderSide::SELL) {
            if (m_bids.empty())
                return false;
            auto& bidOrders = m_bids.begin()->second;
            auto bidPrice = m_bids.begin()->first;

            // Only fill if the sell order price is <= highest bid
            if (order.getPrice() <= bidPrice) {
                return processOrders(order, bidOrders);
            }
        } else {
            throw std::runtime_error("invalid order side");
        }
        return false;
    }

    bool processOrders(Order& order, std::deque<Order>& orders) {
        if (orders.empty())
            return false;

        while (!order.isFilled() && !orders.empty()) {
            auto& matchOrder = orders.front();
            order.fill(matchOrder);
            if (matchOrder.isFilled()) {
                orders.pop_front();
            }
            onOrderFillCallback(order, matchOrder);
        }
        return true;
    }

    void handleMarketOrder(Order& order) { fillAndNotify(order); }

    void handleLimitOrder(Order& order) {
        if (!fillAndNotify(order)) {}
    }

    void requestStop() { m_stopFlag = true; }
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
