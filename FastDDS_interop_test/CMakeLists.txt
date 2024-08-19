cmake_minimum_required(VERSION 3.22)

project("secure_shapes")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fastcdr 2 REQUIRED)
find_package(fastrtps 2.12 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP jsoncpp)

include_directories(/usr/include/pcap) # Include directory for pcap.h
include_directories(/usr/include/jsoncpp) # Include directory for jsoncpp

add_library(shape_type Shape.cxx)
target_link_libraries(shape_type fastcdr fastrtps)

add_library(NetboxMessageL_type NetboxMessageL.cxx)
target_link_libraries(NetboxMessageL_type fastcdr fastrtps)


add_library(NetboxMessage_type NetboxMessage.cxx)
target_link_libraries(NetboxMessage_type fastcdr fastrtps)


find_library(JSONCPP_LIB jsoncpp HINTS "/usr/lib" "/usr/lib/x86_64-linux-gnu")

#set(NETBOX_CPP_LIB_PATH "/home/akash/git2/netbox-mirroring/netbox-cpp/build/rust_part")

# Create the secure_shapes executable and link the necessary libraries
add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessagePubSubTypes.cxx NetboxMessageLPubSubTypes.cxx multicaste_test.cpp subscriber_test.cpp main.cpp globals.cpp)
target_link_libraries(secure_shapes fastcdr fastrtps shape_type NetboxMessageL_type NetboxMessage_type pcap ${JSONCPP_LIBRARIES})

