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

    // limit order added to book callback
    std::function<void(Order&)> limitOrderAddCallback;

    // market order failed to fill callback, (low liquidity)
    std::function<void(Order&)> marketOrderFailCallback;

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
                    fillMarketOrder(order);
                    break;
                }
                case OrderType::LIMIT: {
                    // limit orders can be done on a separate thread
                    std::thread j{[this, &order] { fillLimitOrder(order); }};
                    j.detach();
                    break;
                }
                case OrderType::INVALID: throw std::runtime_error("invalid order type");
            }
        }
    }

    auto fillMarketOrder(Order& order) -> void {
        switch (order.getSide()) {
            // execute best price so top of bids or bottom of asks
            case OrderSide::BUY: {
                auto currentLevel = m_asks.begin();
                while (!order.isFilled() && currentLevel != m_asks.end()) {
                    auto& restingOrders = currentLevel->second;
                    processOrder(order, restingOrders);
                    ++currentLevel;
                }

                marketOrderFailCallback(order);
                break;
            }
            case OrderSide::SELL: {
                auto currentLevel = m_bids.begin();
                while (!order.isFilled() && currentLevel != m_bids.end()) {
                    auto& restingOrders = currentLevel->second;
                    processOrder(order, restingOrders);
                    ++currentLevel;
                }

                marketOrderFailCallback(order);
                break;
            }

            case OrderSide::INVALID: break;
        }
    }

    auto fillLimitOrder(Order& order) -> void {
        switch (order.getSide()) {
            case OrderSide::BUY: {
                // find first level where prices >= order price
                auto matchingLevel = std::find_if(m_asks.begin(), m_asks.end(), [&order](const auto& first) {
                    return first.first >= order.getPrice();
                });

                // if its the end this isnt getting matched so push it into the book
                if (matchingLevel == m_asks.end()) {
                    m_bids[order.getPrice()].push_back(order);
                    limitOrderAddCallback(order);
                }

                // otherwise while order is not filled keep pushing or push onto book when no more orders
                auto& [levelPrice, levelOrders] = *matchingLevel;
                while (!order.isFilled()) {
                    if (levelPrice < order.getPrice()) {
                        // push onto book and exit
                        m_bids[order.getPrice()].push_back(order);
                        limitOrderAddCallback(order);
                        return;
                    }

                    processOrder(order, levelOrders);
                    ++matchingLevel;
                }

                break;
            }
            case OrderSide::SELL: {
                // find first level where prices <= order price
                auto matchingLevel = std::find_if(m_bids.begin(), m_bids.end(), [&order](const auto& first) {
                    return first.first <= order.getPrice();
                });

                // if its the end this isnt getting matched so push it into the book
                if (matchingLevel == m_bids.end()) {
                    m_asks[order.getPrice()].push_back(order);
                    limitOrderAddCallback(order);
                }

                // otherwise while order is not filled keep pushing or push onto book when no more orders
                auto& [levelPrice, levelOrders] = *matchingLevel;
                while (!order.isFilled()) {
                    if (levelPrice > order.getPrice()) {
                        // push onto book and exit
                        m_asks[order.getPrice()].push_back(order);
                        limitOrderAddCallback(order;
                        return;
                    }

                    processOrder(order, levelOrders);
                    ++matchingLevel;
                }

                break;
            }

            case OrderSide::INVALID: break;
        }
    }

    bool processOrder(Order& order, std::deque<Order>& orders) {
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

    void requestStop() { m_stopFlag = true; }
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
