
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

using namespace eprosima::fastdds::dds;
std::atomic<bool> stop_capture(false);
std::mutex mtx;
std::condition_variable cv;

ShapePublisher::ShapePublisher() : participant_(nullptr), publisher_(nullptr), topic_(nullptr), writer_(nullptr), type_(new NetboxMessagePubSubType()), listener_() {}

ShapePublisher::~ShapePublisher()
{
    if (writer_ && publisher_)
    {
        publisher_->delete_datawriter(writer_);
    }

    if (publisher_ && participant_)
    {
        participant_->delete_publisher(publisher_);
    }

    if (topic_ && participant_)
    {
        participant_->delete_topic(topic_);
    }
    if (participant_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

bool ShapePublisher::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("publisher_participant");

    if (with_security)
    {
        using namespace std;
        string example_security_configuration_path = "file://../../examples/security_configuration_files/";
        string dds_sec = "dds.sec.";
        string auth = dds_sec + "auth.";
        string auth_plugin = "builtin.PKI-DH";
        string auth_prefix = auth + auth_plugin + ".";
        string access = dds_sec + "access.";
        string access_plugin = "builtin.Access-Permissions";
        string access_prefix = access + access_plugin + ".";
        string crypto = dds_sec + "crypto.";
        string crypto_plugin = "builtin.AES-GCM-GMAC";
        string plugin = "plugin";

        std::vector<pair<string, string>> security_properties = {
            pair<string, string>(auth + plugin, auth_plugin),
            pair<string, string>(access + plugin, access_plugin),
            pair<string, string>(crypto + plugin, crypto_plugin),
            pair<string, string>(auth_prefix + "identity_ca", example_security_configuration_path + "identity_ca.cert.pem"),
            pair<string, string>(auth_prefix + "identity_certificate", example_security_configuration_path + "cert.pem"),
            pair<string, string>(auth_prefix + "private_key", example_security_configuration_path + "key.pem"),
            pair<string, string>(access_prefix + "permissions_ca", example_security_configuration_path + "permissions_ca.cert.pem"),
            pair<string, string>(access_prefix + "governance", example_security_configuration_path + "governance.p7s"),
            pair<string, string>(access_prefix + "permissions", example_security_configuration_path + "permissions.p7s"),
        };

        for (pair<string, string> property : security_properties)
        {
            participant_qos.properties().properties().emplace_back(property.first, property.second);
        }
    }

    participant_ = DomainParticipantFactory::get_instance()->create_participant(2, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;

    if (participant_)
    {
        publisher_ = participant_->create_publisher(publisher_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;

    if (participant_)
    {
        topic_ = participant_->create_topic("pos", type_.get_type_name(), topic_qos);
    }

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.endpoint().unicast_locator_list.clear();

    // Configure multicast settings
     datawriter_qos.endpoint().multicast_locator_list.clear();
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 2); // Multicast address
    multicast_locator.port = 7900; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);
    datawriter_qos.endpoint().unicast_locator_list.clear();
    if (publisher_ && topic_)
    {
        writer_ = publisher_->create_datawriter(topic_, datawriter_qos, &listener_);
    }

    if (writer_ && topic_ && publisher_ && participant_)
    {
        std::cout << "DataWriter created for the topic swamp_gcs." << std::endl;
        return true;
    }
    else
    {
        return false;
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
    handle = pcap_open_live(device->name, snaplen, 1, 10, errorBuffer);
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
            size_t payload_start_pos = 48+46; // Position after the RTPS header
            size_t payload_length = pkthdr->len - payload_start_pos - 3; // Adjust for truncation

            if (payload_start_pos < pkthdr->len) {
                std::vector<uint8_t> payload(packet + payload_start_pos, packet + pkthdr->len -32);
                std::cout << "Payload segment: ";
                for (const auto& byte : payload) {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                }
                std::cout << std::endl;
                //std::lock_guard<std::mutex> lock(mtx); // Lock the mutex

                std::string source_ipcheck = bytesToIpString(magic - 16);
                std::string destination_ipcheck = bytesToIpString(magic - 12);

                // Update destination_ip based on the configuration
                uint16_t destPort = bytesToUint16(magic - 6);
                int calculatedDomainId = calculateDomainId(destPort);

                std::cout << "Destination port: " << destPort << std::endl;
                std::cout << "Domain ID: " << calculatedDomainId << std::endl;
                std::cout << "Destination IP: " << destination_ip << std::endl;



                // Set the payload in the sample 
             //if (payload.size() < 300) {
                {
                    std::lock_guard<std::mutex> lk(mtx);
                    last_zones_payload = payload;
                }
                new_data = true; // Set the flag indicating new data is available
                cv.notify_all(); // Notify the run thread

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
/*
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
                 //std::this_thread::sleep_for(std::chrono::milliseconds(100));
               
                            std::cout << "Data sent in hex (after write): ";
                            for (auto byte : sample.data()) {
                                std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
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

                            break;*/
                    

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



void ShapePublisher::run() {
    signal(SIGINT, publisher_stop::handle_interrupt);

    std::thread rtps_thread(capture_rtps);

    while (!publisher_stop::stop) {
        std::unique_lock<std::mutex> lk(mtx);
        cv.wait(lk, [] { return new_data || stop_capture; });
std::vector<uint8_t> nice_payload = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x63, 0x6f, 0x6d, 0x6d,
    0x61, 0x6e, 0x64, 0x00, 0x17, 0x66, 0x53, 0x1f,
    0x91, 0x01, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00,
    0xfb, 0x7b, 0x01, 0x65, 0x01, 0x09, 0x01, 0x00,
    0x00, 0x01, 0x00, 0x10, 0x0f, 0x00, 0x0f
};

        if (new_data && listener_.matched) {
            NetboxMessage sample;
            sample.data() = nice_payload;
      
                        writer_->write(&sample);
                        std::cout << "Data sent in hex (after write): ";
                        for (auto byte : sample.data()) {
                            std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)byte << " ";
                        }
                        std::cout << std::dec << std::endl;
        
                
            
            new_data = false; // Reset the flag
        }
    }

    rtps_thread.detach();
}


void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}




