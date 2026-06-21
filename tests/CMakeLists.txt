# EC codec unit tests (Qt Test), linking the headless ec-core library.

add_executable(tst_eccodec tst_eccodec.cpp)
target_link_libraries(tst_eccodec PRIVATE ec-core Qt6::Test)
add_test(NAME eccodec COMMAND tst_eccodec)

add_executable(tst_ecauth tst_ecauth.cpp)
target_link_libraries(tst_ecauth PRIVATE ec-core Qt6::Test)
add_test(NAME ecauth COMMAND tst_ecauth)

add_executable(tst_model tst_model.cpp)
target_link_libraries(tst_model PRIVATE ec-core Qt6::Test)
add_test(NAME model COMMAND tst_model)

# Live smoke test against a real amuled. Reads credentials only from the
# environment or ~/.config/rstamule/config.ini; skips when none are available.
add_executable(tst_eclive tst_eclive.cpp)
target_link_libraries(tst_eclive PRIVATE ec-core Qt6::Test)
add_test(NAME eclive COMMAND tst_eclive)
