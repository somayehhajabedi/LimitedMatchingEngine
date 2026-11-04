#pragma once

#include "matching/Order.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace matching {

// Owns stable-address nodes. Only detached nodes acquired from this pool
// may be released. This class and its clients are single-threaded.
class OrderPool {
public:
    explicit OrderPool(std::size_t initialCapacity = 1024);
    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;
    OrderPool(OrderPool&&) = delete;
    OrderPool& operator=(OrderPool&&) = delete;

    [[nodiscard]] OrderNode* acquire();
    bool release(OrderNode& node) noexcept;

private:
    static constexpr std::size_t BlockSize = 1024;
    void addNodes(std::size_t count);

    std::vector<std::unique_ptr<OrderNode[]>> storage_;
    OrderNode* freeNode_ = nullptr;
};

} // namespace matching
