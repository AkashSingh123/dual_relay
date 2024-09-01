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
#include <mutex>

using namespace eprosima::fastdds::dds;
std::mutex mtx;  // Mutex for thread-safe access to the deque

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *pub_participant1_;
    DomainParticipant *sub_participant1_;
    Publisher *publisher1_;
    Subscriber *subscriber1_;
    Topic *topic1_;
    DataReader *reader1_;
    DataWriter *writer1_;
    TypeSupport type1_;

    class PubSubListener : public DataReaderListener, public DataWriterListener
    {
    public:
        PubSubListener(DataWriter* writer) : writer1_(writer), matched(0), received_samples(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage1 sample; // Refer to Type1 NetboxMessage
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK)
            {
                ++received_samples;
                std::cout << "Message received, ID: " << sample.id() << " | Total messages received: " << received_samples << std::endl;
             //  std::this_thread::sleep_for(std::chrono::milliseconds(80));
                 std::lock_guard<std::mutex> lock(mtx);

                writer1_->write(&sample);
                std::cout << "messages sent: " << std::endl;
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
        int matched;
        uint32_t received_samples;
    } listener1_;

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() : 
    pub_participant1_(nullptr), sub_participant1_(nullptr),
    publisher1_(nullptr), subscriber1_(nullptr),
    topic1_(nullptr), reader1_(nullptr), writer1_(nullptr), 
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

    if (topic1_ && pub_participant1_)
    {
        pub_participant1_->delete_topic(topic1_);
    }

    if (publisher1_ && pub_participant1_)
    {
        pub_participant1_->delete_publisher(publisher1_);
    }

    if (pub_participant1_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(pub_participant1_);
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
    datareader_qos.history().depth = 1;
    //datareader_qos.latency_budget().duration = {0, 0}; // Zero latency


    //datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
   //datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite; // Set an appropriate lease duration
    datareader_qos.resource_limits().max_samples = 3000000000; // Set a high number to accumulate more data
    datareader_qos.resource_limits().max_instances = 11;  // Adjust as needed
    datareader_qos.resource_limits().max_samples_per_instance = 13542;

    //datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    //datareader_qos.resource_limits().max_samples = 5000000; // Example high limit
    //datareader_qos.resource_limits().max_instances = 10;  // Example high limit
    //datareader_qos.resource_limits().max_samples_per_instance = 400; // Example high limit
    //datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    //datareader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    //datareader_qos.deadline().period = eprosima::fastrtps::Duration_t(2, 0); // 2 seconds
    //datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    //datareader_qos.history().depth = 1; // Store the last 10 samples
   /* datareader_qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite;
    datareader_qos.latency_budget().duration = eprosima::fastrtps::c_TimeInfinite; // Minimal latency budget*/


    //datareader_qos.destination_order().kind = BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    //datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    //datareader_qos.history().depth = 1;
    //datareader_qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;  // Match Automatic liveliness
    //datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite;  // Set to infinite or specific duration
    //datareader_qos.deadline().period = eprosima::fastrtps::c_TimeInfinite;  // Match infinite deadline
    //datareader_qos.deadline().period = 2;  
    
    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
   /* subscriber_qos.entity_factory().autoenable_created_entities = true;
   subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;*/


    datareader_qos.endpoint().unicast_locator_list.clear();
    
    eprosima::fastrtps::rtps::Locator_t unicast_locator;
    unicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, "192.168.234.29"); // Own IP address
    unicast_locator.port = 7663; // Set the appropriate port number
    datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);
    
    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator1;
    sub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator1, 239, 255, 0, 1);
    sub_multicast_locator1.port = 9151;
    datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator1);

    //SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    /*subscriber_qos.entity_factory().autoenable_created_entities = true;
    subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;*/
    
    subscriber1_ = sub_participant1_->create_subscriber(subscriber_qos);
    if (!subscriber1_)
        return false;
    TopicQos topic_qos1 = TOPIC_QOS_DEFAULT;
    topic_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    topic1_ = sub_participant1_->create_topic("command", type1_.get_type_name(), topic_qos1);
    if (!topic1_)
        return false;


    listener1_ = PubSubListener(nullptr);  // Temporary until we set the writer

    reader1_ = subscriber1_->create_datareader(topic1_, datareader_qos, &listener1_);
    if (!reader1_)
        return false;                
        



    // Initialize the Publisher side
    DomainParticipantQos pub_participant_qos;
    pub_participant_qos.name("publisher_participant");

    pub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(7, pub_participant_qos);
    if (!pub_participant1_)
        return false;

    type1_.register_type(pub_participant1_);

    PublisherQos publisher_qos = PUBLISHER_QOS_DEFAULT;
    publisher1_ = pub_participant1_->create_publisher(publisher_qos);
    if (!publisher1_)
        return false;

        
        
   
    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
   // topic_qos.history().kind = KEEP_ALL_HISTORY_QOS;  // Keep all history

    Topic *pub_topic1 = pub_participant1_->create_topic("command", type1_.get_type_name(), topic_qos);
    if (!pub_topic1)
        return false;

    DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    //datawriter_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;  // Keep all history
    datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

    // Set heartbeat period to 1 second (can be adjusted as needed)
    datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

     // Set nack response delay to a low value for quick response
    datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds 
    
    
    datawriter_qos.resource_limits().max_samples = 3000000000000; // Example high limit
    datawriter_qos.resource_limits().max_instances = 11100;    // Example high limit
    datawriter_qos.resource_limits().max_samples_per_instance = 13542; // Example high limit
 
    
    
    // Configure multicast settings for the Publisher
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator1;
    pub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator1, 239, 255, 0, 7);
    pub_multicast_locator1.port = 9154;
    datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator1);

    writer1_ = publisher1_->create_datawriter(pub_topic1, datawriter_qos, &listener1_);
    if (!writer1_)
        return false;

    listener1_.writer1_ = writer1_;  // Now that the writer is set, update the listener

    return true;
}


void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal)
    {
       // std::this_thread::sleep_for(std::chrono::milliseconds(80));

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
	
