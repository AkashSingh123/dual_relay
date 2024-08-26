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


add_library(NetboxMessage1_type NetboxMessage1.cxx)
target_link_libraries(NetboxMessage1_type fastcdr fastrtps)


add_library(NetboxMessage2_type NetboxMessage2.cxx)
target_link_libraries(NetboxMessage2_type fastcdr fastrtps)


#add_library(newmessage_type newmessage.cxx)
#target_link_libraries(newmessage_type fastcdr fastrtps)

find_library(JSONCPP_LIB jsoncpp HINTS "/usr/lib" "/usr/lib/x86_64-linux-gnu")

#set(NETBOX_CPP_LIB_PATH "/home/akash/git2/netbox-mirroring/netbox-cpp/build/rust_part")

# Create the secure_shapes executable and link the necessary libraries
#add_executable(pubsub_reversemessage NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_reversemessage.cpp)
#add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessage1PubSubTypes.cxx NetboxMessage1PubSubTypes test_qos.cpp subscriber_test2.cpp main.cpp globals.cpp)
#add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessagePubSubTypes.cxx fastdds_pub.cpp subscriber_test2.cpp main.cpp globals.cpp)
#add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessagePubSubTypes.cxx sniff_sub.cpp globals.cpp)
#add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_sniff.cpp globals.cpp)

add_executable(pubsub_newmessage ShapePubSubTypes.cxx NetboxMessage1PubSubTypes.cxx NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_newmessage.cpp globals.cpp)
add_executable(secure_shapes ShapePubSubTypes.cxx NetboxMessage2PubSubTypes.cxx  NetboxMessage1PubSubTypes.cxx Frag_test.cpp subscriber_test2.cpp main.cpp globals.cpp)
add_executable(pubsub_reversemessage NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_reversemessage.cpp)
add_executable(pubsub_reversemessage2 NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_reversemessage2.cpp)
add_executable(pubsub_reversemessage3 NetboxMessage1PubSubTypes.cxx NetboxMessage2PubSubTypes.cxx pubsub_reversemessage3.cpp)



target_link_libraries(pubsub_newmessage fastcdr fastrtps shape_type NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})
target_link_libraries(pubsub_reversemessage fastcdr fastrtps shape_type NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})
target_link_libraries(pubsub_reversemessage2 fastcdr fastrtps shape_type NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})
target_link_libraries(pubsub_reversemessage3 fastcdr fastrtps shape_type NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})

target_link_libraries(secure_shapes fastcdr fastrtps shape_type NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})
