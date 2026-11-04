#pragma once

#include "matching/Order.h"
#include "matching/OrderPool.h"
#include "matching/PriceLevel.h"
#include "matching/TradeReporter.h"

#include <cstddef>
#include <iosfwd>
#include <map>
#include <string>
#include <unordered_map>

namespace matching {

// Single-threaded engine. The reporter must outlive the book and must not
// re-enter it. Returned order pointers expire on cancellation/full execution.
class OrderBook {
public:
    explicit OrderBook(TradeReporter& reporter,
                       std::size_t expectedActiveOrders = 1024);
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;
    OrderBook(OrderBook&&) = delete;
    OrderBook& operator=(OrderBook&&) = delete;

    void processNewOrder(std::string id, Side side, OrderType type,
                         std::int64_t price, std::int64_t quantity);
    void cancelOrder(const std::string& id);
    void modifyOrder(std::string id, Side side,
                     std::int64_t price, std::int64_t quantity);
    [[nodiscard]] const OrderNode* bestBuy() const noexcept;
    [[nodiscard]] const OrderNode* bestSell() const noexcept;
    void print(std::ostream& output) const;

private:
    using Levels = std::map<std::int64_t, PriceLevel>;
    void addToBook(OrderNode& order);
    void tryMatch(OrderNode& incoming);

    TradeReporter& reporter_;
    OrderPool pool_;
    Levels buyLevels_;
    Levels sellLevels_;
    std::unordered_map<std::string, OrderNode*> orderIndex_;
};

} // namespace matching
