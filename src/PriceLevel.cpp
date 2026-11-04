#include "matching/PriceLevel.h"

#include <cassert>
#include <limits>
#include <stdexcept>

namespace matching {

void PriceLevel::append(OrderNode& order) {
    assert(order.quantity > 0);
    assert(order.prev == nullptr && order.next == nullptr);
    assert(head_ != &order);
    if (order.quantity > std::numeric_limits<std::int64_t>::max() - total_)
        throw std::overflow_error("Price-level quantity exceeds int64_t");

    order.prev = tail_;
    if (tail_)
        tail_->next = &order;
    else
        head_ = &order;
    tail_ = &order;
    total_ += order.quantity;
}

void PriceLevel::remove(OrderNode& order) noexcept {
    if (order.prev)
        order.prev->next = order.next;
    else
        head_ = order.next;
    if (order.next)
        order.next->prev = order.prev;
    else
        tail_ = order.prev;
    total_ -= order.quantity;
    order.prev = nullptr;
    order.next = nullptr;
}

void PriceLevel::reduceQuantity(OrderNode& order, std::int64_t quantity) noexcept {
    assert(quantity > 0 && quantity <= order.quantity);
    total_ -= quantity;
    order.quantity -= quantity;
    if (order.quantity == 0)
        remove(order);
}

} // namespace matching
