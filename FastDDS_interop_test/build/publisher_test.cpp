/*#include "ShapePublisher.hpp"
#include "ShapePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>

#include <unistd.h>
#include <signal.h>
#include <string>
#include <thread>

using namespace eprosima::fastdds::dds;

ShapePublisher::ShapePublisher() : participant_(nullptr), publisher_(nullptr), topic_(nullptr), writer_(nullptr), type_(new ShapeTypePubSubType()), listener_() {}

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

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

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
        topic_ = participant_->create_topic("Square", type_.get_type_name(), topic_qos);
    }

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;

    if (publisher_ && topic_)
    {
        writer_ = publisher_->create_datawriter(topic_, datawriter_qos, &listener_);
    }

    if (writer_ && topic_ && publisher_ && participant_)
    {
        std::cout << "DataWriter created for the topic Square." << std::endl;
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

void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    ShapeType sample;
    sample.color("BLUE");
    sample.shape_size(12);

    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            sample.x(10 * (number_of_messages_sent % 10));
            sample.y(10 * (number_of_messages_sent % 9));
            writer_->write(&sample);
            std::cout << "Sending sample " << number_of_messages_sent << std::endl;
            ++number_of_messages_sent;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter *, const PublicationMatchedStatus &info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}*/


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

    participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participant_qos);

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
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
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

// For handling stop signal to break the infinite loop
namespace publisher_stop
{
    volatile sig_atomic_t stop;
    void handle_interrupt(int)
    {
        stop = 1;
    }
}

/*void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.payload() = {42, 43, 44, 45}; // Example vector data
    sample.id = 2;
    sample.timesetamp = 
    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            writer_->write(&sample);
            std::cout << "Sending sample with data: ";
            for (const auto& byte : sample.payload())
            {
                std::cout << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            ++number_of_messages_sent;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}*/
/*void ShapePublisher::run()
{   

  signal(SIGINT, publisher_stop::handle_interrupt);

    // Execute tshark to capture data
            FILE* pipe = popen("tshark -i any -f \"udp\" -T fields -e data -Y \"udp.port == 7410\" -c 1", "r");
            if (!pipe)
            {
                std::cerr << "Failed to run tshark command." << std::endl;
                return;
            }

            char buffer[128];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
            {
                result += buffer;
            }
            pclose(pipe);


    // Read captured data
//    std::ifstream file("captured_data.txt");
    std::string line;
 //   std::ifstream file("captured_data.txt");

 //   signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    sample.topics().emplace_back("swamp_gcs");
    //sample.payload() = {58, 100, 7, 17, 1, 0, 0, 1, 2, 0, 0, 128, 63, 0, 16, 251, 69, 224, 239, 0, 66}; // Example vector data
    //sample.payload() = {25, 100, 7, 17, 1, 0, 0, 1, 2, 0, 0, 128, 63, 0, 16, 251, 69, 224, 239, 0, 66};
    sample.payload() = {0x25, 0x64, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x10, 0xFB, 0x45, 0xE0, 0xEF, 0x00, 0x42}; // Example vector data

    int number_of_messages_sent = 0;

   while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            writer_->write(&sample);
            std::cout << "Sending sample with data: ";
            for (const auto& byte : sample.payload())
            {
                std::cout << std::hex << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            ++number_of_messages_sent;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}*/

/*
void capture_tshark() //all rtps
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    while (!publisher_stop::stop)
    {
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);
            if (line.find("\"rtps") != std::string::npos)
            {
                result += line;
            }
        }

        if (!result.empty())
        {
            // Print captured data
            std::cout << "Captured data: " << result << std::endl;

            // Write captured data to file
            std::ofstream outfile("captured_data.txt", std::ios_base::app);
            if (outfile.is_open())
            {
                outfile << "Captured data: " << result << std::endl;
                outfile.close();
            }
            else
            {
                std::cerr << "Failed to open file for writing captured data." << std::endl;
            }
        }
    }
    pclose(pipe);
}

*/


/*
void capture_tshark() //no fileter on number of lines
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    bool found_issue_data = false;

    while (!publisher_stop::stop && !found_issue_data)
    {
        std::string result = "";
        bool capture_issue_data = false;
        int lines_since_entity_id = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);

            if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
            {
                capture_issue_data = true;
                lines_since_entity_id = 0;
            }

            if (capture_issue_data)
            {
                lines_since_entity_id++;
                if (lines_since_entity_id <= 15)
                {
                    if (line.find("\"rtps.issueData\": ") != std::string::npos)
                    {
                        result += line;

                        // Print captured data
                        std::cout << "Captured data: " << line << std::endl;

                        // Write captured data to file
                        std::ofstream outfile("captured_data.txt", std::ios_base::app);
                        if (outfile.is_open())
                        {
                            outfile << "Captured data: " << line << std::endl;
                            outfile.close();
                        }
                        else
                        {
                            std::cerr << "Failed to open file for writing captured data." << std::endl;
                        }
                    }
                }
                else
                {
                    capture_issue_data = false;
                }
            }
        }
    }
    pclose(pipe);
}

*/
/*
void capture_tshark()
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    bool found_issue_data = false;

    while (!publisher_stop::stop && !found_issue_data)
    {
        std::string result = "";
        bool capture_issue_data = false;
        int lines_since_entity_id = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);

            if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
            {
                capture_issue_data = true;
                lines_since_entity_id = 0;
            }

            if (capture_issue_data)
            {
                lines_since_entity_id++;
                if (lines_since_entity_id <= 15 && line.find("\"rtps.issueData\": ") != std::string::npos)
                {
                    result += line;
                    found_issue_data = true;
                    break;
                }

                if (lines_since_entity_id > 10)
                {
                    capture_issue_data = false;
                }
            }
        }

        if (!result.empty())
        {
            // Print captured data
            std::cout << "Captured data: " << result << std::endl;

            // Write captured data to file
            std::ofstream outfile("captured_data.txt", std::ios_base::app);
            if (outfile.is_open())
            {
                outfile << "Captured data: " << result << std::endl;
                outfile.close();
            }
            else
            {
                std::cerr << "Failed to open file for writing captured data." << std::endl;
            }
        }
    }
    pclose(pipe);
}
*/


/*
void capture_tshark()
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    while (!publisher_stop::stop)
    {
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);

            if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
            {
                // Print the line with the specific writer entity ID
                std::cout << line << std::endl;

                // Write the line to the file
                std::ofstream outfile("captured_data.txt", std::ios_base::app);
                if (outfile.is_open())
                {
                    outfile << line << std::endl;
                    outfile.close();
                }
                else
                {
                    std::cerr << "Failed to open file for writing captured data." << std::endl;
                }
            }
        }
    }
    pclose(pipe);
}

*/

/*void capture_tshark(NetboxMessage& sample)
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    while (!publisher_stop::stop)
    {
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);
            if (line.find("\"rtps.param.topicName\": \"swamp_gcs\"") != std::string::npos)
            {
                result += line;
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                {
                    line = buffer;
                    if (line.find("\"rtps.issueData\": ") != std::string::npos)
                    {
                        std::istringstream ss(line.substr(line.find(": ") + 2));
                        std::string byte_str;
                        std::vector<uint8_t> payload;

                        while (std::getline(ss, byte_str, ':'))
                        {
                            payload.push_back(static_cast<uint8_t>(std::stoul(byte_str, nullptr, 16)));
                        }
                        std::cout << "Captured payload: ";
                        for (const auto& byte : payload)
                        {
                            std::cout << std::hex << static_cast<int>(byte) << " ";
                        }
                        std::cout << std::endl;

                        sample.payload() = payload;
                        break;
                    }
                }
            }
        }

        if (!result.empty())
        {
            // Print captured data
            std::cout << "Captured data: " << result << std::endl;

            // Write captured data to file
            std::ofstream outfile("captured_data.txt", std::ios_base::app);
            if (outfile.is_open())
            {
                outfile << "Captured data: " << result << std::endl;
                outfile.close();
            }
            else
            {
                std::cerr << "Failed to open file for writing captured data." << std::endl;
            }
        }
    }
    pclose(pipe);
}*/

/*void ShapePublisher::run() // earliest main
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    sample.topics().emplace_back("swamp_gcs");
    sample.payload() = {0x25, 0x64, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x10, 0xFB, 0x45, 0xE0, 0xEF, 0x00, 0x42}; // Example vector data
    std::thread tshark_thread(capture_tshark);

    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            // Print the payload before sending
            std::cout << "Sending sample with payload: ";
            for (const auto& byte : sample.payload())
            {
                std::cout << std::hex << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;

            writer_->write(&sample);
            ++number_of_messages_sent;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;

    tshark_thread.join();
}*/

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
/*void capture_tshark()
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];

    while (!publisher_stop::stop)
    {
        std::string result = "";
        bool capture_issue_data = false;
        int lines_since_entity_id = 0;
        std::string topic;
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);

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

                        //result += hex_data;
                        captured_data = hex_data;
                        captured_topic = topic; // Set the captured topic
                        // Print captured data
                        
                        //added code 
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
                      // added code till here  
                        
                                
                        
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

                        // Set the captured data directly

                        //std::cout << "Updated captured_data: " << captured_data << std::endl;
                        std::cout << "Updated captured_topic: " << captured_topic << std::endl;
                        new_data = true;
                    }
                }
                else
                {
                    capture_issue_data = false;
                }
            }
        }
    }
    pclose(pipe);
}
*/

void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);
    //datawriter_qos.endpoint().unicast_locator_list.clear();
    NetboxMessage sample;
    /*sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
    sample.topics().emplace_back("swamp_gcs");
   // std::thread tshark_thread(capture_tshark); 
    //sample.payload() = {0x25, 0x64, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x10, 0xFB, 0x45, 0xE0, 0xEF, 0x00, 0x42}; // Example vector data
    //payload = {0x25, 0x64, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x10, 0xFB, 0x45, 0xE0, 0xEF, 0x00, 0x42};
    
    
    
        std::vector<uint8_t> received_payload = {
        0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C,0x00, 0x11, 0xFC, 0x41, 0x53, 0x06, 0x00, 0x64,
        0x01, 0x09, 0x01, 0x00, 0x00, 0x01, 0x00, 0x64,
        0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x00, 0x11, 0xFC, 0x42, 0x53,
        0x06, 0x00, 0x64, 0x01, 0x09, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x02,  0x00, 0x2C, 0x00, 0x01, 0x00, 0x64, 0xC8, 0x00, 0x2C, 0x02,  0x00, 0x2C, 0x00
      };
    
  */  
  
  
  std::vector<uint8_t> received_payload = {
    0x82, 0x01, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x73, 0x77, 0x61, 0x6D, 0x70, 0x5F, 0x67, 0x63, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0XB2, 0X18, 0X18, 0x91, 0x01, 0X00, 
    0x00, 0x19, 0x00, 0x00, 0X00, 0xFC, 0x5B, 0xA7, 0x01, 0X00, 0x0B, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 
    0x00, 0x00, 0X01, 0x02, 0X00, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0x42, 0X00, 0x40, 0x05, 0x44, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C
};

  
  
  
  
  
    sample.data() = received_payload;


    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        // if (listener_.matched)
        {       writer_->write(&sample);
                ++number_of_messages_sent;
            /*bool send_data = false;
            {
                // Locking is not necessary here since the flag is only set in one place and read in another
                if (new_data)
                {
                    send_data = true;
                    new_data = false; // Reset the flag
                }
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
            

          }*/
            
       }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "\nStopped" << std::endl;

  //  tshark_thread.join();
}


/*void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    std::thread tshark_thread(capture_tshark); 
    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {   std::vector<uint8_t> payload = hex_to_bytes(captured_data);
        if (listener_.matched)
        {
            std::cout << "Captured data to send: " << captured_data << std::endl;

            std::vector<uint8_t> payload;

            if (captured_topic == "command")
            {
                payload = hex_to_bytes(captured_data);
                if (payload.size() > 36)
                {
                    payload = std::vector<uint8_t>(payload.begin() + 36, payload.end());
                    last_command_payload_ = payload;
                }
                sample.topics().clear();
                sample.topics().emplace_back("command");
                sample.payload(last_command_payload_);
            }
            else if (captured_topic == "swamp_gcs")
            {
                payload = hex_to_bytes(captured_data);
                if (payload.size() > 44)
                {
                    payload = std::vector<uint8_t>(payload.begin() + 44, payload.end());
                    last_swamp_gcs_payload_ = payload;
                }
                sample.topics().clear();
                sample.topics().emplace_back("swamp_gcs");
                sample.payload(last_swamp_gcs_payload_);
            }
            else if (captured_topic == "target_update")
            {
                payload = hex_to_bytes(captured_data);
                if (payload.size() > 36)
                {
                    payload = std::vector<uint8_t>(payload.begin() + 36, payload.end());
                    last_target_update_payload_ = payload;
                }
                sample.topics().clear();
                sample.topics().emplace_back("target_update");
                sample.payload(last_target_update_payload_);
            }

            if (!payload.empty())
            {
                // Print the payload before sending
                std::cout << "Sending sample with payload: ";
                for (const auto& byte : sample.payload())
                {
                    std::cout << std::hex << static_cast<int>(byte) << " ";
                }
                std::cout << std::endl;

                writer_->write(&sample);
                ++number_of_messages_sent;
            }
            else
            {
                std::cout << "No captured data to send." << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;

    tshark_thread.join();
}
*/

















/*

void ShapePublisher::capture_tshark()   //kinda works the whole set but after close button
{
    FILE* pipe = popen("tshark -i any -Y rtps -T json", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run tshark command." << std::endl;
        return;
    }

    char buffer[4096];
    while (!publisher_stop::stop)
    {
        std::string result = "";
        bool capture_issue_data = false;
        int lines_since_entity_id = 0;

        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            std::string line(buffer);

            if (line.find("\"rtps.sm.wrEntityId\": \"0x00001602\"") != std::string::npos)
            {
                capture_issue_data = true;
                lines_since_entity_id = 0;
            }

            if (capture_issue_data)
            {
                lines_since_entity_id++;
                if (lines_since_entity_id <= 15 && line.find("\"rtps.issueData\": ") != std::string::npos)
                {
                    result += line;
                    capture_issue_data = false; // Stop capturing after finding the issue data
                }

                if (lines_since_entity_id > 15)
                {
                    capture_issue_data = false;
                }
            }
        }

        if (!result.empty())
        {
            // Print captured data
            std::cout << "Captured data: " << result << std::endl;

            // Write captured data to file
            std::ofstream outfile("captured_data.txt", std::ios_base::app);
            if (outfile.is_open())
            {
                outfile << "Captured data: " << result << std::endl;
                outfile.close();
            }
            else
            {
                std::cerr << "Failed to open file for writing captured data." << std::endl;
            }

            // Extract the actual issue data from the result
            std::string issue_data_hex = result.substr(result.find("\"rtps.issueData\": ") + 19);
            issue_data_hex.erase(issue_data_hex.find_last_of('\"'));

            std::vector<uint8_t> payload;
            for (size_t i = 0; i < issue_data_hex.length(); i += 3)
            {
                std::string byteString = issue_data_hex.substr(i, 2);
                uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
                payload.push_back(byte);
            }

            // Send the payload to the run function for publishing
            publish_data(payload);
        }
    }
    pclose(pipe);
}

void ShapePublisher::publish_data(const std::vector<uint8_t>& payload)
{
    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    sample.topics().emplace_back("swamp_gcs");
    sample.payload() = payload;

    if (listener_.matched)
    {
        writer_->write(&sample);
        std::cout << "Sending sample with data: ";
        for (const auto& byte : sample.payload())
        {
            std::cout << std::hex << static_cast<int>(byte) << " ";
        }
        std::cout << std::endl;
    }
}

void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

    std::thread tshark_thread(&ShapePublisher::capture_tshark, this);

    tshark_thread.join();
}


*/



/*

void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);

   // std::thread tshark_thread(capture_tshark);

    NetboxMessage sample;
    sample.id(2);
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());
    sample.topics().emplace_back("swamp_gcs");
    sample.payload() = {0x25, 0x64, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x80, 0x3F, 0x00, 0x10, 0xFB, 0x45, 0xE0, 0xEF, 0x00, 0x42}; // Example vector data

    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        if (listener_.matched)
        {
            writer_->write(&sample);
            std::cout << "Sending sample with data: ";
            for (const auto& byte : sample.payload())
            {
                std::cout << std::hex << static_cast<int>(byte) << " ";
            }
            std::cout << std::endl;
            ++number_of_messages_sent;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;

   // tshark_thread.join();
}*/

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}


