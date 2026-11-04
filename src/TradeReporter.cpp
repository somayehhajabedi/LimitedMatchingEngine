#include "matching/TradeReporter.h"

#include <ostream>

namespace matching {

void TextTradeReporter::onTrade(const Trade& trade) {
    output_ << "TRADE " << trade.restingId << ' '
            << trade.restingPrice << ' ' << trade.quantity << ' '
            << trade.incomingId << ' ' << trade.incomingPrice << ' '
            << trade.quantity << '\n';
}

} // namespace matching
