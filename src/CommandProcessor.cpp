#include "matching/CommandProcessor.h"
#include "matching/OrderBook.h"
#include "matching/TradeReporter.h"

#include <cstdint>
#include <istream>
#include <sstream>
#include <string>
#include <utility>

namespace matching {
namespace {

bool finished(std::istringstream& input) {
    input >> std::ws;
    return input.eof();
}

bool parseSide(const std::string& text, Side& side) {
    if (text == "BUY")
        side = Side::Buy;
    else if (text == "SELL")
        side = Side::Sell;
    else
        return false;
    return true;
}

} // namespace

void processCommands(std::istream& source, std::ostream& output) {
    TextTradeReporter reporter(output);
    OrderBook book(reporter);
    std::string line;
    while (std::getline(source, line)) {
        std::istringstream input(line);
        std::string command;
        if (!(input >> command))
            continue;

        if (command == "BUY" || command == "SELL") {
            std::string typeText;
            std::string id;
            std::int64_t price = 0;
            std::int64_t quantity = 0;
            if (!(input >> typeText >> price >> quantity >> id) || !finished(input))
                continue;
            if (typeText != "GFD" && typeText != "IOC")
                continue;
            const auto side = command == "BUY" ? Side::Buy : Side::Sell;
            const auto type = typeText == "GFD" ? OrderType::GFD : OrderType::IOC;
            book.processNewOrder(std::move(id), side, type, price, quantity);
        } else if (command == "CANCEL") {
            std::string id;
            if ((input >> id) && finished(input))
                book.cancelOrder(id);
        } else if (command == "MODIFY") {
            std::string id;
            std::string sideText;
            std::int64_t price = 0;
            std::int64_t quantity = 0;
            Side side = Side::Buy;
            if (!(input >> id >> sideText >> price >> quantity) || !finished(input))
                continue;
            if (!parseSide(sideText, side))
                continue;
            book.modifyOrder(std::move(id), side, price, quantity);
        } else if (command == "PRINT" && finished(input)) {
            book.print(output);
        }
    }
}

} // namespace matching
