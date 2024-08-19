#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ShapePublisher.hpp"
#include "NetboxMessagePubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <csignal>
#include <atomic>
#include <thread>
#include <algorithm>
#include <pcap.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <vector>
#include <memory>
#include <array>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm> // For std::search
#include <cmath>     // For std::round
#include <pcap.h>
#include <mutex>
#include "globals.hpp"
#include <json/json.h> // Include the JSON library header

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ShapePublisher.hpp"
#include "NetboxMessagePubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <csignal>
#include <atomic>
#include <thread>
#include <algorithm>
#include <pcap.h>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <iomanip>
#include <sstream>
#include <json/json.h> // Include the JSON library header
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/rtps/attributes/PropertyPolicy.h>
#include <iostream>
#include <csignal>
#include <cstdio>
#include <sstream>
#include <vector>
#include <iomanip>
#include "globals.hpp"
#include "NetboxMessageLPubSubTypes.h"
#include "NetboxMessagePubSubTypes.h"
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <unistd.h>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <memory>
#include <array>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm> // For std::search
#include <cmath>     // For std::round
#include <pcap.h>
#include <mutex>
#include "globals.hpp"
#include <json/json.h> // Include the JSON library header
#include <chrono>
#include "bridge.h"
#include <fastdds/rtps/attributes/RTPSParticipantAttributes.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv6TransportDescriptor.h>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/PublisherAttributes.h>


using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace eprosima::fastdds::dds;
using namespace eprosima::fastrtps::rtps;

std::atomic<bool> stop_capture(false);
std::mutex mtx;
std::condition_variable cv;

std::ofstream log_file("rtps_log.txt", std::ios::out | std::ios::app);
const std::string hex_command = "63 6f 6d 6d 61 6e 64"; // "command"
const std::string hex_zones = "7a 6f 6e 65 73"; // "zones"
const std::string hex_swamp_gcs = "73 77 61 6d 70 5f 67 63 73"; // "swamp_gcs"
const std::string hex_target_update = "74 61 72 67 65 74 5f 75 70 64 61 74 65"; // "target_update"

//std::vector<Config> ShapePublisher::configs; // Define the static member

ShapePublisher::ShapePublisher() : type_(new NetboxMessagePubSubType()), listener_()
{
    load_configs_from_file("config.json");
}

ShapePublisher::~ShapePublisher() {
    for (auto& pub : participants_data) {
        if (pub.publisher && pub.participant) { pub.participant->delete_publisher(pub.publisher); }
        if (pub.participant) { DomainParticipantFactory::get_instance()->delete_participant(pub.participant); }
    }
}

void ShapePublisher::load_configs_from_file(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        throw std::runtime_error("Unable to open config file");
    }

    Json::Value root;
    file >> root;

    for (const auto& config : root["configs"])
    {
        Config conf;
        conf.source_domain_id = config["source_domain_id"].asInt();
        conf.source_destination_ip = config["source_destination_ip"].asString();
        conf.updated_domain_id = config["updated_domain_id"].asInt();;
        conf.updated_destination_ip = config["updated_destination_ip"].asString();

       // std::cout << "source_domain_id: " << conf.source_domain_id << std::endl;
       // std::cout << "source_destination_ip: " << conf.source_destination_ip << std::endl;
       // std::cout << "updated_domain_id: " << conf.updated_domain_id << std::endl;
       // std::cout << "updated_destination_ip: " << conf.updated_destination_ip << std::endl;

        configs.push_back(conf);
    }
}


namespace publisher_stop {
    volatile sig_atomic_t stop;
    void handle_interrupt(int) {
        std::cout << "Stopping capture..." << std::endl;
        stop = 1;
        stop_capture = true;

    }
}


void sigint_handler(int sig) {
    stop_capture = true;
    cv.notify_all(); // Notify all waiting threads
}


void ShapePublisher::print_all_participants_data() const
{
    std::cout << "All ParticipantData entries:" << std::endl;
    for (const auto& participant_data : participants_data)
    {
        std::cout << "ParticipantData:" << std::endl;
        std::cout << "  participant: " << participant_data.participant->get_qos().name() << std::endl;
        std::cout << "  domain_id: " << participant_data.participant->get_domain_id() << std::endl;
        std::cout << "  publisher: " << (participant_data.publisher ? "created" : "not created") << std::endl;
        std::cout << "  topic: " << participant_data.topic->get_name() << std::endl;
        std::cout << "  Number of writers: " << participant_data.writers_data.size() << std::endl;
        for (const auto& writer_data : participant_data.writers_data)
        {
            std::cout << "    WriterData:" << std::endl;
            std::cout << "      source_destination_ip: " << writer_data.source_destination_ip << std::endl;
            std::cout << "      updated_destination_ip: " << writer_data.updated_destination_ip << std::endl;
        }
    }
}




bool ShapePublisher::init(bool with_security)
{
    for (const auto& config : configs)
    {
        // Create and configure DomainParticipant
        DomainParticipantQos participant_qos;
        std::string participant_name = "publisher_participant_" + std::to_string(config.updated_domain_id);
        participant_qos.name(participant_name.c_str());

        if (with_security)
        {
            // Add security settings here if necessary
        }

        // Configure transport descriptor to enable multicast
        auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
        udp_transport->non_blocking_send = true;
        udp_transport->sendBufferSize = 65536;
        udp_transport->receiveBufferSize = 65536;
        participant_qos.transport().user_transports.push_back(udp_transport);
        participant_qos.transport().use_builtin_transports = false;


        //participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.clear();

        // Add Multicast Locator
        eprosima::fastrtps::rtps::Locator_t multicast_locator;
        multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, config.updated_destination_ip);
        multicast_locator.port = 7900;

        participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(multicast_locator);

        // Clear the built-in multicast list
        participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.clear();

        // Create DomainParticipant
        ParticipantData participant_data;
        participant_data.participant = DomainParticipantFactory::get_instance()->create_participant(config.updated_domain_id, participant_qos);
        if (!participant_data.participant)
        {
            std::cerr << "Failed to create participant for domain ID: " << config.updated_domain_id << std::endl;
            return false;
        }

        type_.register_type(participant_data.participant);

        // Create Publisher
        PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
        participant_data.publisher = participant_data.participant->create_publisher(publisher_qos);
        if (!participant_data.publisher)
        {
            std::cerr << "Failed to create publisher for domain ID: " << config.updated_domain_id << std::endl;
            return false;
        }

        // Create Topic
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        participant_data.topic = participant_data.participant->create_topic("pos", type_.get_type_name(), topic_qos);
        if (!participant_data.topic)
        {
            std::cerr << "Failed to create topic for domain ID: " << config.updated_domain_id << std::endl;
            return false;
        }

        // Configure DataWriter QoS
        DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
        datawriter_qos.endpoint().unicast_locator_list.clear();
        datawriter_qos.endpoint().multicast_locator_list.clear();
        datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // Set to reliable reliability
        //datawriter_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS; // Set to reliable reliability

        datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;

        datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);
        //participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.clear();

        // Create DataWriter
        WriterData writer_data;
        writer_data.writer = participant_data.publisher->create_datawriter(participant_data.topic, datawriter_qos, &listener_);
        writer_data.source_destination_ip = config.source_destination_ip;
        writer_data.updated_destination_ip = config.updated_destination_ip;

        if (!writer_data.writer)
        {
            std::cerr << "Failed to create data writer for domain ID: " << config.updated_domain_id << std::endl;
            return false;
        }

        participant_data.writers_data.push_back(writer_data);
        participants_data.push_back(participant_data);

        // Debugging output
        std::cout << "Participant Created: " << participant_name << std::endl;
        std::cout << "  Domain ID: " << config.updated_domain_id << std::endl;
        std::cout << "  Multicast Locator: " << IPLocator::ip_to_string(multicast_locator) << ":" << multicast_locator.port << std::endl;
        std::cout << "  DataWriter for source IP: " << config.source_destination_ip << " to multicast IP: " << config.updated_destination_ip << std::endl;

    }

    return true;
}





/*
bool ShapePublisher::init(bool with_security)
{       // DomainParticipantQos participant_qos;

    for (const auto& config : configs)
    {   DomainParticipantQos participant_qos;
        // Create and configure DomainParticipant
        std::string participant_name = "publisher_participant_" + std::to_string(config.updated_domain_id);
        participant_qos.name(participant_name.c_str());

        if (with_security)
        {
            // Add security settings here if necessary
        }

        // Configure transport descriptor to enable multicast
        auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
        udp_transport->non_blocking_send = true;
        udp_transport->sendBufferSize = 65536;
        udp_transport->receiveBufferSize = 65536;
        participant_qos.transport().user_transports.push_back(udp_transport);
        participant_qos.transport().use_builtin_transports = false;

        // Clear the built-in multicast list
        //participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.clear();

        // Create DomainParticipant
        ParticipantData participant_data;
        participant_data.participant = DomainParticipantFactory::get_instance()->create_participant(config.updated_domain_id, participant_qos);
        if (!participant_data.participant)
        {
            return false;
        }

        type_.register_type(participant_data.participant);



        // Create Publisher
        PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
        participant_data.publisher = participant_data.participant->create_publisher(publisher_qos);
        if (!participant_data.publisher)
        {
            return false;
        }

        // Create Topic
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        participant_data.topic = participant_data.participant->create_topic("pos", type_.get_type_name(), topic_qos);
        if (!participant_data.topic)
        {
            return false;
        }
        DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
        datawriter_qos.endpoint().unicast_locator_list.clear();
        datawriter_qos.endpoint().multicast_locator_list.clear();
        //datawriter_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        //datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;

        // Configure DataWriter QoS
        datawriter_qos.endpoint().unicast_locator_list.clear();
        datawriter_qos.endpoint().multicast_locator_list.clear();
        datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // Set to reliable reliability

        // Add Multicast Locator
        eprosima::fastrtps::rtps::Locator_t multicast_locator;
        multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, config.updated_destination_ip);
        multicast_locator.port = 7900;

        participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(multicast_locator);
        datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);

        // Create DataWriter
        WriterData writer_data;
        writer_data.writer = participant_data.publisher->create_datawriter(participant_data.topic, datawriter_qos, &listener_);
        writer_data.source_destination_ip = config.source_destination_ip;
        writer_data.updated_destination_ip = config.updated_destination_ip;

        participant_data.writers_data.push_back(writer_data);
        participants_data.push_back(participant_data);

        std::cout << "Multicast Locators for updated_destination_ip: " << config.updated_destination_ip << std::endl;
    }

    return true;
}

*/


/*
bool ShapePublisher::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("publisher_participant");

    if (with_security)
    {
        // Add security settings here if necessary
    }
        std::vector<eprosima::fastrtps::rtps::Locator_t> all_multicast_locators;
        //datawriter_qos.history().depth = 1;
    for (const auto& config : configs)
    {
       // std::cout << "Processing Config: " << std::endl;
       // std::cout << "source_domain_id: " << config.source_domain_id << std::endl;
       // std::cout << "source_destination_ip: " << config.source_destination_ip << std::endl;
       // std::cout << "updated_domain_id: " << config.updated_domain_id << std::endl;
       // std::cout << "updated_destination_ip: " << config.updated_destination_ip << std::endl;

        ParticipantData participant_data;
        std::string participant_name = "publisher_participant_" + std::to_string(config.updated_domain_id);
        participant_qos.name(participant_name.c_str());



        participant_data.participant = DomainParticipantFactory::get_instance()->create_participant(config.updated_domain_id, participant_qos);
        if (!participant_data.participant)
        {
            return false;
        }

        type_.register_type(participant_data.participant);


        // Configure transport descriptor to enable multicast
        auto udp_transport = std::make_shared<UDPv4TransportDescriptor>();
        udp_transport->non_blocking_send = true;
        udp_transport->sendBufferSize = 65536;
        udp_transport->receiveBufferSize = 65536;
        participant_qos.transport().user_transports.push_back(udp_transport);
        participant_qos.transport().use_builtin_transports = false;






        PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
        participant_data.publisher = participant_data.participant->create_publisher(publisher_qos);
        if (!participant_data.publisher)
        {
            return false;
        }
        
        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        participant_data.topic = participant_data.participant->create_topic("pos", type_.get_type_name(), topic_qos);
        if (!participant_data.topic)
        {
            return false;
        }
        DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
        datawriter_qos.endpoint().unicast_locator_list.clear();
        datawriter_qos.endpoint().multicast_locator_list.clear();
        datawriter_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        //datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;

        datawriter_qos.endpoint().unicast_locator_list.clear();
        datawriter_qos.endpoint().multicast_locator_list.clear();



        datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // Set to reliable reliability

        //datawriter_qos.resource_limits().max_samples = 100000000;
        //datawriter_qos.resource_limits().allocated_samples = 10000;
        //datawriter_qos.resource_limits().max_samples_per_instance = 1000;
        //datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;

        //datawriter_qos.publish_mode().kind = eprosima::fastrtps::ASYNCHRONOUS_PUBLISH_MODE;
        //datawriter_qos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        eprosima::fastrtps::rtps::Locator_t multicast_locator;
        multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, config.updated_destination_ip);
        multicast_locator.port = 7900;
        participant_qos.wire_protocol().builtin.metatrafficMulticastLocatorList.push_back(multicast_locator);

        datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);
        std::cout << "Multicast Locators for updated_destination_ip: " << config.updated_destination_ip << std::endl;

      /*  eprosima::fastrtps::rtps::Locator_t unicast_locator;
        unicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, config.updated_destination_ip);
        unicast_locator.port = 7900; // Set the appropriate port for unicast communication
        datawriter_qos.endpoint().unicast_locator_list.push_back(unicast_locator);*/



      //  all_multicast_locators.push_back(multicast_locator);

     /*  WriterData writer_data;
        writer_data.writer = participant_data.publisher->create_datawriter(participant_data.topic, datawriter_qos, &listener_);
        writer_data.source_destination_ip = config.source_destination_ip;
        writer_data.updated_destination_ip = config.updated_destination_ip;

        participant_data.writers_data.push_back(writer_data);

        
        participants_data.push_back(participant_data);


    }
   /* std::cout << "All Multicast Locators:" << std::endl;
    for (const auto& locator : datawriter_qos.endpoint().multicast_locator_list)
    {
        std::cout << "  Locator kind: " << locator.kind << std::endl;
        std::cout << "  Locator address: " << IPLocator::ip_to_string(locator) << std::endl;
        std::cout << "  Locator port: " << locator.port << std::endl;
    }
*/

 /*   return true;
  //  print_all_participants_data();

}
*/

std::vector<uint8_t> hex_to_bytes(const std::string& hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 3) { // skip the colons
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

std::string bytesToHexString(const u_char* bytes, int length) {
    std::ostringstream oss;
    for (int i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
    }
    return oss.str();
}

std::string bytesToIpString(const u_char* bytes) {
    std::ostringstream oss;
    oss << (int)bytes[0] << "." << (int)bytes[1] << "." << (int)bytes[2] << "." << (int)bytes[3];
    return oss.str();
}

uint16_t bytesToUint16(const u_char* bytes) {
    return (bytes[0] << 8) | bytes[1];
}

int calculateDomainId(uint16_t port) {
    double domainId = (port - 7400) / 250.0;
    return std::round(domainId);
}


void capture_rtps() {
    char errorBuffer[PCAP_ERRBUF_SIZE];
    pcap_if_t* interfaces;
    pcap_if_t* device;
    pcap_t* handle;

    // Find available devices
    if (pcap_findalldevs(&interfaces, errorBuffer) == -1) {
        std::cerr << "Error finding devices: " << errorBuffer << std::endl;
        return;
    }

    // Select the first device
    device = interfaces;
    if (!device) {
        std::cerr << "No devices found" << std::endl;
        return;
    }

    int snaplen = 65535; // Maximum capture length
    int promisc = 1;     // Promiscuous mode
    int to_ms = 100;       // Read timeout in milliseconds

    // Open the selected device for packet capture
    handle = pcap_open_live(device->name, snaplen, 1, 1, errorBuffer);
    if (!handle) {
        std::cerr << "Could not open device: " << device->name << ": " << errorBuffer << std::endl;
        return;
    }

    std::cout << "Using device: " << device->name << std::endl;

    // Set a filter to capture only RTPS packets
    struct bpf_program fp;
    char filterExp[] = "udp";
    if (pcap_compile(handle, &fp, filterExp, 0, PCAP_NETMASK_UNKNOWN) == -1) {
        std::cerr << "Could not parse filter: " << pcap_geterr(handle) << std::endl;
        return;
    }

    if (pcap_setfilter(handle, &fp) == -1) {
        std::cerr << "Could not install filter: " << pcap_geterr(handle) << std::endl;
        return;
    }

    auto packetHandler = [](u_char* userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
        // RTPS magic header bytes
        const u_char rtps_magic[] = {0x52, 0x54, 0x50, 0x53};

        // Search for the RTPS magic header
        const u_char* magic = std::search(packet, packet + pkthdr->len, std::begin(rtps_magic), std::end(rtps_magic));
        if (magic == packet + pkthdr->len) {
            return; // RTPS magic header not found
        }

        // Check if the submessage ID is 0x15
        if (magic + 24 <= packet + pkthdr->len && magic[20] == 0x15) {

            // Check encapsulation kind
            if (magic + 48 <= packet + pkthdr->len) {
                uint16_t encapsulation_kind = (magic[44] << 8) | magic[45];
                if (encapsulation_kind != 0x0001) {
                    return; // Skip packets with encapsulation kind other than 0x0001
                }
            }

            size_t pos = 0;
            size_t last_topic_end = 0;

            bool found_topic = false;
            size_t payload_start_pos = 48+50; // Position after the RTPS header
            size_t payload_length = pkthdr->len - payload_start_pos - 3; // Adjust for truncation

            if (payload_start_pos < pkthdr->len) {
                std::vector<uint8_t> payload(packet + payload_start_pos, packet + pkthdr->len -32);
                std::cout << "Payload segment: ";
                for (const auto& byte : payload) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                }
                std::cout << std::endl;

                std::string source_ipcheck = bytesToIpString(magic - 16);
                std::string destination_ipcheck = bytesToIpString(magic - 12);

                // Update destination_ip based on the configuration
                for (const auto& config : configs) {
                    if (destination_ipcheck == config.source_destination_ip) {
                        destination_ip = config.updated_destination_ip;
                        std::cout << "Updated Domain ID: " << config.updated_domain_id << std::endl;
                        break;
                    }
                }

                uint16_t destPort = bytesToUint16(magic - 6);
                int calculatedDomainId = calculateDomainId(destPort);

                std::cout << "Destination port: " << destPort << std::endl;
                std::cout << "Domain ID: " << calculatedDomainId << std::endl;
                std::cout << "Destination IP: " << destination_ip << std::endl;

                // Set the payload in the sample 
             //if (payload.size() < 300) {
                   std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
                 NetboxMessage sample;
                 sample.id(2);
                 sample.data().clear();

                sample.data()= payload;
                std::vector<uint8_t> nice_payload = {
                  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
                  0x0a, 0x00, 0x00, 0x00, 0x73, 0x77, 0x61, 0x6d,
                  0x70, 0x5f, 0x67, 0x63, 0x73, 0x00, 0x00, 0x00,
                  0x00, 0x00, 0x00, 0x00, 0xf3, 0xfb, 0x80, 0x1d,
                  0x91, 0x01, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
                  0xfb, 0x6b, 0x02, 0x65, 0x01, 0x09, 0x01, 0x00,
                  0x00, 0x01, 0x00, 0x10, 0x0f, 0x00, 0x0f, 0x07,
                  0x01, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                  0x00, 0x0e, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01,
                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf5, 0x01,
                  0x00, 0x00, 0xf5, 0x01, 0x00, 0x00
               };
                //sample.data() = nice_payload;

                std::cout << "Sending data in hex: ";
                for (auto byte : sample.data()) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                }
                std::cout << std::dec << std::endl;

                std::cout << "Sending data in bytes: ";
                for (auto byte : sample.data()) {
                    std::cout << (int)byte << " ";
                }
                std::cout << std::endl;
                for (auto& pub_data : participants_data) {
                    for (auto& writer_data : pub_data.writers_data) {
                        if (writer_data.updated_destination_ip == destination_ip) {
                            writer_data.writer->write(&sample);
                            //std::this_thread::sleep_for(std::chrono::milliseconds(100));

                            std::cout << "Data sent in hex (after write): ";
                            for (auto byte : sample.data()) {
                                const std::vector<uint8_t>& bata = sample.data(); 
                                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                                printf("Size of data vector: %zu\n", bata.size());
                            }
                            std::cout << std::dec << std::endl;

                            sample.data().clear();
                            // Clear the payload after sending

                            // Print data after clearing
                            std::cout << "Data cleared in hex (after clear): ";
                            for (auto byte : sample.data()) {
                                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                            }
                            std::cout << std::dec << std::endl;

                            std::cout << "Data cleared in bytes (after clear): ";
                            for (auto byte : sample.data()) {
                                std::cout << (int)byte << " ";
                            }
                            std::cout << std::endl;

                            break;
                        }
                    }
                }

                //new_data = true; // Set the send_data flag to true

            }
        }
    };

    // Start capturing packets
    while (!stop_capture) {
        pcap_dispatch(handle, 0, packetHandler, nullptr); // Capture packets in a non-blocking manner
    }

    // Cleanup
    pcap_close(handle);
}



void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    std::thread rtps_thread(capture_rtps);

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {   new_data = true;
            //bool send_data = false;
            //if (new_data)
            //{
           //     send_data = true;
           //     new_data = false; // Reset the flag
           // }

           // if (send_data)
           // {
                //std::cout << "Captured topic: " << captured_topic << std::endl;
              //  std::cout << "Destination IP: " << destination_ip << std::endl;

              //  ++number_of_messages_sent;
            //}
        }

    }

    int number_of_messages_sent = 0;

    rtps_thread.detach();
 
}



void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}




