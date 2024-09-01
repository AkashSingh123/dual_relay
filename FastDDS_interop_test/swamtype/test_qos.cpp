#include "ShapePublisher.hpp"
#include "NetboxMessage1PubSubTypes.h"
#include <fastdds/rtps/common/Locator.h>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
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

ShapePublisher::ShapePublisher() : participant_(nullptr), publisher_(nullptr), topic_(nullptr), writer_(nullptr), type_(new NetboxMessage1PubSubType()), listener_() {}

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
    //topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    //topic_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    //topic_qos.destination_order().kind = BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    //topic_qos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    //topic_qos.history().depth = 100;  // Set to a high number or as per your needs

    if (participant_)
    {
        topic_ = participant_->create_topic("7", type_.get_type_name(), topic_qos);
    }

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
   // Set History to KeepAll
//    datawriter_qos.history().kind = eprosima::fastdds::dds::KEEP_ALL_HISTORY_QOS;

    datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS; 
    datawriter_qos.history().depth = 100;  // Set to a high number or as per your needs

    datawriter_qos.resource_limits().max_samples = 10000; // Set according to your needs
    datawriter_qos.resource_limits().max_instances = 1;   // Set according to your needs
    datawriter_qos.resource_limits().max_samples_per_instance = 10000; // Set according to your needs

    // Set Durability to Volatile (equivalent to None)
    //datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;


    // Set Deadline period to None (infinite)
    datawriter_qos.deadline().period = eprosima::fastrtps::c_TimeInfinite;  // No deadline constraint


     // Use default Latency Budget (None)
    //datawriter_qos.latency_budget().duration = eprosima::fastrtps::Duration_t(0, 1);

    // Use default Ownership (None)
    //datawriter_qos.ownership().kind = SHARED_OWNERSHIP_QOS;

    // Use default Liveliness (None)
    //datawriter_qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    //datawriter_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite;  // Default setting


   // Set Reliability to Reliable with `max_blocking_time = 1 sec`
   datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
   datawriter_qos.reliability().max_blocking_time = eprosima::fastrtps::Duration_t(1, 0);  // 1 second max blocking time

   // Set Destination Order to BySourceTimestamp
   //datawriter_qos.destination_order().kind = BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;





    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
    multicast_locator.port = 7900; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);

    if (publisher_ && topic_)
    {
        writer_ = publisher_->create_datawriter(topic_, datawriter_qos, &listener_);
    }

    if (writer_ && topic_ && publisher_ && participant_)
    {
        std::cout << "DataWriter created for the topic 7." << std::endl;
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

    NetboxMessage1 sample;

    // The payload you want to send
    /*std::vector<uint8_t> received_payload = {0x00, 0x18, 0xFB, 0xBC, 0x19, 0x64, 0x05, 0x12, 
                                    0x01, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 
                                    0xC8, 0x42, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 
                                    0x96, 0x43};*/
  
  std::vector<uint8_t> received_payload = {
    0x00, 0x18, 0xFB, 0x26, 0x08, 0x82, 0x05, 0x12, 
    0x01, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 
    0xC8, 0x42, 0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 
    0x96, 0x43
};



    sample.id(2);  // Set the ID to 2
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());  // Set the current timestamp
    sample.topics().push_back("swamp_gcs");
    sample.payload() = received_payload;

    while (!publisher_stop::stop)
    {
        writer_->write(&sample);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}

