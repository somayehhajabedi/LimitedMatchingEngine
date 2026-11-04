cmake_minimum_required(VERSION 3.16)
project(MatchingEngine VERSION 1.0.0 LANGUAGES CXX)

option(MATCHING_ENABLE_SANITIZERS "Enable address and undefined-behavior checks" OFF)

add_library(matching_engine
    src/OrderPool.cpp
    src/PriceLevel.cpp
    src/TradeReporter.cpp
    src/OrderBook.cpp
    src/CommandProcessor.cpp
)
target_include_directories(matching_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(matching_engine PUBLIC cxx_std_20)
set_target_properties(matching_engine PROPERTIES CXX_EXTENSIONS OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(matching_engine PRIVATE -Wall -Wextra -Wpedantic)
    if(MATCHING_ENABLE_SANITIZERS)
        target_compile_options(matching_engine PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(matching_engine PUBLIC -fsanitize=address,undefined)
    endif()
endif()

add_executable(matching-engine src/main.cpp)
target_link_libraries(matching-engine PRIVATE matching_engine)

include(CTest)
if(BUILD_TESTING)
    add_executable(matching-engine-tests tests/MatchingEngineTests.cpp)
    target_link_libraries(matching-engine-tests PRIVATE matching_engine)
    add_test(NAME matching-engine-tests COMMAND matching-engine-tests)
endif()
