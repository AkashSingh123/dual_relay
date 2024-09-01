#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "ShapePublisher.hpp"
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

#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <iostream>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/common/Locator.h>
#include "ShapeSubscriber.hpp"
#include "NetboxMessage1PubSubTypes.h" // Include Type 1
#include "NetboxMessage2PubSubTypes.h" // Include Type 1


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

ShapePublisher::ShapePublisher() : type_(new NetboxMessage2PubSubType()), listener_()
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
        participant_qos.wire_protocol().builtin.metatrafficUnicastLocatorList.clear();

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
       // topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
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
        datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;
      //  datawriter_qos.reliability().max_fragment_size = 1024;

       // datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);
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
    int to_ms = 100;     // Read timeout in milliseconds

    // Open the selected device for packet capture
    handle = pcap_open_live(device->name, snaplen, promisc, to_ms, errorBuffer);
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

    // Create a context structure to hold the accumulated payload
    struct CaptureContext {
        std::vector<uint8_t> accumulated_payload;
    };

    CaptureContext context;

    // Define the packet handler as a lambda, using the context
    auto packetHandler = [](u_char* userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
        CaptureContext* context = reinterpret_cast<CaptureContext*>(userData);

        // RTPS magic header bytes
        const u_char rtps_magic[] = {0x52, 0x54, 0x50, 0x53};

        // Search for the RTPS magic header
        const u_char* magic = std::search(packet, packet + pkthdr->len, std::begin(rtps_magic), std::end(rtps_magic));
        if (magic == packet + pkthdr->len) {
            return; // RTPS magic header not found
        }

        // Check for submessage ID and handle accordingly
        if (magic + 24 <= packet + pkthdr->len) {
            uint8_t submessage_id = magic[20];

            if (submessage_id == 0x15) {
                // If submessage ID 0x15 is encountered, send accumulated 0x16 payload (if any) first
                if (!context->accumulated_payload.empty()) {
                    std::cout << "Sending accumulated payload before handling 0x15." << std::endl;

                    // Send the accumulated payload
                   // std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
                    NetboxMessage2 sample;
                    sample.id(2);
                    sample.data().clear();
                    sample.data() = context->accumulated_payload; // Use the accumulated payload

                    /*std::cout << "Sending accumulated payload in hex: ";
                    for (auto byte : sample.data()) {
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    }
                    std::cout << std::dec << std::endl;*/

                    for (auto& pub_data : participants_data) {
                        for (auto& writer_data : pub_data.writers_data) {
                            if (writer_data.updated_destination_ip == destination_ip) {
                                writer_data.writer->write(&sample);

                                // Clear the payload after sending
                                context->accumulated_payload.clear();

                                break;
                            }
                        }
                    }
                }

                // Handle submessage ID 0x15 and send the payload immediately
                if (magic + 48 <= packet + pkthdr->len) {
                    uint16_t encapsulation_kind = (magic[44] << 8) | magic[45];
                    if (encapsulation_kind == 0x0001) {
                        std::cout << "Fragment found with submessage ID 0x15 and encapsulation kind 0x0001" << std::endl;

                        size_t payload_start_pos = 48 + 50; // Position after the RTPS header
                        if (payload_start_pos < pkthdr->len) {
                            std::vector<uint8_t> payload(packet + payload_start_pos, packet + pkthdr->len - 32);

                           /* std::cout << "Payload segment (0x15): ";
                            for (const auto& byte : payload) {
                                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                            }
                            std::cout << std::dec << std::endl;*/

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

                            /*std::cout << "Destination port: " << destPort << std::endl;
                            std::cout << "Domain ID: " << calculatedDomainId << std::endl;
                            std::cout << "Destination IP: " << destination_ip << std::endl;*/

                            // Set the payload in the sample 
                         //   std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
                            NetboxMessage2 sample;
                            sample.id(2);
                            sample.data().clear();
                            sample.data() = payload;

                            /*std::cout << "Sending data in hex: ";
                            for (auto byte : sample.data()) {
                                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                            }
                            std::cout << std::dec << std::endl;*/

                            for (auto& pub_data : participants_data) {
                                for (auto& writer_data : pub_data.writers_data) {
                                    if (writer_data.updated_destination_ip == destination_ip) {
                                        writer_data.writer->write(&sample);

                                       /* std::cout << "Data sent in hex (after write): ";
                                        for (auto byte : sample.data()) {
                                            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                                        }
                                        std::cout << std::dec << std::endl;*/

                                        sample.data().clear();

                                        /*std::cout << "Data cleared in hex (after clear): ";
                                        for (auto byte : sample.data()) {
                                            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                                        }
                                        std::cout << std::dec << std::endl;*/

                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (submessage_id == 0x16) {
                size_t payload_start_pos = 48 + 28 + 22; // Default for subsequent fragments
                bool is_initial_fragment = false;

                if (magic + 58 <= packet + pkthdr->len) {  // Adjusted position for encapsulation kind
                    uint16_t encapsulation_kind = (magic[56] << 8) | magic[57];
                    uint16_t following_bytes = (magic[58] << 8) | magic[59]; // Next two bytes after encapsulation kind
                    if (encapsulation_kind == 0x0001 && following_bytes == 0x0000) {
                        std::cout << "First fragment found with submessage ID 0x16, encapsulation kind 0x0001, and following bytes 00 00" << std::endl;

                        // If a new initial fragment is found, reset the accumulated payload
                        context->accumulated_payload.clear();

                        is_initial_fragment = true;
                        payload_start_pos = 48 + 28 + 34; // Adjust for initial fragment
                    }
                }

                if (payload_start_pos < pkthdr->len) {
                    std::vector<uint8_t> payload(packet + payload_start_pos, packet + pkthdr->len);

                    // Print the subsequent payload before adding
                    /*std::cout << "Subsequent fragment payload before adding: ";
                    for (const auto& byte : payload) {
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    }
                    std::cout << std::dec << std::endl;*/

                    // Add to the accumulated payload
                    context->accumulated_payload.insert(context->accumulated_payload.end(), payload.begin(), payload.end());

                    // Print the accumulated payload after adding
                   /* std::cout << "Accumulated payload after adding: ";
                    for (const auto& byte : context->accumulated_payload) {
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    }
                    std::cout << std::dec << std::endl;*/

                    // Update metadata for 0x16 fragments
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

                    /*std::cout << "Destination port: " << destPort << std::endl;
                    std::cout << "Domain ID: " << calculatedDomainId << std::endl;
                    std::cout << "Destination IP: " << destination_ip << std::endl;*/

                }
            } else {
                // If a non-0x16 submessage ID is encountered, send the accumulated payload
                if (!context->accumulated_payload.empty()) {
                    std::cout << "Sending accumulated payload." << std::endl;

                    // Send the accumulated payload
                 //   std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
                    NetboxMessage2 sample;
                    sample.id(2);
                    sample.data().clear();
                    sample.data() = context->accumulated_payload; // Use the accumulated payload

                    /*std::cout << "Sending accumulated payload in hex: ";
                    for (auto byte : sample.data()) {
                        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                    }
                    std::cout << std::dec << std::endl;*/

                    for (auto& pub_data : participants_data) {
                        for (auto& writer_data : pub_data.writers_data) {
                            if (writer_data.updated_destination_ip == destination_ip) {
                                writer_data.writer->write(&sample);

                                // Clear the payload after sending
                                context->accumulated_payload.clear();

                                break;
                            }
                        }
                    }
                }
            }
        }
    };

    // Start capturing packets, pass the context structure as user data
    while (!stop_capture) {
        pcap_dispatch(handle, 0, packetHandler, reinterpret_cast<u_char*>(&context)); // Capture packets in a non-blocking manner
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



class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *publisher_participant_;
    DomainParticipant *subscriber_participant_;
    Publisher *publisher_instance_;
    Subscriber *subscriber_instance_;
    Topic *pubsub_topic_;
    DataReader *data_reader_;
    DataWriter *data_writer_;
    TypeSupport message_type_;

    class PubSubListener : public DataReaderListener, public DataWriterListener
    {
    public:
        PubSubListener(DataWriter* writer) : associated_writer_(writer), match_count(0), received_samples_count(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK)
            {
                ++received_samples_count;
                std::cout << "Message received, ID: " << sample.id() << std::endl;
                associated_writer_->write(&sample);
            }
            else
            {
                std::cout << "Read failed with error code." << std::endl;
            }
        }

        void on_publication_matched(DataWriter *writer, const PublicationMatchedStatus &info) override
        {
            match_count = info.current_count;
            std::cout << "Number of matched readers: " << match_count << std::endl;
        }

        DataWriter* associated_writer_;
        int match_count;
        uint32_t received_samples_count;
    } pubsub_listener_;

    static volatile sig_atomic_t stop_signal_flag;
    static void handle_interrupt(int) { stop_signal_flag = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal_flag = 0;

CombinedPubSub::CombinedPubSub() : 
    publisher_participant_(nullptr), 
    subscriber_participant_(nullptr),
    publisher_instance_(nullptr), 
    subscriber_instance_(nullptr),
    pubsub_topic_(nullptr), 
    data_reader_(nullptr), 
    data_writer_(nullptr), 
    message_type_(new NetboxMessage1PubSubType()), 
    pubsub_listener_(nullptr) {}

CombinedPubSub::~CombinedPubSub()
{
    if (data_reader_ && subscriber_instance_)
    {
        subscriber_instance_->delete_datareader(data_reader_);
    }

    if (subscriber_instance_ && subscriber_participant_)
    {
        subscriber_participant_->delete_subscriber(subscriber_instance_);
    }

    if (pubsub_topic_ && publisher_participant_)
    {
        publisher_participant_->delete_topic(pubsub_topic_);
    }

    if (publisher_instance_ && publisher_participant_)
    {
        publisher_participant_->delete_publisher(publisher_instance_);
    }

    if (publisher_participant_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(publisher_participant_);
    }

    if (subscriber_participant_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(subscriber_participant_);
    }
}



bool CombinedPubSub::init()
{
    // Initialize the Subscriber side
    DomainParticipantQos sub_participant_qos;
    sub_participant_qos.name("subscriber_participantp");

    subscriber_participant_ = DomainParticipantFactory::get_instance()->create_participant(7, sub_participant_qos);
    if (!subscriber_participant_)
        return false;

    message_type_.register_type(subscriber_participant_);

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite;
    datareader_qos.latency_budget().duration = eprosima::fastrtps::c_TimeInfinite; // Minimal latency budget 


    datareader_qos.endpoint().unicast_locator_list.clear();

    // Add unicast locator with IP address 10.58.40.225
    eprosima::fastrtps::rtps::Locator_t unicast_locator;
    unicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, "192.168.1.52");
    unicast_locator.port = 9161; 
    datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);

    // Add multicast locator
    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
    sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, 239, 255, 0, 7);
    sub_multicast_locator.port = 7900;
    datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    subscriber_qos.entity_factory().autoenable_created_entities = true;
    subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;
    
    subscriber_instance_ = subscriber_participant_->create_subscriber(subscriber_qos);
    if (!subscriber_instance_)
        return false;

    pubsub_topic_ = subscriber_participant_->create_topic("1", message_type_.get_type_name(), TOPIC_QOS_DEFAULT);
    if (!pubsub_topic_)
        return false;

    pubsub_listener_ = PubSubListener(nullptr);  // Temporary until we set the writer

    data_reader_ = subscriber_instance_->create_datareader(pubsub_topic_, datareader_qos, &pubsub_listener_);
    if (!data_reader_)
        return false;

    // Initialize the Publisher side
    DomainParticipantQos pub_participant_qos;
    pub_participant_qos.name("publisher_participantp");

    publisher_participant_ = DomainParticipantFactory::get_instance()->create_participant(1, pub_participant_qos);
    if (!publisher_participant_)
        return false;

    message_type_.register_type(publisher_participant_);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
    publisher_instance_ = publisher_participant_->create_publisher(publisher_qos);
    if (!publisher_instance_)
        return false;

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    Topic *pub_topic = publisher_participant_->create_topic("1", message_type_.get_type_name(), topic_qos);
    if (!pub_topic)
        return false;

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Configure multicast settings for the Publisher
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
    pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, 239, 255, 0, 1);
    pub_multicast_locator.port = 7900;
    datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

    data_writer_ = publisher_instance_->create_datawriter(pub_topic, datawriter_qos, &pubsub_listener_);
    if (!data_writer_)
        return false;

    pubsub_listener_.associated_writer_ = data_writer_;  // Now that the writer is set, update the listener

    return true;
}


void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_flag)
    {
        // Keep running until interrupted
    }
    std::cout << "\nStopped" << std::endl;
}

int main(int argc, char** argv) {
    ShapePublisher publisher;
    CombinedPubSub combined_pubsub;

    if (publisher.init(false) && combined_pubsub.init()) {
        std::thread publisher_thread(&ShapePublisher::run, &publisher);
        std::thread combined_pubsub_thread(&CombinedPubSub::run, &combined_pubsub);

        publisher_thread.join();
        combined_pubsub_thread.join();
    } else {
        std::cerr << "Failed to initialize publisher or combined_pubsub." << std::endl;
    }

    return 0;
}

