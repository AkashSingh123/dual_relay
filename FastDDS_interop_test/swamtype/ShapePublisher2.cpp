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
    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    datawriter_qos.history().depth = 1;

    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 2); // Multicast address
    multicast_locator.port = 7900; // Multicast port
    datawriter_qos.endpoint().multicast_locator_list.push_back(multicast_locator);

    if (publisher_ && topic_)
    {
        writer_ = publisher_->create_datawriter(topic_, datawriter_qos, &listener_);
    }

    if (writer_ && topic_ && publisher_ && participant_)
    {
        std::cout << "DataWriter created for the topic pos." << std::endl;
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

    NetboxMessage sample;

    // The payload you want to send
    std::vector<uint8_t> received_payload = {
        0xFB, 0x43, 0x04, 0x78, 0x05, 0x12, 0x01, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0xC8, 0x42,
        0x00, 0x00, 0x48, 0x43, 0x00, 0x00, 0x96, 0x43
    };

    sample.id(2);  // Set the ID to 2
    sample.timestamp(std::chrono::system_clock::now().time_since_epoch().count());  // Set the current timestamp
    sample.topics().push_back("pos");
    sample.payload() = received_payload;

    while (!publisher_stop::stop)
    {
        writer_->write(&sample);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapePublisher::SubscriberListener::on_publication_matched(DataWriter*, const PublicationMatchedStatus& info)
{
    matched = info.current_count;
    std::cout << "Number of matched readers: " << matched << std::endl;
}

