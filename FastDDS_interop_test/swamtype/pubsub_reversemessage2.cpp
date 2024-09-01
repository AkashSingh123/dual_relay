#include "ShapePublisher.hpp"
#include "ShapeSubscriber.hpp"
#include "NetboxMessage1PubSubTypes.h" // Include Type 1
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
#include "NetboxMessage2PubSubTypes.h" // Include Type 1
using namespace eprosima::fastdds::dds;
//using namespace Type1; // Use Type1 namespace for NetboxMessage

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *pub_participant1_;
    DomainParticipant *pub_participant2_;
    DomainParticipant *sub_participant1_;
    Publisher *publisher1_;
    Publisher *publisher2_;
    Subscriber *subscriber1_;
    Topic *topic1_;
    Topic *pub_topic1_;
    Topic *pub_topic2_;
    DataReader *reader1_;
    DataWriter *writer1_;
    DataWriter *writer2_;
    TypeSupport type1_;

    class PubSubListener : public DataReaderListener, public DataWriterListener
    {
    public:
    PubSubListener(DataWriter* writer1 = nullptr, DataWriter* writer2 = nullptr) 
        : writer1_(writer1), writer2_(writer2), matched(0), received_samples(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage1 sample; // Refer to Type1 NetboxMessage
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK)
            {
                ++received_samples;
                std::cout << "Message received, ID: " << sample.id() << " | Total messages received: " << received_samples << std::endl;

                // Write the received sample to both writers
                writer1_->write(&sample);
                writer2_->write(&sample);
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

        DataWriter* writer1_;
        DataWriter* writer2_;
        int matched;
        uint32_t received_samples;
    } listener1_;

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() : 
    pub_participant1_(nullptr), pub_participant2_(nullptr), sub_participant1_(nullptr),
    publisher1_(nullptr), publisher2_(nullptr), subscriber1_(nullptr),
    topic1_(nullptr), pub_topic1_(nullptr), pub_topic2_(nullptr),
    reader1_(nullptr), writer1_(nullptr), writer2_(nullptr), 
    type1_(new NetboxMessage1PubSubType()), // Use Type1 TypeSupport
    listener1_(nullptr) {}

CombinedPubSub::~CombinedPubSub()
{
    if (reader1_ && subscriber1_)
    {
        subscriber1_->delete_datareader(reader1_);
    }

    if (subscriber1_ && sub_participant1_)
    {
        sub_participant1_->delete_subscriber(subscriber1_);
    }

    if (topic1_ && sub_participant1_)
    {
        sub_participant1_->delete_topic(topic1_);
    }

    if (publisher1_ && pub_participant1_)
    {
        pub_participant1_->delete_publisher(publisher1_);
    }

    if (pub_topic1_ && pub_participant1_)
    {
        pub_participant1_->delete_topic(pub_topic1_);
    }

    if (publisher2_ && pub_participant2_)
    {
        pub_participant2_->delete_publisher(publisher2_);
    }

    if (pub_topic2_ && pub_participant2_)
    {
        pub_participant2_->delete_topic(pub_topic2_);
    }

    if (pub_participant1_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant1_);
    }

    if (pub_participant2_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant2_);
    }

    if (sub_participant1_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(sub_participant1_);
    }
}



bool CombinedPubSub::init()
{
    // Initialize the Subscriber side
    DomainParticipantQos sub_participant_qos;
    sub_participant_qos.name("subscriber_participant2");

    sub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(1, sub_participant_qos);
    if (!sub_participant1_)
        return false;

    type1_.register_type(sub_participant1_);

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;

    datareader_qos.endpoint().unicast_locator_list.clear();

    eprosima::fastrtps::rtps::Locator_t unicast_locator1;
    unicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator1, "192.168.1.52"); // Own IP address
    unicast_locator1.port = 7663; // Set the appropriate port number
    datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator1);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator1;
    sub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator1, 239, 255, 0, 1);
    sub_multicast_locator1.port = 7900;
    datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator1);

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    
    subscriber1_ = sub_participant1_->create_subscriber(subscriber_qos);
    if (!subscriber1_)
        return false;

    TopicQos topic_qos1 = TOPIC_QOS_DEFAULT;
    topic_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    topic1_ = sub_participant1_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!topic1_)
        return false;

    listener1_ = PubSubListener();    // Temporary until we set the writers

    reader1_ = subscriber1_->create_datareader(topic1_, datareader_qos, &listener1_);
    if (!reader1_)
        return false;                

    // Initialize the Publisher side for domain 8
    DomainParticipantQos pub_participant_qos1;
    pub_participant_qos1.name("publisher_participant1");

    pub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(8, pub_participant_qos1);
    if (!pub_participant1_)
        return false;

    type1_.register_type(pub_participant1_);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
    publisher1_ = pub_participant1_->create_publisher(publisher_qos);
    if (!publisher1_)
        return false;

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    pub_topic1_ = pub_participant1_->create_topic("pos", type1_.get_type_name(), topic_qos);
    if (!pub_topic1_)
        return false;

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator1;
    pub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator1, 239, 255, 0, 8);
    pub_multicast_locator1.port = 7900;
    datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator1);

    writer1_ = publisher1_->create_datawriter(pub_topic1_, datawriter_qos, &listener1_);
    if (!writer1_)
        return false;

    // Initialize the Publisher side for domain 7
    DomainParticipantQos pub_participant_qos2;
    pub_participant_qos2.name("publisher_participant2");

    pub_participant2_ = DomainParticipantFactory::get_instance()->create_participant(7, pub_participant_qos2);
    if (!pub_participant2_)
        return false;

    type1_.register_type(pub_participant2_);

    publisher2_ = pub_participant2_->create_publisher(publisher_qos);
    if (!publisher2_)
        return false;

    pub_topic2_ = pub_participant2_->create_topic("pos", type1_.get_type_name(), topic_qos);
    if (!pub_topic2_)
        return false;

    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator2;
    pub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator2, 239, 255, 0, 7);
    pub_multicast_locator2.port = 7900;
    datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator2);

    writer2_ = publisher2_->create_datawriter(pub_topic2_, datawriter_qos, &listener1_);
    if (!writer2_)
        return false;

    listener1_.writer1_ = writer1_;  // Now that the first writer is set, update the listener
    listener1_.writer2_ = writer2_;  // Now that the second writer is set, update the listener

    return true;
}

void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));

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
	
