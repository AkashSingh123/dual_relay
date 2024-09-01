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
#include <deque>
#include <mutex>

std::mutex mtx;  // Mutex for thread-safe access to the deque
std::deque<NetboxMessage1> message_queue;  // Deque to store messages

using namespace eprosima::fastdds::dds;

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *pub_participant1_, *pub_participant2_;
    DomainParticipant *sub_participant1_, *sub_participant2_;
    Publisher *publisher1_, *publisher2_;
    Subscriber *subscriber1_, *subscriber2_;
    Topic *topic1_, *topic2_;
    Topic *pub_topic1_, *pub_topic2_;
    DataReader *reader1_, *reader2_;
    DataWriter *writer1_, *writer2_;
    TypeSupport type1_;
    
    
    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(DataReader* reader, DataWriter* writer, int msg_id)
            : reader_(reader), writer_(writer), msg_id_(msg_id), received_samples(0) {}

        void on_data_available(DataReader *reader) override
        {
            NetboxMessage1 sample; // Refer to Type1 NetboxMessage
            SampleInfo info;
            ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

            if (return_code == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
                if (sample.id() != 0)
                {
                    std::cout << "Message ID does not match. Skipping processing. ID: " << sample.id() << std::endl;
                    return;  // Return immediately if ID is not 0
                }

                std::lock_guard<std::mutex> lock(mtx);  // Lock the mutex for thread-safe access
                sample.id(msg_id_);  // Assign custom ID based on the listener
                message_queue.push_back(sample);  // Add to deque

                std::cout << "Message received by reader with ID: " << sample.id() << std::endl;
                ++received_samples;
            }
            else
            {
                std::cout << "Read failed with error code." << std::endl;
            }
        }

        DataReader* reader_;
        DataWriter* writer_;
        int msg_id_;
        uint32_t received_samples;
    };

    std::vector<PubSubListener> listeners_;  // Store all listeners

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
    
    void process_queue();  // Separate function to process the queue

};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() :
    pub_participant1_(nullptr), pub_participant2_(nullptr),
    sub_participant1_(nullptr), sub_participant2_(nullptr),
    publisher1_(nullptr), publisher2_(nullptr),
    subscriber1_(nullptr), subscriber2_(nullptr),
    topic1_(nullptr), topic2_(nullptr),
    pub_topic1_(nullptr), pub_topic2_(nullptr),
    reader1_(nullptr), reader2_(nullptr),
    writer1_(nullptr), writer2_(nullptr),
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

    if (pub_topic1_ && pub_participant1_)
    {
        pub_participant1_->delete_topic(pub_topic1_);
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
    if (sub_participant2_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(sub_participant2_);
    }
}

bool CombinedPubSub::init()
{
    // Initialize the first Subscriber side for domain 1, multicast 239.255.0.1
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
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator1, "192.168.234.29");
    unicast_locator1.port = 9161;
    datareader_qos1.endpoint().unicast_locator_list.push_back(unicast_locator1);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator1;
    sub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator1, 239, 255, 0, 1);
    sub_multicast_locator1.port = 7651;
    datareader_qos1.endpoint().multicast_locator_list.push_back(sub_multicast_locator1);

    subscriber1_ = sub_participant1_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!subscriber1_)
        return false;

    TopicQos topic_qos1 = TOPIC_QOS_DEFAULT;
    topic_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;

    topic1_ = sub_participant1_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!topic1_)
        return false;

  //  listener1_ = PubSubListener();

    reader1_ = subscriber1_->create_datareader(topic1_, datareader_qos1, nullptr);
    if (!reader1_)
        return false;

    // Initialize the second Subscriber side for domain 7, multicast 239.255.0.7
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
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator2, "192.168.234.29");
    unicast_locator2.port = 9165;
    datareader_qos2.endpoint().unicast_locator_list.push_back(unicast_locator2);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator2;
    sub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator2, 239, 255, 0, 7);
    sub_multicast_locator2.port = 9151;
    datareader_qos2.endpoint().multicast_locator_list.push_back(sub_multicast_locator2);

    subscriber2_ = sub_participant2_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!subscriber2_)
        return false;
        
    TopicQos topic_qos2 = TOPIC_QOS_DEFAULT;
    topic_qos2.reliability().kind = RELIABLE_RELIABILITY_QOS;

    topic2_ = sub_participant2_->create_topic("7", type1_.get_type_name(), topic_qos2);
    if (!topic2_)
        return false;

   // listener2_ = PubSubListener();

    reader2_ = subscriber2_->create_datareader(topic2_, datareader_qos2, nullptr);
    if (!reader2_)
        return false;

    // Initialize additional Subscribers
    // For domain 8, multicast 239.255.0.8


    // Initialize the first Publisher side for domain 8
    DomainParticipantQos pub_participant_qos1;
    pub_participant_qos1.name("publisher_participant1");

    pub_participant1_ = DomainParticipantFactory::get_instance()->create_participant(1, pub_participant_qos1);
    if (!pub_participant1_)
        return false;

    type1_.register_type(pub_participant1_);

    publisher1_ = pub_participant1_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (!publisher1_)
        return false;

    pub_topic1_ = pub_participant1_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!pub_topic1_)
        return false;

    DataWriterQos datawriter_qos1 = DATAWRITER_QOS_DEFAULT;
    datawriter_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;

    datawriter_qos1.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};
    datawriter_qos1.reliable_writer_qos().times.heartbeatPeriod = {1, 0};
    datawriter_qos1.reliable_writer_qos().times.nackResponseDelay = {0, 10};
  //  datawriter_qos1.reliability().max_blocking_time = eprosima::fastrtps::Duration_t(1, 0);  // 1 second max blocking time

    
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator1;
    pub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator1, 239, 255, 0, 1);
    pub_multicast_locator1.port = 7654;
    datawriter_qos1.endpoint().multicast_locator_list.push_back(pub_multicast_locator1);

    writer1_ = publisher1_->create_datawriter(pub_topic1_, datawriter_qos1, nullptr);
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

    pub_topic2_ = pub_participant2_->create_topic("7", type1_.get_type_name(), topic_qos1);
    if (!pub_topic2_)
        return false;

    DataWriterQos datawriter_qos2 = DATAWRITER_QOS_DEFAULT;
    datawriter_qos2.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    datawriter_qos2.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};
    datawriter_qos2.reliable_writer_qos().times.heartbeatPeriod = {1, 0};
    datawriter_qos2.reliable_writer_qos().times.nackResponseDelay = {0, 10};
   // datawriter_qos2.reliability().max_blocking_time = eprosima::fastrtps::Duration_t(1, 0);  // 1 second max blocking time
    
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator2;
    pub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator2, 239, 255, 0, 7);
    pub_multicast_locator2.port = 9154;
    datawriter_qos2.endpoint().multicast_locator_list.push_back(pub_multicast_locator2);

    writer2_ = publisher2_->create_datawriter(pub_topic2_, datawriter_qos2, nullptr);
    if (!writer2_)
        return false;

    // Initialize the third Publisher side for domain 9


    // listener1_ = PubSubListener(writer2_);
   //  listener2_ = PubSubListener(writer1_);
    listeners_.emplace_back(reader1_, writer2_, 999);  // Reader1 -> Writer2
    reader1_->set_listener(&listeners_.back());

    listeners_.emplace_back(reader2_, writer1_, 1000); // Reader2 -> Writer1
    reader2_->set_listener(&listeners_.back());


    return true;
}




void CombinedPubSub::process_queue()
{
    while (!stop_signal)
    {
        if (!message_queue.empty())
        {
          //  std::this_thread::sleep_for(std::chrono::milliseconds(100));
            NetboxMessage1 message = message_queue.front();  // Get the first message
            message_queue.pop_front();  // Remove the message from the deque

            for (auto& listener : listeners_)
            {
                if (message.id() == listener.msg_id_)
                {
                    listener.writer_->write(&message);  // Send using the corresponding writer
                    std::cout << "Message sent with ID: " << message.id() << std::endl;
                }
            }
        }
    }
}




void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::thread processing_thread(&CombinedPubSub::process_queue, this);  // Start the processing thread

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal)
    {

    }

    processing_thread.join();  // Ensure the processing thread is joined before exiting
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



