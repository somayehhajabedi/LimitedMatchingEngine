#include "matching/CommandProcessor.h"

#include <exception>
#include <iostream>

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    try {
        matching::processCommands(std::cin, std::cout);
    } catch (const std::exception& error) {
        std::cerr << "matching-engine: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
