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

struct Level {
    Price price;
    Quantity_ty quantity;
};

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
     *
     *
     * matching lowest possible ask and highest bid
     */

    // price levels
    std::map<Price, std::deque<Order>, std::less<>> m_asks;
    std::map<Price, std::deque<Order>, std::greater<>> m_bids;
    std::queue<Order> m_orders;

    // callbacks for notifications
    std::function<void(Order&)> onOrderAddCallback;
    std::function<void(Order&, Order&)> onOrderFillCallback;

    // lock for orders
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
                    handleLimitOrder(order);
                    break;
                }
                case OrderType::INVALID: throw std::runtime_error("invalid order type");
            }
        }
    }

    auto fillAndNotify(Order& order) -> bool {
        switch (order.getSide()) {
            case OrderSide::BUY: {
                // match with lowest ask
                auto bestAsk = m_asks.begin();
                if (bestAsk == m_asks.end()) {
                    return false;
                }

                auto& askOrders = bestAsk->second;
                while (!order.isFilled() && !askOrders.empty()) {
                    auto& askOrder = askOrders.front();

                    order.fill(askOrder);

                    if (askOrder.isFilled()) {
                        askOrders.pop_front();
                    }

                    onOrderFillCallback(order, askOrder);
                }
                break;
            }
            case OrderSide::SELL: {
                // match with highest bid
                auto bestBid = m_bids.begin();
                if (bestBid == m_bids.end()) {
                    return false;
                }

                auto& bidOrders = bestBid->second;
                while (!order.isFilled() && !bidOrders.empty()) {
                    auto& bidOrder = bidOrders.front();

                    order.fill(bidOrder);

                    if (bidOrder.isFilled()) {
                        bidOrders.pop_front();
                    }

                    onOrderFillCallback(order, bidOrder);
                }
                break;
            }
            case OrderSide::INVALID: throw std::runtime_error("invalid order side");
        }

        return true;
    }

    template<typename Compare>
    auto crossOrder(Order& order, std::map<Price, std::deque<Order>, Compare>& book) -> bool {
        // match with highest bid
        auto bestLevel = book.begin();
        if (bestLevel == book.end()) {
            return false;
        }

        auto& restingOrders = bestLevel->second;
        while (!order.isFilled() && !restingOrders.empty()) {
            auto& topOrder = book.front();

            order.fill(topOrder);

            if (topOrder.isFilled()) {
                restingOrders.pop_front();
            }

            onOrderFillCallback(order, topOrder);
        }
        return true;
    }

    auto handleMarketOrder(Order& order) -> void { fillAndNotify(order); }

    auto handleLimitOrder(Order& order) -> void {
        if (!fillAndNotify(order)) {
            // cant use ternary bc of templating
            if (order.getSide() == OrderSide::BUY) {
                addOrderToBook(order, m_bids);
            } else {
                addOrderToBook(order, m_asks);
            }
        }
    }

    template<typename Compare>
    auto addOrderToBook(Order& order, std::map<Price, std::deque<Order>, Compare>& book) -> void {
        auto it = book.find(order.getPrice());
        if (it == book.end()) {
            book[order.getPrice()] = std::deque<Order>{order};
        } else {
            it->second.push_back(order);
        }
    }

    auto requestStop() -> void { m_stopFlag = true; }
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
