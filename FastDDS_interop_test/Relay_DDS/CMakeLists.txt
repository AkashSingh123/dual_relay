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
include_directories(${CMAKE_SOURCE_DIR}/MessageSerialisers) # Include the directory where NetboxMessage*.cxx files are located

# Add the new include directory for Relay_DDS headers
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add libraries for NetboxMessage1 and NetboxMessage2
add_library(NetboxMessage1_type MessageSerialisers/NetboxMessage1.cxx)
target_link_libraries(NetboxMessage1_type fastcdr fastrtps)

add_library(NetboxMessage2_type MessageSerialisers/NetboxMessage2.cxx)
target_link_libraries(NetboxMessage2_type fastcdr fastrtps)

# Link the JSONCPP library
find_library(JSONCPP_LIB jsoncpp HINTS "/usr/lib" "/usr/lib/x86_64-linux-gnu")

# Create executables and link necessary libraries
add_executable(config_sniffer_relay 
    MessageSerialisers/NetboxMessage1PubSubTypes.cxx 
    MessageSerialisers/NetboxMessage2PubSubTypes.cxx 
    src/config_sniffer.cpp 
    src/subscriber_test2.cpp 
    src/main.cpp 
    src/globals.cpp
)
target_link_libraries(config_sniffer_relay fastcdr fastrtps NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})

add_executable(multiple_writer 
    MessageSerialisers/NetboxMessage1PubSubTypes.cxx 
    MessageSerialisers/NetboxMessage2PubSubTypes.cxx 
    src/multiple_writer.cpp 
    src/subscriber_test2.cpp 
    src/main.cpp 
    src/globals.cpp
)
target_link_libraries(multiple_writer fastcdr fastrtps NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})

add_executable(topic_dds_relay 
    MessageSerialisers/NetboxMessage1PubSubTypes.cxx 
    MessageSerialisers/NetboxMessage2PubSubTypes.cxx 
    src/topic_dds_relay.cpp
)
target_link_libraries(topic_dds_relay fastcdr fastrtps NetboxMessage1_type NetboxMessage2_type pcap ${JSONCPP_LIBRARIES})

