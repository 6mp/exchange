#ifndef ORDERBOOK_ORDERBOOK_HPP
#define ORDERBOOK_ORDERBOOK_HPP

// interesting
// https://medium.com/@amitava.webwork/designing-low-latency-high-performance-order-matching-engine-a07bd58594f4

#include "Order.hpp"

#include <map>
#include <thread>
#include <queue>

class Orderbook {
    // callbacks
    // order in queue
    std::function<void(const Order&)> onOrderQueued;
    // first order is the one that was placed, second is the one that was resting
    std::function<void(const Order&, const Order&)> onOrderFill;
    // order that was added to the book
    std::function<void(const Order&)> onOrderAddedToBook;
    // order that was killed
    std::function<void(const Order&)> onOrderKill;
    // order that was deleted
    std::function<void(const Order&)> onOrderDeleted;

    // price levels and order queue

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
    std::map<Price, std::deque<Order>, std::less<>> m_asks;
    std::map<Price, std::deque<Order>, std::greater<>> m_bids;
    std::vector<Order> m_orders;

    // multithreading
    std::mutex m_lock;
    std::condition_variable m_condVar;
    std::atomic<bool> m_stopFlag{false};
    std::thread m_matchingThread;

public:
    Orderbook(
        decltype(onOrderQueued) onOrderAdd,
        decltype(onOrderFill) onOrderFill,
        decltype(onOrderAddedToBook) onOrderAddedToBook,
        decltype(onOrderKill) onOrderKill);

    ~Orderbook();

    auto addOrder(const Order& order) -> void;

    auto removeOrder(const Order& order) -> void;

    void matchingThread();

    void fillMarketOrder(Order& order);

    void fillLimitOrder(Order& order);

    template<typename Comp>
    auto matchMarketOrder(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void;

    template<typename Comp>
    auto matchLimitOrder(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void;

    bool processOrder(Order& order, std::deque<Order>& orders) const;

    void requestStop();
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
