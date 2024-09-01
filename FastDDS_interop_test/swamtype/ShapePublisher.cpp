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
//namespace bp = boost::process;

//bp::child tshark_process;


ShapePublisher::ShapePublisher() : topic2(nullptr), topic3(nullptr), type_(new NetboxMessagePubSubType()), listener_() {}

ShapePublisher::~ShapePublisher()
{
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

    // Create participants for both domain IDs
    participant2 = DomainParticipantFactory::get_instance()->create_participant(2, participant_qos);
    participant3 = DomainParticipantFactory::get_instance()->create_participant(3, participant_qos);

    if (!participant2 || !participant3)
    {
        return false;
    }

    type_.register_type(participant2);
    type_.register_type(participant3);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;

    publisher2 = participant2->create_publisher(publisher_qos);
    publisher3 = participant3->create_publisher(publisher_qos);

    if (!publisher2 || !publisher3)
    {
        return false;
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic2 = participant2->create_topic("pos", type_.get_type_name(), topic_qos);
    topic3 = participant3->create_topic("waypoints_gcs", type_.get_type_name(), topic_qos);


    if (!topic2 || !topic3)
    {
        return false;
    }

   /* DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.endpoint().unicast_locator_list.clear();
    // Configure multicast settings for participant2
    eprosima::fastrtps::rtps::Locator_t multicast_locator2;
    multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator2, 239, 255, 0, 2); // Multicast address
    multicast_locator2.port = 7900; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator2);

    writer2 = publisher2->create_datawriter(topic2, datawriter_qos, &listener_);

    // Configure multicast settings for participant3
    eprosima::fastrtps::rtps::Locator_t multicast_locator3;
    multicast_locator3.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator3, 239, 255, 0, 3); // Multicast address
    multicast_locator3.port = 7901; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.clear(); // Clear previous locator
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator3);

    writer3 = publisher3->create_datawriter(topic3, datawriter_qos, &listener_);*/
    
    
    
    DataWriterQos datawriter_qos2 = DATAWRITER_QOS_DEFAULT;
    DataWriterQos datawriter_qos3 = DATAWRITER_QOS_DEFAULT;
    datawriter_qos2.endpoint().unicast_locator_list.clear();
    datawriter_qos3.endpoint().unicast_locator_list.clear();
    // Configure multicast settings for participant2
    eprosima::fastrtps::rtps::Locator_t multicast_locator2;
    multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator2, 239, 255, 0, 2); // Multicast address
    multicast_locator2.port = 7900; // Multicast port
    datawriter_qos2.endpoint().multicast_locator_list.push_back(multicast_locator2);

    writer2 = publisher2->create_datawriter(topic2, datawriter_qos2, &listener_);

    // Configure multicast settings for participant3
    eprosima::fastrtps::rtps::Locator_t multicast_locator3;
    multicast_locator3.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator3, 239, 255, 0, 3); // Multicast address
    multicast_locator3.port = 7900; // Multicast port
    datawriter_qos3.endpoint().multicast_locator_list.push_back(multicast_locator3);

    writer3 = publisher3->create_datawriter(topic3, datawriter_qos3, &listener_);


    if (writer2 && writer3)
    {
        std::cout << "DataWriters created for the topics pos and target_update." << std::endl;
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
        stop = 1;
    }
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 3) { // skip the colons
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}
pid_t tshark_pid = -1;

// Signal handler to handle Ctrl+C and stop the tshark process



void handle_signal(int signal) {
    std::cout << "Stopping TShark..." << std::endl;
    system("pkill tshark");
    exit(signal);
}


void capture_tshark()
{
    FILE* pipe = popen("sudo stdbuf -o0 -e0 tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }
    
    signal(SIGINT, publisher_stop::handle_interrupt);
    tshark_pid = fileno(pipe);

    char buffer[4096];
    bool found_ip = false;
    int lines_since_ip = 0;
    std::string topic;
    std::string potential_destination_ip; // Initialize potential_destination_ip

    while (!publisher_stop::stop)
    {   signal(SIGINT, publisher_stop::handle_interrupt);
        std::string line;
        destination_ip.clear();
        found_ip = false;
        lines_since_ip = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            line = buffer;

            if (!found_ip)
            {
                // Look for the destination IP and directly substitute it
                if (line.find("\"ip.dst_host\": \"239.255.0.2\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.3";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                  //  std::cout << "Matched IP: 239.255.0.3" << std::endl;
                }
                else if (line.find("\"ip.dst_host\": \"239.255.0.3\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.2";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                   // std::cout << "Matched IP: 239.255.0.2 please check" << std::endl;
                }
            }
            else
            {
                lines_since_ip++;

                if (lines_since_ip > 70)
                {
                    // Reset if more than 70 lines have passed since the IP was found
                    found_ip = false;
                    topic.clear();
                    continue;
                }

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

                if (!topic.empty())
                {
                    // Check for rtps.issueData within 15 lines of finding the writer entity ID
                    for (int i = 0; i < 15; ++i)
                    {
                        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                        {
                            line = buffer;

                            if (line.find("\"rtps.issueData\": ") != std::string::npos)
                            {
                                size_t start_pos = line.find("\"rtps.issueData\": ") + 19; // 19 is the length of "\"rtps.issueData\": "
                                size_t end_pos = line.find("\"", start_pos);
                                std::string hex_data = line.substr(start_pos, end_pos - start_pos);

                                captured_data = hex_data;
                                captured_topic = topic; // Set the captured topic
                                destination_ip = potential_destination_ip; // Set the destination IP

                                // Assign the payloads based on the captured topic
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

                                // Print captured data
                                std::cout << "Captured data: " << hex_data << std::endl;
                                std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                                std::cout << "Destination IP: " << destination_ip << std::endl;

                                // Set the new data flag
                                new_data = true;

                                // Clear state variables
                                found_ip = false;
                                topic.clear();

                                // Stop capturing once issue data is found
                                break;
                            }
                        }
                    }

                    // Clear topic after processing
                    topic.clear();
                }
            }
        }
    }
    pclose(pipe);
}

/*

void capture_tshark()
{
    FILE* pipe = popen("sudo stdbuf -o10 -e10 tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    bool found_ip = false;
    int lines_since_ip = 0;
    std::string topic;
    std::string potential_destination_ip; // Initialize potential_destination_ip

    while (!publisher_stop::stop)
    {
        std::string line;
        destination_ip.clear();
        found_ip = false;
        lines_since_ip = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            line = buffer;

            if (!found_ip)
            {
                // Look for the destination IP and directly substitute it
                if (line.find("\"ip.dst_host\": \"239.255.0.2\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.3";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                  //  std::cout << "Matched IP: 239.255.0.3" << std::endl;
                }
                else if (line.find("\"ip.dst_host\": \"239.255.0.3\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.2";
                    found_ip = true;
                    lines_since_ip = 0; // Reset lines_since_ip
                   // std::cout << "Matched IP: 239.255.0.2 please check" << std::endl;
                }
            }
            else
            {
                lines_since_ip++;

                if (lines_since_ip > 70)
                {
                    // Reset if more than 70 lines have passed since the IP was found
                    found_ip = false;
                    topic.clear();
                    continue;
                }

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

                if (!topic.empty())
                {
                    // Check for rtps.issueData within 15 lines of finding the writer entity ID
                    for (int i = 0; i < 15; ++i)
                    {
                        if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                        {
                            line = buffer;

                            if (line.find("\"rtps.issueData\": ") != std::string::npos)
                            {
                                size_t start_pos = line.find("\"rtps.issueData\": ") + 19; // 19 is the length of "\"rtps.issueData\": "
                                size_t end_pos = line.find("\"", start_pos);
                                std::string hex_data = line.substr(start_pos, end_pos - start_pos);

                                captured_data = hex_data;
                                captured_topic = topic; // Set the captured topic
                                destination_ip = potential_destination_ip; // Set the destination IP

                                // Assign the payloads based on the captured topic
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

                                // Print captured data
                                std::cout << "Captured data: " << hex_data << std::endl;
                                std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                                std::cout << "Destination IP: " << destination_ip << std::endl;

                                // Set the new data flag
                                new_data = true;

                                // Clear state variables
                                found_ip = false;
                                topic.clear();

                                // Stop capturing once issue data is found
                                break;
                            }
                        }
                    }

                    // Clear topic after processing
                    topic.clear();
                }
            }
        }
    }
    pclose(pipe);
}
*/
/*void capture_tshark()
{
    bp::ipstream pipe_stream;
    tshark_process = bp::child(bp::search_path("tshark"), "-i", "any", "-Y", "rtps", "-T", "json", bp::std_out > pipe_stream);

    std::string line;
    bool found_ip = false;
    int lines_since_ip = 0;
    std::string topic;

    while (!publisher_stop::stop && tshark_process.running())
    {
        std::string potential_destination_ip; // Initialize potential_destination_ip
        destination_ip.clear();
        found_ip = false;
        lines_since_ip = 0;

        while (std::getline(pipe_stream, line))
        {
            if (!found_ip)
            {
                // Look for the destination IP and directly substitute it
                if (line.find("\"ip.dst_host\": \"239.255.0.2\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.3";
                    found_ip = true;
                    std::cout << "Matched IP: 239.255.0.2" << std::endl;
                    lines_since_ip = 0; // Reset lines_since_ip
                }
                else if (line.find("\"ip.dst_host\": \"239.255.0.3\"") != std::string::npos)
                {
                    potential_destination_ip = "239.255.0.2";
                    found_ip = true;
                    std::cout << "Matched IP: 239.255.0.3" << std::endl;
                    lines_since_ip = 0; // Reset lines_since_ip
                }
            }
            else
            {
                lines_since_ip++;

                if (lines_since_ip > 40)
                {
                    // Reset if more than 40 lines have passed since the IP was found
                    found_ip = false;
                    continue;
                }

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
                    std::cout << "Matched topic: " << topic << std::endl;
                }

                if (!topic.empty())
                {
                    // Check for rtps.issueData within 15 lines of finding the writer entity ID
                    for (int i = 0; i < 15; ++i)
                    {
                        if (std::getline(pipe_stream, line))
                        {
                            if (line.find("\"rtps.issueData\": ") != std::string::npos)
                            {
                                size_t start_pos = line.find("\"rtps.issueData\": ") + 19; // 19 is the length of "\"rtps.issueData\": "
                                size_t end_pos = line.find("\"", start_pos);
                                std::string hex_data = line.substr(start_pos, end_pos - start_pos);

                                captured_data = hex_data;
                                captured_topic = topic; // Set the captured topic
                                destination_ip = potential_destination_ip; // Set the destination IP

                                // Assign the payloads based on the captured topic
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
                                    if (payload.size() > 36)
                                    {
                                        last_target_update_payload = std::vector<uint8_t>(payload.begin() + 36, payload.end());
                                    }
                                }

                                // Print captured data
                                std::cout << "Captured data: " << hex_data << std::endl;
                                std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                                std::cout << "Destination IP: " << destination_ip << std::endl;

                                // Set the new data flag
                                new_data = true;

                                // Stop capturing once issue data is found
                                break;
                            }
                        }
                    }

                    // Clear topic after processing
                    topic.clear();
                }
            }
        }
    }
    tshark_process.wait();
}*/





void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    std::thread tshark_thread(capture_tshark);
    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
         bool send_data = false;
            {
  //              std::lock_guard<std::mutex> lock(data_mutex);
                if (new_data)
                {
                    send_data = true;
                    new_data = false; // Reset the flag
                    //last_send_time = std::chrono::steady_clock::now();
                }
            }

            // Clear the sample topics before setting new ones
            if (send_data)
            {
                // Clear the sample topics before setting new ones
                sample.topics().clear();

                // Send the payload to the correct destination based on the topic and IP
                if (captured_topic == "command")
                {
                    sample.topics().emplace_back("command");
                    sample.payload(last_command_payload);

                    if (destination_ip == "239.255.0.2")
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

                    if (destination_ip == "239.255.0.2")
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

                    if (destination_ip == "239.255.0.2")
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

                // Print the payload before sending
                std::cout << "Sending sample with " << captured_topic << " payload: ";
                for (const auto& byte : sample.payload())
                {
                    std::cout << std::hex << static_cast<int>(byte) << " ";
                }
                std::cout << std::endl;

                ++number_of_messages_sent;
            }
        }

       // std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    std::cout << "\nStopped" << std::endl;

    tshark_thread.join();
}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}



