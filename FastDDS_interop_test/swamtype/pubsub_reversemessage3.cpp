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

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *pub_participant1_, *pub_participant2_, *pub_participant3_;
    DomainParticipant *sub_participant1_, *sub_participant2_;
    Publisher *publisher1_, *publisher2_, *publisher3_;
    Subscriber *subscriber1_, *subscriber2_;
    Topic *topic1_, *topic2_;
    Topic *pub_topic1_, *pub_topic2_, *pub_topic3_;
    DataReader *reader1_, *reader2_;
    DataWriter *writer1_, *writer2_, *writer3_;
    TypeSupport type1_;

    class PubSubListener : public DataReaderListener, public DataWriterListener
    {
    public:
        PubSubListener(DataWriter* writer1 = nullptr, DataWriter* writer2 = nullptr, DataWriter* writer3 = nullptr) 
            : writer1_(writer1), writer2_(writer2), writer3_(writer3), matched(0), received_samples(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage1 sample; // Refer to Type1 NetboxMessage
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK)
            {
                ++received_samples;
                std::cout << "Message received, ID: " << sample.id() << " | Total messages received: " << received_samples << std::endl;

                // Write the received sample to all writers
                if (writer1_) writer1_->write(&sample);
                if (writer2_) writer2_->write(&sample);
                if (writer3_) writer3_->write(&sample);
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
        DataWriter* writer3_;
        int matched;
        uint32_t received_samples;
    } listener1_, listener2_;

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() : 
    pub_participant1_(nullptr), pub_participant2_(nullptr), pub_participant3_(nullptr),
    sub_participant1_(nullptr), sub_participant2_(nullptr),
    publisher1_(nullptr), publisher2_(nullptr), publisher3_(nullptr),
    subscriber1_(nullptr), subscriber2_(nullptr),
    topic1_(nullptr), topic2_(nullptr),
    pub_topic1_(nullptr), pub_topic2_(nullptr), pub_topic3_(nullptr),
    reader1_(nullptr), reader2_(nullptr),
    writer1_(nullptr), writer2_(nullptr), writer3_(nullptr), 
    type1_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    if (reader1_ && subscriber1_)
    {
        subscriber1_->delete_datareader(reader1_);
    }
    if (reader2_ && subscriber2_)
    {
        subscriber2_->delete_datareader(reader2_);
    }

    if (subscriber1_ && sub_participant1_)
    {
        sub_participant1_->delete_subscriber(subscriber1_);
    }
    if (subscriber2_ && sub_participant2_)
    {
        sub_participant2_->delete_subscriber(subscriber2_);
    }

    if (topic1_ && sub_participant1_)
    {
        sub_participant1_->delete_topic(topic1_);
    }
    if (topic2_ && sub_participant2_)
    {
        sub_participant2_->delete_topic(topic2_);
    }

    if (publisher1_ && pub_participant1_)
    {
        pub_participant1_->delete_publisher(publisher1_);
    }
    if (publisher2_ && pub_participant2_)
    {
        pub_participant2_->delete_publisher(publisher2_);
    }
    if (publisher3_ && pub_participant3_)
    {
        pub_participant3_->delete_publisher(publisher3_);
    }

    if (pub_topic1_ && pub_participant1_)
    {
        pub_participant1_->delete_topic(pub_topic1_);
    }
    if (pub_topic2_ && pub_participant2_)
    {
        pub_participant2_->delete_topic(pub_topic2_);
    }
    if (pub_topic3_ && pub_participant3_)
    {
        pub_participant3_->delete_topic(pub_topic3_);
    }

    if (pub_participant1_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant1_);
    }
    if (pub_participant2_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant2_);
    }
    if (pub_participant3_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant3_);
    }

    if (sub_participant1_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(sub_participant1_);
    }
    if (sub_participant2_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(sub_participant2_);
    }
}

bool CombinedPubSub::init()
{
    // Initialize the first Subscriber side
    DomainParticipantQos sub_participant_qos1;
    sub_participant_qos1.name("subscriber_participant1");

    sub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(1, sub_participant_qos1);
    if (!sub_participant1_)
        return false;

    type1_.register_type(sub_participant1_);

    DataReaderQos datareader_qos1 = DATAREADER_QOS_DEFAULT;
    datareader_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos1.history().kind = KEEP_LAST_HISTORY_QOS;
    datareader_qos1.durability().kind = VOLATILE_DURABILITY_QOS;

    datareader_qos1.endpoint().unicast_locator_list.clear();

    eprosima::fastrtps::rtps::Locator_t unicast_locator1;
    unicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator1, "192.168.50.29"); // Own IP address
    unicast_locator1.port = 7663; // Set the appropriate port number
    datareader_qos1.endpoint().unicast_locator_list.push_back(unicast_locator1);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator1;
    sub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator1, 239, 255, 0, 1);
    sub_multicast_locator1.port = 7900;
    datareader_qos1.endpoint().multicast_locator_list.push_back(sub_multicast_locator1);

    subscriber1_ = sub_participant1_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!subscriber1_)
        return false;
        
    TopicQos topic_qos1 = TOPIC_QOS_DEFAULT;
    topic_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;     

    topic1_ = sub_participant1_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!topic1_)
        return false;

    listener1_ = PubSubListener();  // Initialize listener for the first reader

    reader1_ = subscriber1_->create_datareader(topic1_, datareader_qos1, &listener1_);
    if (!reader1_)
        return false;                

    // Initialize the second Subscriber side
    DomainParticipantQos sub_participant_qos2;
    sub_participant_qos2.name("subscriber_participant2");

    sub_participant2_ = DomainParticipantFactory::get_instance()->create_participant(7, sub_participant_qos2);
    if (!sub_participant2_)
        return false;

    type1_.register_type(sub_participant2_);

    DataReaderQos datareader_qos2 = DATAREADER_QOS_DEFAULT;
    datareader_qos2.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos2.history().kind = KEEP_LAST_HISTORY_QOS;
    datareader_qos2.durability().kind = VOLATILE_DURABILITY_QOS;

    datareader_qos2.endpoint().unicast_locator_list.clear();

    eprosima::fastrtps::rtps::Locator_t unicast_locator2;
    unicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator2, "192.168.50.29"); // Own IP address
    unicast_locator2.port = 7664; // Set a different port number
    datareader_qos2.endpoint().unicast_locator_list.push_back(unicast_locator2);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator2;
    sub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator2, 239, 255, 0, 7);
    sub_multicast_locator2.port = 7900;
    datareader_qos2.endpoint().multicast_locator_list.push_back(sub_multicast_locator2);

    subscriber2_ = sub_participant2_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!subscriber2_)
        return false;

    topic2_ = sub_participant2_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!topic2_)
        return false;

    listener2_ = PubSubListener();  // Initialize listener for the second reader

    reader2_ = subscriber2_->create_datareader(topic2_, datareader_qos2, &listener2_);
    if (!reader2_)
        return false;

    // Initialize the first Publisher side for domain 8
    DomainParticipantQos pub_participant_qos1;
    pub_participant_qos1.name("publisher_participant1");

    pub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(8, pub_participant_qos1);
    if (!pub_participant1_)
        return false;

    type1_.register_type(pub_participant1_);

    publisher1_ = pub_participant1_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!publisher1_)
        return false;
        
        
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    
    pub_topic1_ = pub_participant1_->create_topic("pos", type1_.get_type_name(), topic_qos);
    if (!pub_topic1_)
        return false;

    DataWriterQos datawriter_qos1 = DATAWRITER_QOS_DEFAULT;
    datawriter_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;

    datawriter_qos1.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

    // Set heartbeat period to 1 second (can be adjusted as needed)
    datawriter_qos1.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

     // Set nack response delay to a low value for quick response
    datawriter_qos1.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds */



    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator1;
    pub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator1, 239, 255, 0, 8);
    pub_multicast_locator1.port = 7900;
    datawriter_qos1.endpoint().multicast_locator_list.push_back(pub_multicast_locator1);

    writer1_ = publisher1_->create_datawriter(pub_topic1_, datawriter_qos1, &listener1_);
    if (!writer1_)
        return false;

    // Initialize the second Publisher side for domain 7
    DomainParticipantQos pub_participant_qos2;
    pub_participant_qos2.name("publisher_participant2");

    pub_participant2_ = DomainParticipantFactory::get_instance()->create_participant(7, pub_participant_qos2);
    if (!pub_participant2_)
        return false;

    type1_.register_type(pub_participant2_);

    publisher2_ = pub_participant2_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!publisher2_)
        return false;

    pub_topic2_ = pub_participant2_->create_topic("pos", type1_.get_type_name(), topic_qos);
    if (!pub_topic2_)
        return false;

    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator2;
    pub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator2, 239, 255, 0, 7);
    pub_multicast_locator2.port = 7900;
    datawriter_qos1.endpoint().multicast_locator_list.push_back(pub_multicast_locator2);

    writer2_ = publisher2_->create_datawriter(pub_topic2_, datawriter_qos1, &listener1_);
    if (!writer2_)
        return false;

    // Initialize the third Publisher side for domain 1
    DomainParticipantQos pub_participant_qos3;
    pub_participant_qos3.name("publisher_participant3");

    pub_participant3_ = DomainParticipantFactory::get_instance()->create_participant(1, pub_participant_qos3);
    if (!pub_participant3_)
        return false;

    type1_.register_type(pub_participant3_);

    publisher3_ = pub_participant3_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!publisher3_)
        return false;

    pub_topic3_ = pub_participant3_->create_topic("pos", type1_.get_type_name(), topic_qos);
    if (!pub_topic3_)
        return false;

    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator3;
    pub_multicast_locator3.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator3, 239, 255, 0, 1);
    pub_multicast_locator3.port = 7900;
    datawriter_qos1.endpoint().multicast_locator_list.push_back(pub_multicast_locator3);

    writer3_ = publisher3_->create_datawriter(pub_topic3_, datawriter_qos1, &listener2_);
    if (!writer3_)
        return false;

    listener1_.writer1_ = writer1_;  // Set the writer for domain 8
    listener1_.writer2_ = writer2_;  // Set the writer for domain 7

    listener2_.writer3_ = writer3_;  // Set the writer for domain 1

    return true;
}

void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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

