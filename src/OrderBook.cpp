#include "matching/OrderBook.h"

#include <algorithm>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace matching {

OrderBook::OrderBook(TradeReporter& reporter, std::size_t expectedActiveOrders)
    : reporter_(reporter), pool_(expectedActiveOrders) {
    orderIndex_.reserve(expectedActiveOrders);
}

void OrderBook::addToBook(OrderNode& order) {
    auto& levels = order.side == Side::Buy ? buyLevels_ : sellLevels_;
    auto [levelIt, created] = levels.try_emplace(order.price);
    // Roll back registration if allocation or quantity aggregation fails.
    // The caller retains ownership of the acquired node until this succeeds.
    try {
        auto [indexIt, inserted] = orderIndex_.emplace(order.id, &order);
        if (!inserted)
            throw std::logic_error("Duplicate active order ID");
        try {
            levelIt->second.append(order);
        } catch (...) {
            orderIndex_.erase(indexIt);
            throw;
        }
    } catch (...) {
        if (created)
            levels.erase(levelIt);
        throw;
    }
}

void OrderBook::tryMatch(OrderNode& incoming) {
    const bool isBuy = incoming.side == Side::Buy;
    auto& levels = isBuy ? sellLevels_ : buyLevels_;
    while (!levels.empty() && incoming.quantity > 0) {
        auto best = isBuy ? levels.begin() : std::prev(levels.end());
        const auto price = best->first;
        if ((isBuy && price > incoming.price) || (!isBuy && price < incoming.price))
            break;

        auto& level = best->second;
        auto* resting = level.front();
        const auto quantity = std::min(resting->quantity, incoming.quantity);
        reporter_.onTrade({resting->id, resting->price,
                           incoming.id, incoming.price, quantity});

        level.reduceQuantity(*resting, quantity);
        incoming.quantity -= quantity;
        if (resting->quantity == 0) {
            orderIndex_.erase(resting->id);
            pool_.release(*resting);
        }
        if (level.empty())
            levels.erase(best);
    }
}

void OrderBook::processNewOrder(std::string id, Side side, OrderType type,
                                std::int64_t price, std::int64_t quantity) {
    if (id.empty() || price <= 0 || quantity <= 0 || orderIndex_.find(id) != orderIndex_.end())
        return;

    OrderNode incoming;
    incoming.id = std::move(id);
    incoming.price = price;
    incoming.quantity = quantity;
    incoming.side = side;
    tryMatch(incoming);
    if (incoming.quantity == 0 || type == OrderType::IOC)
        return;

    auto* node = pool_.acquire();
    try {
        node->id = std::move(incoming.id);
        node->price = incoming.price;
        node->quantity = incoming.quantity;
        node->side = incoming.side;
        addToBook(*node);
    } catch (...) {
        pool_.release(*node);
        throw;
    }
}

void OrderBook::cancelOrder(const std::string& id) {
    const auto it = orderIndex_.find(id);
    if (it == orderIndex_.end())
        return;
    auto* order = it->second;
    auto& levels = order->side == Side::Buy ? buyLevels_ : sellLevels_;
    const auto levelIt = levels.find(order->price);
    levelIt->second.remove(*order);
    if (levelIt->second.empty())
        levels.erase(levelIt);
    orderIndex_.erase(it);
    pool_.release(*order);
}

void OrderBook::modifyOrder(std::string id, Side side,
                            std::int64_t price, std::int64_t quantity) {
    if (id.empty() || price <= 0 || quantity <= 0 || orderIndex_.find(id) == orderIndex_.end())
        return;
    cancelOrder(id);
    processNewOrder(std::move(id), side, OrderType::GFD, price, quantity);
}

const OrderNode* OrderBook::bestBuy() const noexcept {
    return buyLevels_.empty() ? nullptr : buyLevels_.rbegin()->second.front();
}

const OrderNode* OrderBook::bestSell() const noexcept {
    return sellLevels_.empty() ? nullptr : sellLevels_.begin()->second.front();
}

void OrderBook::print(std::ostream& output) const {
    output << "SELL:\n";
    for (auto it = sellLevels_.rbegin(); it != sellLevels_.rend(); ++it)
        output << it->first << ' ' << it->second.sum() << '\n';
    output << "BUY:\n";
    for (auto it = buyLevels_.rbegin(); it != buyLevels_.rend(); ++it)
        output << it->first << ' ' << it->second.sum() << '\n';
}

} // namespace matching
