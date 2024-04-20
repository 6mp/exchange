#ifndef ORDERBOOK_ORDERBOOK_HPP
#define ORDERBOOK_ORDERBOOK_HPP

#include "Order.hpp"

#include <map>
#include <thread>
#include <queue>

class Orderbook {
    // callbacks
    std::function<void(Order&)> onOrderAdd;
    std::function<void(Order&, Order&)> onOrderFill;
    std::function<void(Order&)> onOrderAddedToBook;
    std::function<void(Order&)> onOrderKill;

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
        decltype(onOrderAdd) onOrderAdd,
        decltype(onOrderFill) onOrderFill,
        decltype(onOrderAddedToBook) onOrderAddedToBook,
        decltype(onOrderKill) onOrderKill);

    ~Orderbook();

    auto addOrder(Order& order) -> void;

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
