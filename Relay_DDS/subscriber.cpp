#include "subscriber.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/types/TypesBase.h>
#include <fastrtps/rtps/common/Locator.h>
#include <iostream>
#include <vector>
#include <thread>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

using namespace eprosima::fastdds::dds;
using eprosima::fastrtps::types::OctetSeq;

ShapeSubscriber::ShapeSubscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
{
}

ShapeSubscriber::~ShapeSubscriber()
{
    if (reader_ != nullptr)
    {
        subscriber_->delete_datareader(reader_);
    }
    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }
    if (participant_ != nullptr)
    {
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
}

bool ShapeSubscriber::init(bool with_security)
{
    // Create Participant
    DomainParticipantQos participant_qos;
    participant_qos.transport().use_builtin_transports = false;
    auto udp_transport = std::make_shared<eprosima::fastrtps::rtps::UDPv4TransportDescriptor>();
    udp_transport->interfaceWhiteList.push_back("239.255.0.1"); // Multicast address
    participant_qos.transport().user_transports.push_back(udp_transport);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant_ == nullptr)
    {
        return false;
    }

    // Create Topic
    topic_ = participant_->create_topic("swamp_gcs", "OctetSeq", TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }

    // Create Subscriber
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    // Create DataReader
    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    reader_ = subscriber_->create_datareader(topic_, reader_qos, &listener_);

    if (reader_ == nullptr)
    {
        return false;
    }

    return true;
}

void ShapeSubscriber::run()
{
    std::cout << "Subscriber running. Waiting for data..." << std::endl;
    std::this_thread::sleep_for(std::chrono::hours(1)); // Keep the subscriber running
}

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    OctetSeq raw_data;
    SampleInfo info;

    while (reader->take_next_sample(&raw_data, &info) == ReturnCode_t::RETCODE_OK)
    {
        if (info.valid_data)
        {
            std::cout << "Received raw data: ";
            for (auto byte : raw_data)
            {
                std::cout << std::hex << static_cast<int>(byte) << " ";
            }
            std::cout << std::dec << std::endl;
        }
    }
}

void ShapeSubscriber::SubscriberListener::on_subscription_matched(DataReader* reader, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched++;
        std::cout << "Matched a subscriber" << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched--;
        std::cout << "Unmatched a subscriber" << std::endl;
    }
    else
    {
        std::cout << "Subscription matched status changed" << std::endl;
    }
}


