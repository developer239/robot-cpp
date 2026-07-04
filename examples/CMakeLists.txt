# Small runnable programs that double as manual smoke tests. They perform real
# injection, so they are examples rather than automated tests.
add_executable(example_type_text type_text.cpp)
target_link_libraries(example_type_text PRIVATE robot::robot)

add_executable(example_capture_screen capture_screen.cpp)
target_link_libraries(example_capture_screen PRIVATE robot::robot)

add_executable(example_record_replay record_replay.cpp)
target_link_libraries(example_record_replay PRIVATE robot::robot)