#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastrtps/transport/UDPv4TransportDescriptor.h>
#include <fastrtps/types/TypesBase.h>
#include <iostream>
#include <vector>
#include <thread>
#include <signal.h>
#include <thread>
#include <iostream>
#include <vector>
#include "ShapeSubscriber.hpp"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <iostream>
#include <vector>
using namespace eprosima::fastdds::dds;
using eprosima::fastrtps::types::OctetSeq;

class RawDataSubscriber
{
public:
    RawDataSubscriber()
    {
        // Create Participant
        DomainParticipantQos participant_qos;
        participant_qos.transport().use_builtin_transports = false;

        eprosima::fastrtps::rtps::Locator_t multicast_locator;
        multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
        multicast_locator.port = 7900; // Multicast port
        participant_qos.transport().user_transports.push_back(udp_transport);
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

        // Create Topic
        topic_ = participant_->create_topic("swamp_gcs", "OctetSeq", TOPIC_QOS_DEFAULT);

        // Create Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        // Create DataReader
        DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
        reader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        reader_ = subscriber_->create_datareader(topic_, reader_qos, &listener_);
    }

    ~RawDataSubscriber()
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

private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataReader* reader_;

    class SubListener : public DataReaderListener
    {
    public:
        void on_data_available(DataReader* reader) override
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
    } listener_;
};
int main()
{
    RawDataSubscriber subscriber;
    std::cout << "Subscriber running. Waiting for data..." << std::endl;

    // Keep the subscriber running
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
    return 0;
}

