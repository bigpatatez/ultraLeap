cmake_minimum_required(VERSION 3.10)
project(leapProject C)

include_directories(/home/ubuntu/ultraleap-hand-tracking-service_5.17.1.0-a9f25232-arm64/LeapSDK/include)
link_directories(/home/ubuntu/ultraleap-hand-tracking-service_5.17.1.0-a9f25232-arm64/LeapSDK/lib)

add_executable(example PollingSample.c ExampleConnection.c)
target_link_libraries(example -L/home/ubuntu/ultraleap-hand-tracking-service_5.17.1.0-a9f25232-arm64/LeapSDK/lib -lLeapC)

add_executable(callback CallbackSample.c ExampleConnection.c bram.c)
target_link_libraries(callback -L/home/ubuntu/ultraleap-hand-tracking-service_5.17.1.0-a9f25232-arm64/LeapSDK/lib -lLeapC)

add_executable(wifiTry mainprogram.c ExampleConnection.c bram.c wifi.c)
target_link_libraries(wifiTry -L/home/ubuntu/ultraleap-hand-tracking-service_5.17.1.0-a9f25232-arm64/LeapSDK/lib -lLeapC)
