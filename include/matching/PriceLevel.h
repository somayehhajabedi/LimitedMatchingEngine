#pragma once

#include "matching/Order.h"

#include <cstdint>

namespace matching {

// Intrusive FIFO; does not own nodes. append requires a detached, positive
// order. remove/reduceQuantity require an order belonging to this level.
class PriceLevel {
public:
    PriceLevel() = default;
    PriceLevel(const PriceLevel&) = delete;
    PriceLevel& operator=(const PriceLevel&) = delete;
    PriceLevel(PriceLevel&&) = delete;
    PriceLevel& operator=(PriceLevel&&) = delete;

    [[nodiscard]] bool empty() const noexcept { return head_ == nullptr; }
    [[nodiscard]] OrderNode* front() noexcept { return head_; }
    [[nodiscard]] const OrderNode* front() const noexcept { return head_; }
    [[nodiscard]] std::int64_t sum() const noexcept { return total_; }

    void append(OrderNode& order);
    void remove(OrderNode& order) noexcept;
    void reduceQuantity(OrderNode& order, std::int64_t quantity) noexcept;

private:
    OrderNode* head_ = nullptr;
    OrderNode* tail_ = nullptr;
    std::int64_t total_ = 0;
};

} // namespace matching
