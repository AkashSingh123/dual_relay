#include "ShapePublisher.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/rtps/attributes/PropertyPolicy.h>
#include <iostream>
#include <fstream>
#include <csignal>
#include <cstdio>
#include <sstream>
#include <vector>
#include <iomanip>
#include "globals.hpp"
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
std::atomic<bool> stop_capture(false);
std::mutex mtx;
std::condition_variable cv;

using namespace eprosima::fastdds::dds;

ShapePublisher::ShapePublisher() : topic1(nullptr), topic2(nullptr), topic3(nullptr), type_(new NetboxMessagePubSubType()), listener_() {}

ShapePublisher::~ShapePublisher()
{
    if (writer1 && publisher1) { publisher1->delete_datawriter(writer1); }
    if (publisher1 && participant1) { participant1->delete_publisher(publisher1); }
    if (participant1) { DomainParticipantFactory::get_instance()->delete_participant(participant1); }

    if (writer2 && publisher2) { publisher2->delete_datawriter(writer2); }
    if (publisher2 && participant2) { participant2->delete_publisher(publisher2); }
    if (participant2) { DomainParticipantFactory::get_instance()->delete_participant(participant2); }

    if (writer3 && publisher3) { publisher3->delete_datawriter(writer3); }
    if (publisher3 && participant3) { participant3->delete_publisher(publisher3); }
    if (participant3) { DomainParticipantFactory::get_instance()->delete_participant(participant3); }
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

    participant1 = DomainParticipantFactory::get_instance()->create_participant(1, participant_qos);
    participant2 = DomainParticipantFactory::get_instance()->create_participant(2, participant_qos);
    participant3 = DomainParticipantFactory::get_instance()->create_participant(3, participant_qos);

    if (!participant1 || !participant2 || !participant3)
    {
        return false;
    }

    type_.register_type(participant1);
    type_.register_type(participant2);
    type_.register_type(participant3);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;

    publisher1 = participant1->create_publisher(publisher_qos);
    publisher2 = participant2->create_publisher(publisher_qos);
    publisher3 = participant3->create_publisher(publisher_qos);

    if (!publisher1 || !publisher2 || !publisher3)
    {
        return false;
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic1 = participant1->create_topic("pos", type_.get_type_name(), topic_qos);
    topic2 = participant2->create_topic("pos", type_.get_type_name(), topic_qos);
    topic3 = participant3->create_topic("pos", type_.get_type_name(), topic_qos);

    if (!topic1 || !topic2 || !topic3)
    {
        return false;
    }

    DataWriterQos datawriter_qos1 = DATAWRITER_QOS_DEFAULT;
    DataWriterQos datawriter_qos2 = DATAWRITER_QOS_DEFAULT;
    DataWriterQos datawriter_qos3 = DATAWRITER_QOS_DEFAULT;
    datawriter_qos1.endpoint().unicast_locator_list.clear();
    datawriter_qos2.endpoint().unicast_locator_list.clear();
    datawriter_qos3.endpoint().unicast_locator_list.clear();

    eprosima::fastrtps::rtps::Locator_t multicast_locator1;
    multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator1, 239, 255, 0, 1); // Multicast address
    multicast_locator1.port = 7900; // Multicast port
    datawriter_qos1.endpoint().multicast_locator_list.push_back(multicast_locator1);

    writer1 = publisher1->create_datawriter(topic1, datawriter_qos1, &listener_);

    eprosima::fastrtps::rtps::Locator_t multicast_locator2;
    multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator2, 239, 255, 0, 2); // Multicast address
    multicast_locator2.port = 7900; // Multicast port
    datawriter_qos2.endpoint().multicast_locator_list.push_back(multicast_locator2);

    writer2 = publisher2->create_datawriter(topic2, datawriter_qos2, &listener_);

    eprosima::fastrtps::rtps::Locator_t multicast_locator3;
    multicast_locator3.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator3, 239, 255, 0, 3); // Multicast address
    multicast_locator3.port = 7900; // Multicast port
    datawriter_qos3.endpoint().multicast_locator_list.push_back(multicast_locator3);

    writer3 = publisher3->create_datawriter(topic3, datawriter_qos3, &listener_);

    if (writer1 && writer2 && writer3)
    {
        std::cout << "DataWriters created for the topics pos, waypoints_gcs, and target_update." << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}

/*namespace publisher_stop {
    volatile sig_atomic_t stop;
    void handle_interrupt(int) {
        std::cout << "Stopping capture..." << std::endl;
        stop = 1;
        stop_capture = true;

    }
}*/


namespace publisher_stop {
    std::atomic<bool> stop(false);

    void handle_interrupt(int) {
        std::cout << "Stopping capture..." << std::endl;
        stop = true;
        stop_capture = true;
        cv.notify_all();
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

// Function to print RTPS packet details and extract necessary information
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

    // Open the selected device for packet capture
    handle = pcap_open_live(device->name, BUFSIZ, 1, 1, errorBuffer);
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

        std::string writerEntityId = bytesToHexString(magic + 32, 4);
        if (writerEntityId == "00 00 0e 02 " || writerEntityId == "00 00 12 02 " ||
            writerEntityId == "00 00 16 02 " || writerEntityId == "00 00 18 02 ") {

            char timestampStr[64];
            std::tm* tm_info = std::localtime(&pkthdr->ts.tv_sec);
            std::strftime(timestampStr, sizeof(timestampStr), "%Y-%m-%d %H:%M:%S", tm_info);
            std::cout << "Packet received at: " << timestampStr << "." << std::setfill('0') << std::setw(6) << pkthdr->ts.tv_usec << std::endl;

            std::cout << "Packet length: " << pkthdr->len << std::endl;
            std::cout << "RTPS magic header found at offset: " << (magic - packet) << std::endl;

            // Protocol version is located after the magic header
            if (magic + 4 + 2 <= packet + pkthdr->len) {
                std::cout << "Protocol version: " << (int)magic[4] << "." << (int)magic[5] << std::endl;
            }

            // GUID prefix starts at offset 8 after the magic header
            if (magic + 16 + 12 <= packet + pkthdr->len) {
                std::cout << "GUID prefix: " << bytesToHexString(magic + 8, 12) << std::endl;
            }

            // Submessage ID starts at offset 20 after the magic header
            if (magic + 24 <= packet + pkthdr->len) {
                std::cout << "Submessage ID: " << std::hex << (int)magic[20] << std::dec << std::endl;
            }

            // Writer Entity ID
            std::cout << "Writer Entity ID: " << bytesToHexString(magic + 32, 4) << std::endl;

            // Writer Entity Key is located immediately after the Writer Entity ID
            if (magic + 40 <= packet + pkthdr->len) {
                std::cout << "Writer Entity Key: " << bytesToHexString(magic + 36, 4) << std::endl;
            }

            std::string source_ipcheck = bytesToIpString(magic - 16);
            std::string destination_ipcheck = bytesToIpString(magic - 12);

            std::cout << "Source IP: " << source_ipcheck << std::endl;
            std::cout << "Destination IP: " << destination_ipcheck << std::endl;
            std::lock_guard<std::mutex> lock(mtx); // Lock the mutex
            if (destination_ipcheck == "239.255.0.1") {
                destination_ip = "239.255.0.2";
                std::cout << "Now Destination IP: " << destination_ip << std::endl;
            } else if (destination_ipcheck == "239.255.0.2") {
                destination_ip = "239.255.0.3";
                std::cout << "Now Destination IP: " << destination_ip << std::endl;
            } else if (destination_ipcheck == "239.255.0.3") {
                destination_ip = "239.255.0.1";
                std::cout << "Now Destination IP: " << destination_ip << std::endl;
            }
            //destination_ip = "239.255.0.1";
            if (magic + 48 <= packet + pkthdr->len) {
                uint16_t destPort = bytesToUint16(magic - 6);
                int calculatedDomainId = calculateDomainId(destPort);

                std::cout << "Destination port: " << destPort << std::endl;
                std::cout << "Domain ID: " << calculatedDomainId << std::endl;
            }

            if (writerEntityId == "00 00 0e 02 ") {
                std::cout << "Topic: swamp_gcs" << std::endl;
                captured_topic = "swamp_gcs";
            } else if (writerEntityId == "00 00 12 02 ") {
                std::cout << "Topic: target_update" << std::endl;
                captured_topic = "target_update";
            } else if (writerEntityId == "00 00 16 02 ") {
                std::cout << "Topic: command" << std::endl;
                captured_topic = "command";
            } else if (writerEntityId == "00 00 18 02 ") {
                std::cout << "Topic: zones" << std::endl;
                captured_topic = "zones";
            }

            if (magic + 48 <= packet + pkthdr->len) {
                std::string real_serialized_data = bytesToHexString(magic + 48, pkthdr->len - (magic + 48 - packet));
                //std::cout << "Real Serialized data: " << real_serialized_data << std::endl;

                // Handle payload for each topic
                std::vector<uint8_t> payloadprep = hex_to_bytes(real_serialized_data);
                //std::unique_lock<std::mutex> lock(mtx);
                if (captured_topic == "command") {
                    if (payloadprep.size() > 36) {
                        last_command_payload = std::vector<uint8_t>(payloadprep.begin() + 36, payloadprep.end());
                       // std::cout << "Last command payload: " << real_serialized_data << std::endl;
                    }
                } else if (captured_topic == "swamp_gcs") {
                    if (payloadprep.size() > 44) {
                        last_swamp_gcs_payload = std::vector<uint8_t>(payloadprep.begin() + 44, payloadprep.end());
                       // std::cout << "Last swamp_gcs payload: " << real_serialized_data << std::endl;
                    }
                } else if (captured_topic == "target_update") {
                    if (payloadprep.size() > 44) {
                        last_target_update_payload = std::vector<uint8_t>(payloadprep.begin() + 44, payloadprep.end());
                      //  std::cout << "Last target_update payload: " << real_serialized_data << std::endl;
                    }
                } else if (captured_topic == "zones") {
                    if (payloadprep.size() > 36) {
                        last_zones_payload = std::vector<uint8_t>(payloadprep.begin() + 36, payloadprep.end());
                       // std::cout << "Last zones payload: " << real_serialized_data << std::endl;
                    }
                }

                new_data = true; // Set the send_data flag to true
            }
            
               // lock.unlock();
                cv.notify_one(); // Notify the main thread that new data is available

            std::cout << std::endl << std::endl; // Two-line space after each loop
        }
    };

    // Start capturing packets
    while (!stop_capture) {
        pcap_dispatch(handle, 0, packetHandler, nullptr); // Capture packets in a non-blocking manner
    }


    // Cleanup
    pcap_freealldevs(interfaces);
    pcap_close(handle);
    
}
void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    sample.timestamp(timestamp);


    std::thread rtps_thread(capture_rtps);
    std::cout << "Timestamp being sent: " << timestamp << std::endl;

    int number_of_messages_sent = 0;
    std::ofstream log_file("payload_log.txt", std::ios::out | std::ios::app);
   // if (!log_file.is_open()) {
   //     std::cerr << "Failed to open the log file." << std::endl;
   //     return;
  //  }

    while (!publisher_stop::stop)
    { 
    
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{ return new_data; }); // Wait for new data
        if (stop_capture) break; // Exit the loop if stop signal is received

      //std::cout << "Last zones payload: " << real_serialized_data << std::endl;
        if (listener_.matched)
        {   
            bool send_data = false;
            if (new_data)
            {
                send_data = true;
                new_data = false; // Reset the flag
            }

            if (send_data)
            {   std::cout << "Inside loop Destination IP: " << destination_ip << std::endl;
                sample.topics().clear();
                std::cout << "Captured topic in main run loop: " << captured_topic << std::endl; // Print captured topic
                std::cout << "Captured payload in main run loop: " << captured_data << std::endl; // Print captured payload

                if (captured_topic == "command")
                {   //std::cerr << "yesssss" << std::endl;
                    sample.topics().emplace_back("command");
                    sample.payload(last_command_payload);
                    if (destination_ip == "239.255.0.1")
                    {   std::cerr << "yesssss" << std::endl;
                        writer1->write(&sample);
                        std::cout << "Sending to command at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {   std::cerr << "yesssss" << std::endl;
                        writer2->write(&sample);
                        std::cout << "Sending to command at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {   std::cerr << "yesssss" << std::endl;
                        writer3->write(&sample);
                        std::cout << "Sending to command at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "swamp_gcs")
                {    //std::cerr << "yesssss" << std::endl; 
                    sample.topics().emplace_back("swamp_gcs");
                    sample.payload(last_swamp_gcs_payload);
                    if (destination_ip == "239.255.0.1")
                    {   std::cerr << "yesssss" << std::endl;
                        writer1->write(&sample);
                       std::cout << "Sending to swamp_gcs at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {   std::cerr << "yesssss" << std::endl;
                        writer2->write(&sample);
                        std::cout << "Sending to swamp_gcs at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {   std::cerr << "yesssss" << std::endl;
                        writer3->write(&sample);
                        std::cout << "Sending to swamp_gcs at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "target_update")
                {   //std::cerr << "yesssss" << std::endl;
                    sample.topics().emplace_back("target_update");
                    sample.payload(last_target_update_payload);
                    if (destination_ip == "239.255.0.1")
                    {   std::cerr << "yesssss" << std::endl;
                        writer1->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {
                        writer2->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {   std::cerr << "yesssss" << std::endl;
                        writer3->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "zones")
                {   
                    sample.topics().emplace_back("zones");
                    sample.payload(last_zones_payload);
                    if (destination_ip == "239.255.0.1")
                    {   std::cerr << "yesssss" << std::endl;
                        writer1->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {   std::cerr << "yesssss" << std::endl;
                        writer2->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {   std::cerr << "yesssss" << std::endl;
                        writer3->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.3 using writer3_" << std::endl;
                    }
                }

               // log_file << "Sending sample with " << captured_topic << " payload: ";

                for (const auto &byte : sample.payload())
                {
                   // log_file << std::hex << static_cast<int>(byte) << " ";

                }
                std::cout << std::endl;

                ++number_of_messages_sent;
            }
        }

        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

   rtps_thread.detach();

   //log_file.close();

}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}

