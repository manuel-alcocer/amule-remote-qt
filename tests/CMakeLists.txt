# EC codec unit tests (Qt Test), linking the headless ec-core library.

add_executable(tst_eccodec tst_eccodec.cpp)
target_link_libraries(tst_eccodec PRIVATE ec-core Qt6::Test)

add_test(NAME eccodec COMMAND tst_eccodec)
