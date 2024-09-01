

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



void ShapePublisher::run()
{
    signal(SIGINT, publisher_stop::handle_interrupt);
    //datawriter_qos.endpoint().unicast_locator_list.clear();
    NetboxMessage sample;
  
  std::vector<uint8_t> received_payload = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x73, 0x77, 0x61, 0x6D, 0x70, 0x5F, 0x67, 0x63, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0XB2, 0X18, 0X18, 0x91, 0x01, 0X00, 
    0x00, 0x19, 0x00, 0x00, 0X00, 0xFC, 0x5B, 0xA7, 0x01, 0X00, 0x0B, 0x07, 0x11, 0x01, 0x00, 0x00, 0x01, 0x02, 
    0x00, 0x00, 0X01, 0x02, 0X00, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0x42, 0X00, 0x40, 0x05, 0x44,  
    0x00, 0x00, 0X01, 0x02, 0X00, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0x42, 0X00, 0x40, 0x05, 0x44, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0X00, 0X50, 0x41, 0x00, 0x00, 0x0C, 0X00
};

  
  
  
  
  
    sample.payload() = received_payload;


    int number_of_messages_sent = 0;

    while (!publisher_stop::stop)
    {
        // if (listener_.matched)
        {       writer_->write(&sample);
                ++number_of_messages_sent;
            
       }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "\nStopped" << std::endl;

  //  tshark_thread.join();
}



















void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}


