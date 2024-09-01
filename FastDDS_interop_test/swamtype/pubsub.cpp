#include "ShapePublisher.hpp"
#include "ShapeSubscriber.hpp"
#include "NetboxMessagePubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/rtps/common/Locator.h>
#include <signal.h>
#include <thread>
#include <iostream>

using namespace eprosima::fastdds::dds;

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *pub_participant_;
    DomainParticipant *sub_participant_;
    Publisher *publisher_;
    Subscriber *subscriber_;
    Topic *topic_;
    DataReader *reader_;
    DataWriter *writer_;
    TypeSupport type_;

    class PubSubListener : public DataReaderListener, public DataWriterListener
    {
    public:
        PubSubListener(DataWriter* writer) : writer_(writer), matched(0), received_samples(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage sample;
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK)
            {
                ++received_samples;
                std::cout << "Message received, ID: " << sample.id() << std::endl;
                writer_->write(&sample);
            }
            else
            {
                std::cout << "Read failed with error code." << std::endl;
            }
        }

        void on_publication_matched(DataWriter *writer, const PublicationMatchedStatus &info) override
        {
            matched = info.current_count;
            std::cout << "Number of matched readers: " << matched << std::endl;
        }

        DataWriter* writer_;
        int matched;
        uint32_t received_samples;
    } listener_;

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() : 
    pub_participant_(nullptr), sub_participant_(nullptr),
    publisher_(nullptr), subscriber_(nullptr),
    topic_(nullptr), reader_(nullptr), writer_(nullptr), 
    type_(new NetboxMessagePubSubType()),
    listener_(nullptr) {}

CombinedPubSub::~CombinedPubSub()
{
    if (reader_ && subscriber_)
    {
        subscriber_->delete_datareader(reader_);
    }

    if (subscriber_ && sub_participant_)
    {
        sub_participant_->delete_subscriber(subscriber_);
    }

    if (topic_ && pub_participant_)
    {
        pub_participant_->delete_topic(topic_);
    }

    if (publisher_ && pub_participant_)
    {
        pub_participant_->delete_publisher(publisher_);
    }

    if (pub_participant_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant_);
    }

    if (sub_participant_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(sub_participant_);
    }
}


bool CombinedPubSub::init()
{
    // Initialize the Subscriber side
    DomainParticipantQos sub_participant_qos;
    sub_participant_qos.name("subscriber_participant");

    sub_participant_ = DomainParticipantFactory::get_instance()->create_participant(7, sub_participant_qos);
    if (!sub_participant_)
        return false;

    type_.register_type(sub_participant_);

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    datareader_qos.latency_budget().duration = eprosima::fastrtps::Duration_t(0, 0); // Minimal latency budget
    // Configure multicast settings for the Subscriber
    
    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
    sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, 239, 255, 0, 7);
    sub_multicast_locator.port = 7900;
    datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    subscriber_qos.entity_factory().autoenable_created_entities = true;
    subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;
    
    subscriber_ = sub_participant_->create_subscriber(subscriber_qos);
    if (!subscriber_)
        return false;

    topic_ = sub_participant_->create_topic("1", type_.get_type_name(), TOPIC_QOS_DEFAULT);
    if (!topic_)
        return false;

    listener_ = PubSubListener(nullptr);  // Temporary until we set the writer

    reader_ = subscriber_->create_datareader(topic_, datareader_qos, &listener_);
    if (!reader_)
        return false;

    // Initialize the Publisher side
    DomainParticipantQos pub_participant_qos;
    pub_participant_qos.name("publisher_participant");

    pub_participant_ = DomainParticipantFactory::get_instance()->create_participant(1, pub_participant_qos);
    if (!pub_participant_)
        return false;

    type_.register_type(pub_participant_);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
    publisher_ = pub_participant_->create_publisher(publisher_qos);
    if (!publisher_)
        return false;

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    topic_qos.history().kind = KEEP_ALL_HISTORY_QOS;  // Keep all history

    Topic *pub_topic = pub_participant_->create_topic("1", type_.get_type_name(), topic_qos);
    if (!pub_topic)
        return false;

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Configure multicast settings for the Publisher
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
    pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, 239, 255, 0, 1);
    pub_multicast_locator.port = 7900;
    datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

    writer_ = publisher_->create_datawriter(pub_topic, datawriter_qos, &listener_);
    if (!writer_)
        return false;

    listener_.writer_ = writer_;  // Now that the writer is set, update the listener

    return true;
}


void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal)
    {
        // Keep running until interrupted
    }
    std::cout << "\nStopped" << std::endl;
}

int main()
{
    CombinedPubSub combined_pubsub;
    if (combined_pubsub.init())
    {
        combined_pubsub.run();
    }
    else
    {
        std::cerr << "Failed to initialize CombinedPubSub." << std::endl;
        return 1;
    }

    return 0;
}

