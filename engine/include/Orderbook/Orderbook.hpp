#ifndef ORDERBOOK_ORDERBOOK_HPP
#define ORDERBOOK_ORDERBOOK_HPP

#include "Order.hpp"

#include <map>
#include <thread>
#include <queue>

class Orderbook {
    // callbacks


    using callback_order_ty = std::add_const<Order>&;

    // order in queue
    std::function<void(const Order&)> onOrderQueued;
    // first order is the one that was placed, second is the one that was resting
    std::function<void(const Order&, const Order&)> onOrderFill;
    // order that was added to the book
    std::function<void(const Order&)> onOrderAddedToBook;
    // order that was killed
    std::function<void(const Order&)> onOrderKill;

    // price levels and order queue
    std::map<Price, std::deque<Order>, std::less<>> m_asks;
    std::map<Price, std::deque<Order>, std::greater<>> m_bids;
    std::queue<Order> m_orders;

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

    void matchingThread();

    void fillMarketOrder(Order& order);

    void fillLimitOrder(Order& order);

    template<typename Comp>
    auto fillShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void;

    template<typename Comp>
    auto fillLimitShit(Order& order, std::map<Price, std::deque<Order>, Comp>& priceMap) -> void;

    bool processOrder(Order& order, std::deque<Order>& orders) const;

    void requestStop();
};

#endif    // ORDERBOOK_ORDERBOOK_HPP
