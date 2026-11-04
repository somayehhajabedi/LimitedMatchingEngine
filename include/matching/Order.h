#pragma once

#include <cstdint>
#include <string>

namespace matching {

enum class Side : std::uint8_t { Buy, Sell };
enum class OrderType : std::uint8_t { GFD, IOC };

struct OrderNode {
    std::string id;
    std::int64_t price = 0;
    std::int64_t quantity = 0;
    Side side = Side::Buy;
    OrderNode* prev = nullptr;
    OrderNode* next = nullptr;
    bool inUse = false;
};

} // namespace matching
