#include "ShapePublisher.hpp"
#include "NetboxMessagePubSubTypes.h"
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <unistd.h>
#include <signal.h>
#include <string>
#include <thread>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <memory>
#include <array>
#include "globals.hpp" // Include the globals header

using namespace eprosima::fastdds::dds;

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

    participant_ = DomainParticipantFactory::get_instance()->create_participant(5, participant_qos);

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
        topic_ = participant_->create_topic("target_update", type_.get_type_name(), topic_qos);
    }

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.endpoint().unicast_locator_list.clear();

    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 5); // Multicast address
    multicast_locator.port = 7900; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);

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

// For handling stop signal to break the infinite loop
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


 //literally works
void capture_tshark()
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    std::ofstream outfile("rtps_data.log", std::ios_base::app);

    while (!publisher_stop::stop)
    {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);
            outfile << line;
        }
    }
    pclose(pipe);
    outfile.close();
}

void process_logged_data()
{
    std::ifstream infile("rtps_data.log");
    std::string line;
    bool capture_issue_data = false;
    int lines_since_entity_id = 0;
    std::string topic;

    while (std::getline(infile, line))
    {
        if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
        {
            capture_issue_data = true;
            lines_since_entity_id = 0;
            topic = "command";
        }
        else if (line.find("\"rtps.sm.wrEntityId\": \"0x00000e02\"") != std::string::npos)
        {
            capture_issue_data = true;
            lines_since_entity_id = 0;
            topic = "swamp_gcs";
        }
        else if (line.find("\"rtps.sm.wrEntityId\": \"0x00001202\"") != std::string::npos)
        {
            capture_issue_data = true;
            lines_since_entity_id = 0;
            topic = "target_update";
        }

        if (capture_issue_data)
        {
            lines_since_entity_id++;
            if (lines_since_entity_id <= 15)
            {
                if (line.find("\"rtps.issueData\": ") != std::string::npos)
                {
                    size_t start_pos = line.find("\"rtps.issueData\": ") + 19; // 19 is the length of "\"rtps.issueData\": "
                    size_t end_pos = line.find("\"", start_pos);
                    std::string hex_data = line.substr(start_pos, end_pos - start_pos);

                    captured_data = hex_data;
                    captured_topic = topic; // Set the captured topic

                    // Convert the captured data
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

                    std::cout << "Captured data: " << hex_data << std::endl;

                    // Write captured data to file
                    std::ofstream outfile("captured_data.txt", std::ios_base::app);
                    if (outfile.is_open())
                    {
                        outfile << "Captured data: " << hex_data << std::endl;
                        outfile << "Updated captured_topic: " << captured_topic << std::endl; // Log the updated topic
                        outfile.close();
                    }
                    else
                    {
                        std::cerr << "Failed to open file for writing captured data." << std::endl;
                    }

                    std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                    new_data = true; // Set the new data flag
                    std::ofstream clearfile("rtps_data.log", std::ofstream::trunc);
                    clearfile.close();


                    break; // Stop capturing once issue data is found
                }
            }
            else
            {
                capture_issue_data = false;
            }
        }
    }
    infile.close();

}


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
            process_logged_data(); // Process the logged data

            bool send_data = false;
            if (new_data)
            {
                send_data = true;
                new_data = false; // Reset the flag
            }

            if (send_data)
            {
                // Send the last payloads for each topic
                if (!last_command_payload.empty())
                {
                    sample.topics().clear();
                    sample.topics().emplace_back("command");
                    sample.payload(last_command_payload);

                    // Print the payload before sending
                    std::cout << "Sending sample with command payload: ";
                    for (const auto& byte : sample.payload())
                    {
                        std::cout << std::hex << static_cast<int>(byte) << " ";
                    }
                    std::cout << std::endl;

                    writer_->write(&sample);
                    ++number_of_messages_sent;
                }

                if (!last_swamp_gcs_payload.empty())
                {
                    sample.topics().clear();
                    sample.topics().emplace_back("swamp_gcs");
                    sample.payload(last_swamp_gcs_payload);

                    // Print the payload before sending
                    std::cout << "Sending sample with swamp_gcs payload: ";
                    for (const auto& byte : sample.payload())
                    {
                        std::cout << std::hex << static_cast<int>(byte) << " ";
                    }
                    std::cout << std::endl;

                    writer_->write(&sample);
                    ++number_of_messages_sent;
                }

                if (!last_target_update_payload.empty())
                {
                    sample.topics().clear();
                    sample.topics().emplace_back("target_update");
                    sample.payload(last_target_update_payload);

                    // Print the payload before sending
                    std::cout << "Sending sample with target_update payload: ";
                    for (const auto& byte : sample.payload())
                    {
                        std::cout << std::hex << static_cast<int>(byte) << " ";
                    }
                    std::cout << std::endl;

                    writer_->write(&sample);
                    ++number_of_messages_sent;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::cout << "\nStopped" << std::endl;

    tshark_thread.join();
}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}

