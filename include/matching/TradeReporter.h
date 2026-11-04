#pragma once

#include <cstdint>
#include <iosfwd>
#include <string_view>

namespace matching {

// IDs are borrowed: reporters must consume them during onTrade(), or copy
// them if they retain events. Exceptions from reporters propagate to callers.
struct Trade {
    std::string_view restingId;
    std::int64_t restingPrice;
    std::string_view incomingId;
    std::int64_t incomingPrice;
    std::int64_t quantity;
};

class TradeReporter {
public:
    virtual ~TradeReporter() = default;
    virtual void onTrade(const Trade& trade) = 0;
};

class TextTradeReporter final : public TradeReporter {
public:
    explicit TextTradeReporter(std::ostream& output) noexcept : output_(output) {}
    void onTrade(const Trade& trade) override;

private:
    std::ostream& output_;
};

} // namespace matching
