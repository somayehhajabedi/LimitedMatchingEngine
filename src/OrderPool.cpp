#include "matching/OrderPool.h"

#include <utility>

namespace matching {

OrderPool::OrderPool(std::size_t initialCapacity) {
    if (initialCapacity > 0)
        addNodes(initialCapacity);
}

void OrderPool::addNodes(std::size_t count) {
    auto nodes = std::make_unique<OrderNode[]>(count);
    auto* first = nodes.get();
    storage_.push_back(std::move(nodes));
    for (std::size_t i = 0; i < count; ++i) {
        first[i].next = freeNode_;
        freeNode_ = &first[i];
    }
}

OrderNode* OrderPool::acquire() {
    if (!freeNode_)
        addNodes(BlockSize);
    auto* node = freeNode_;
    freeNode_ = node->next;
    node->next = nullptr;
    node->inUse = true;
    return node;
}

bool OrderPool::release(OrderNode& node) noexcept {
    if (!node.inUse)
        return false;
    node.inUse = false;
    node.id.clear();
    node.price = 0;
    node.quantity = 0;
    node.side = Side::Buy;
    node.prev = nullptr;
    node.next = freeNode_;
    freeNode_ = &node;
    return true;
}

} // namespace matching
