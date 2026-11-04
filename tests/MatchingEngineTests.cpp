#include "matching/CommandProcessor.h"
#include "matching/OrderBook.h"
#include "matching/OrderPool.h"
#include "matching/PriceLevel.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int passed = 0;

void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error(message);
}

void scenario(const std::string& name, const std::string& commands,
              const std::string& expected) {
    std::istringstream input(commands);
    std::ostringstream output;
    matching::processCommands(input, output);
    require(output.str() == expected,
            name + "\nExpected:\n" + expected + "Actual:\n" + output.str());
    ++passed;
}

void componentTests() {
    using namespace matching;
    std::ostringstream output;
    TextTradeReporter reporter(output);
    OrderBook book(reporter, 2);
    require(!book.bestBuy() && !book.bestSell(), "Empty best prices");
    book.processNewOrder("s1", Side::Sell, OrderType::GFD, 101, 2);
    require(book.bestSell() && book.bestSell()->id == "s1", "Sell-only bestSell regression");
    book.processNewOrder("s2", Side::Sell, OrderType::GFD, 100, 3);
    book.processNewOrder("b1", Side::Buy, OrderType::GFD, 90, 2);
    book.processNewOrder("b2", Side::Buy, OrderType::GFD, 91, 2);
    require(book.bestSell()->price == 100 && book.bestBuy()->price == 91, "Best prices");
    book.cancelOrder("s2");
    require(book.bestSell()->price == 101, "Best after cancel");
    ++passed;

    OrderPool pool(2);
    std::vector<OrderNode*> nodes;
    for (int i = 0; i < 2050; ++i) {
        auto* node = pool.acquire();
        node->id = std::to_string(i);
        nodes.push_back(node);
    }
    for (int i = 0; i < 2050; ++i)
        require(nodes[i]->id == std::to_string(i), "Stable pool addresses after growth");
    auto* last = nodes.back();
    for (auto* node : nodes)
        require(pool.release(*node), "Pool release");
    require(!pool.release(*last), "Double release guard");
    require(pool.acquire() == last, "Pool LIFO reuse");
    ++passed;

    PriceLevel level;
    OrderNode first;
    OrderNode middle;
    OrderNode lastOrder;
    first.quantity = 3;
    middle.quantity = 4;
    lastOrder.quantity = 5;
    level.append(first);
    level.append(middle);
    level.append(lastOrder);
    level.remove(middle);
    require(level.sum() == 8 && first.next == &lastOrder && lastOrder.prev == &first,
            "Remove middle and aggregate");
    level.reduceQuantity(first, 3);
    require(level.front() == &lastOrder && level.sum() == 5, "Fill head");
    level.remove(lastOrder);
    require(level.empty() && level.sum() == 0, "Remove final node");
    ++passed;

    OrderBook large(reporter);
    large.processNewOrder("large", Side::Buy, OrderType::GFD,
                          1, std::numeric_limits<std::int64_t>::max());
    bool overflow = false;
    try {
        large.processNewOrder("extra", Side::Buy, OrderType::GFD, 1, 1);
    } catch (const std::overflow_error&) {
        overflow = true;
    }
    require(overflow, "Overflow is detected");
    large.cancelOrder("large");
    large.processNewOrder("extra", Side::Buy, OrderType::GFD, 1, 1);
    require(large.bestBuy() && large.bestBuy()->id == "extra", "Overflow rollback keeps index valid");
    ++passed;
}
} // namespace

int main() {
    try {
        scenario("Example 1", "BUY GFD 1000 10 order1\nPRINT\n", "SELL:\nBUY:\n1000 10\n");
        scenario("Example 2", "BUY GFD 1000 10 order1\nBUY GFD 1000 20 order2\nPRINT\n",
                 "SELL:\nBUY:\n1000 30\n");
        scenario("Example 3", "BUY GFD 1000 10 order1\nBUY GFD 1001 20 order2\nPRINT\n",
                 "SELL:\nBUY:\n1001 20\n1000 10\n");
        scenario("Example 4", "BUY GFD 1000 10 order1\nSELL GFD 900 20 order2\nPRINT\n",
                 "TRADE order1 1000 10 order2 900 10\nSELL:\n900 10\nBUY:\n");
        scenario("Example 5", "BUY GFD 1000 10 ORDER1\nBUY GFD 1010 10 ORDER2\nSELL GFD 1000 15 ORDER3\n",
                 "TRADE ORDER2 1010 10 ORDER3 1000 10\nTRADE ORDER1 1000 5 ORDER3 1000 5\n");
        scenario("Sell FIFO", "SELL GFD 100 5 s1\nSELL GFD 100 7 s2\nBUY GFD 100 8 b1\nPRINT\n",
                 "TRADE s1 100 5 b1 100 5\nTRADE s2 100 3 b1 100 3\nSELL:\n100 4\nBUY:\n");
        scenario("Modify priority", "BUY GFD 1000 10 a\nBUY GFD 1000 10 b\nMODIFY a BUY 1000 20\nSELL GFD 900 20 c\n",
                 "TRADE b 1000 10 c 900 10\nTRADE a 1000 10 c 900 10\n");
        scenario("IOC partial", "SELL GFD 100 2 s\nBUY IOC 100 5 b\nPRINT\n",
                 "TRADE s 100 2 b 100 2\nSELL:\nBUY:\n");
        scenario("IOC unmatched", "BUY IOC 100 5 b\nMODIFY b BUY 90 1\nPRINT\n", "SELL:\nBUY:\n");
        scenario("Side change", "BUY GFD 100 5 b\nMODIFY b SELL 100 5\nPRINT\n", "SELL:\n100 5\nBUY:\n");
        scenario("Missing IDs", "CANCEL missing\nMODIFY missing SELL 100 2\nPRINT\n", "SELL:\nBUY:\n");
        scenario("Duplicate active ID", "BUY GFD 100 5 a\nSELL IOC 90 10 a\nPRINT\n", "SELL:\nBUY:\n100 5\n");
        scenario("Reuse cancelled ID", "BUY GFD 100 5 a\nCANCEL a\nSELL GFD 101 2 a\nPRINT\n", "SELL:\n101 2\nBUY:\n");
        scenario("Invalid input", "BUY GFD 0 2 a\nSELL GFD 1 -2 b\nBUY GFD 1 0 c\nBUY GFD 1 2\nPRINT\n",
                 "SELL:\nBUY:\n");
        scenario("Invalid modify", "BUY GFD 100 5 a\nMODIFY a SELL 0 3\nPRINT\n", "SELL:\nBUY:\n100 5\n");
        scenario("Strict columns", "BUY GFD 100 2 a extra\nPRINT extra\nPRINT\n", "SELL:\nBUY:\n");
        scenario("Descending both sides", "SELL GFD 110 2 a\nSELL GFD 120 3 b\nBUY GFD 90 4 c\nBUY GFD 80 5 d\nPRINT\n",
                 "SELL:\n120 3\n110 2\nBUY:\n90 4\n80 5\n");
        componentTests();
        std::cout << passed << " checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
