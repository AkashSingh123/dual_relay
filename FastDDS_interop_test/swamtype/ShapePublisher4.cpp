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
#include "globals.hpp" // Include the globals header
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

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

namespace publisher_stop
{
    volatile sig_atomic_t stop;
    void handle_interrupt(int)
    {
        std::cout << "Stopping TShark..." << std::endl;
        system("pkill tshark");
        stop = 1;
    }
}
std::string bytesToHexString(const u_char* bytes, int length) {
    std::ostringstream oss;
    for (int i = 0; i < length; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << " ";
    }
    return oss.str();
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

void capture_tshark()
{
    
    FILE* pipe = popen("sudo stdbuf -i0 -o0 -e0 tshark -i any -Y rtps -T json -l --update-interval 1", "r");
 // FILE* pipe = popen("sudo stdbuf -i0 -o0 -e0 tshark -i any -Y \"(ip.dst == 239.255.0.2 || ip.dst == 239.255.0.3 || ip.dst == 239.255.0.1) && rtps\" -Y rtps -T json", "r");
//    FILE* pipe = popen("sudo stdbuf -i0 -o0 -e0 tshark -i any -Y \"udp && (ip.dst == 239.255.0.1 || ip.dst == 239.255.0.2 || ip.dst == 239.255.0.3)\" -T json | jq -c 'select(.layers[\"rtps.issueData\"])'", "r");
    //FILE* pipe = popen("sudo stdbuf -i0 -o0 -e0 tshark -i any -Y \"udp && (ip.dst == 239.255.0.1 || ip.dst == 239.255.0.2 || ip.dst == 239.255.0.3)\" -T json | jq -c 'select(.layers.rtps.sm.wrEntityId == \"0x00001202\" and .layers.serializedData[\"rtps.issueData\"])'", "r");

    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    signal(SIGINT, publisher_stop::handle_interrupt);

    char buffer[1024];
    bool found_ip = false;
    int lines_since_ip = 0;
    std::string topic;
    std::string potential_destination_ip; // Initialize potential_destination_ip
    std::ofstream logFile("tshark_output.log", std::ios::app); // Open log file in append mode

    while (!publisher_stop::stop)
    {
        std::string line;
        destination_ip.clear();
        found_ip = false;
        lines_since_ip = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            line = buffer;
            //std::cout << "Read line: " << line << std::endl;  // Print each line read from the buffer


            if (!found_ip)
            {
                if (line.find("\"ip.dst_host\": \"239.255.0.1\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.2";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                }
                else if (line.find("\"ip.dst_host\": \"239.255.0.2\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.3";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                }
                else if (line.find("\"ip.dst_host\": \"239.255.0.3\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.1";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                }
            }
            else
            {
             lines_since_ip++;

             if (lines_since_ip > 70)
                {
                    found_ip = false;
                    topic.clear();
                    continue;
                }

             if (lines_since_ip >= 55 && lines_since_ip <= 70)
               {

                if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
                {
                    topic = "command";
                }
                else if (line.find("\"rtps.sm.wrEntityId\": \"0x00000e02\"") != std::string::npos)
                {
                    topic = "swamp_gcs";
                }
                else if (line.find("\"rtps.sm.wrEntityId\": \"0x00001202\"") != std::string::npos)
                {
                    topic = "target_update";
                }
                else if (line.find("\"rtps.sm.wrEntityId\": \"0x00001802\"") != std::string::npos)
                {
                    topic = "zones";
                }


                if (!topic.empty())
                {
                    for (int i = 0; i < 14; ++i)
                    {
                        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                        {
                            line = buffer;
                          //  logFile << line;  // Save each line to the file
                            if (line.find("\"rtps.issueData\": ") != std::string::npos)
                            {   
                                logFile << line;  // Log the line when rtps.issueData is found

                                size_t start_pos = line.find("\"rtps.issueData\": ") + 19; // 19 is the length of "\"rtps.issueData\": "
                                size_t end_pos = line.find("\"", start_pos);
                                std::string hex_data = line.substr(start_pos, end_pos - start_pos);

                                captured_data = hex_data;
                                captured_topic = topic; // Set the captured topic
                                destination_ip = potential_destination_ip; // Set the destination IP

                                std::vector<uint8_t> payload = hex_to_bytes(captured_data);
                                if (captured_topic == "command")
                                {
                                    if (payload.size() > 36)
                                    {
                                        last_command_payload = std::vector<uint8_t>(payload.begin() + 36, payload.end());
                                    }
                                }
                                else if (captured_topic == "swamp_gcs")
                                {
                                    if (payload.size() > 44)
                                    {
                                        last_swamp_gcs_payload = std::vector<uint8_t>(payload.begin() + 44, payload.end());
                                    }
                                }
                                else if (captured_topic == "target_update")
                                {
                                    if (payload.size() > 44)
                                    {
                                        last_target_update_payload = std::vector<uint8_t>(payload.begin() + 44, payload.end());
                                    }
                                }
                                else if (captured_topic == "zones")
                                {
                                    if (payload.size() > 36)
                                    {
                                        last_zones_payload = std::vector<uint8_t>(payload.begin() + 36, payload.end());
                                    }
                                }

                               // std::cout << "Captured data: " << hex_data << std::endl;
                                //std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                                //std::cout << "Destination IP: " << destination_ip << std::endl;

                                new_data = true;

                                found_ip = false;
                                topic.clear();
                                break;
                            }
                        }
                    }
                    topic.clear();
                 }
               }
            }
        }
    }
    logFile.close();

    pclose(pipe);
}

void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    auto timestamp = std::chrono::system_clock::now().time_since_epoch().count();
    sample.timestamp(timestamp);


    std::thread tshark_thread(capture_tshark);
    std::cout << "Timestamp being sent: " << timestamp << std::endl;

    int number_of_messages_sent = 0;
    std::ofstream log_file("payload_log.txt", std::ios::out | std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Failed to open the log file." << std::endl;
        return;
    }

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            bool send_data = false;
            if (new_data)
            {
                send_data = true;
                new_data = false; // Reset the flag
            }

            if (send_data)
            {
                sample.topics().clear();

                if (captured_topic == "command")
                {
                    sample.topics().emplace_back("command");
                    sample.payload(last_command_payload);
                    if (destination_ip == "239.255.0.1")
                    {
                        writer1->write(&sample);
                        std::cout << "Sending to command at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {
                        writer2->write(&sample);
                        std::cout << "Sending to command at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {
                        writer3->write(&sample);
                        std::cout << "Sending to command at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "swamp_gcs")
                {
                    sample.topics().emplace_back("swamp_gcs");
                    sample.payload(last_swamp_gcs_payload);
                    if (destination_ip == "239.255.0.1")
                    {
                        writer1->write(&sample);
                       std::cout << "Sending to swamp_gcs at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {
                        writer2->write(&sample);
                        std::cout << "Sending to swamp_gcs at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {
                        writer3->write(&sample);
                        std::cout << "Sending to swamp_gcs at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "target_update")
                {
                    sample.topics().emplace_back("target_update");
                    sample.payload(last_target_update_payload);
                    if (destination_ip == "239.255.0.1")
                    {
                        writer1->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {
                        writer2->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {
                        writer3->write(&sample);
                        std::cout << "Sending to target_update at 239.255.0.3 using writer3_" << std::endl;
                    }
                }
                else if (captured_topic == "zones")
                {
                    sample.topics().emplace_back("zones");
                    sample.payload(last_zones_payload);
                    if (destination_ip == "239.255.0.1")
                    {
                        writer1->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.1 using writer1_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.2")
                    {
                        writer2->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.2 using writer2_" << std::endl;
                    }
                    else if (destination_ip == "239.255.0.3")
                    {
                        writer3->write(&sample);
                        std::cout << "Sending to zones at 239.255.0.3 using writer3_" << std::endl;
                    }
                }

                log_file << "Sending sample with " << captured_topic << " payload: ";

                for (const auto &byte : sample.payload())
                {
                    log_file << std::hex << static_cast<int>(byte) << " ";

                }
                std::cout << std::endl;

                ++number_of_messages_sent;
            }
        }

     //   std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "\nStopped" << std::endl;

   tshark_thread.join();
   log_file.close();

}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}

