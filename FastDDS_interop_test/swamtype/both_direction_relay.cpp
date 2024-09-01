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

class PubSubListener : public DataReaderListener {
public:
    DataWriter* targetWriter; // DataWriter to forward data to
    int received_samples;

    PubSubListener(DataWriter* writer) : targetWriter(writer), received_samples(0) {}

    void on_data_available(DataReader* reader) override {
        NetboxMessage1 sample;
        SampleInfo info;
        while (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK) {
            if (info.valid_data) {
                ++received_samples;
                std::cout << "Received message ID: " << sample.id() << ", Total: " << received_samples << std::endl;
                targetWriter->write(&sample);  // Forward the data to the target writer
            }
        }
    }
};

class CombinedPubSub {
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();
    void run();

private:
    DomainParticipant *participant1_, *participant7_;
    Publisher *publisher7_, *publisher1_;
    Subscriber *subscriber7_, *subscriber1_;
    Topic *readtopic7_, *readtopic1_, *writetopic7_, *writetopic1_;
    DataReader *reader7_, *reader1_;
    DataWriter *writer7_, *writer1_;
    TypeSupport type_;
    PubSubListener *listener7_, *listener1_;  // Pointers to listener objects

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub() :
    participant1_(nullptr), participant7_(nullptr),
    publisher7_(nullptr), publisher1_(nullptr),
    subscriber7_(nullptr), subscriber1_(nullptr),
    readtopic7_(nullptr), readtopic1_(nullptr), writetopic7_(nullptr), writetopic1_(nullptr),
    reader7_(nullptr), reader1_(nullptr),
    writer7_(nullptr), writer1_(nullptr),
    listener7_(nullptr), listener1_(nullptr),  // Initialize listener pointers
    type_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub() {
    if (participant7_) {
        participant7_->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant7_);
    }
    if (participant1_) {
        participant1_->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(participant1_);
    }
    delete listener7_;  // Clean up listener memory
    delete listener1_;  // Clean up listener memory
}



bool CombinedPubSub::init()
{
    DomainParticipantQos participant_qos;

    // Initialize participants for Domain ID 7 and 1
    participant7_ = DomainParticipantFactory::get_instance()->create_participant(7, participant_qos);
    participant1_ = DomainParticipantFactory::get_instance()->create_participant(1, participant_qos);
    if (!participant7_ || !participant1_)
        return false;

    type_.register_type(participant7_);
    type_.register_type(participant1_);

    // Initialize subscribers and publishers with specific QoS settings
    SubscriberQos subscriber_qos= SUBSCRIBER_QOS_DEFAULT;
    PublisherQos publisher_qos= PUBLISHER_QOS_DEFAULT;

    // Create and configure subscribers and data readers
    subscriber7_ = participant7_->create_subscriber(subscriber_qos);
    subscriber1_ = participant1_->create_subscriber(subscriber_qos);
    
    DataReaderQos datareader_qos1 = DATAREADER_QOS_DEFAULT;
    //apply_datareader_qos(datareader_qos1); // Function to apply your specific QoS settings
    
    datareader_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos1.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos1.durability().kind = VOLATILE_DURABILITY_QOS;
   /* datareader_qos1.latency_budget().duration = {0, 0};
    datareader_qos1.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    datareader_qos1.reliable_reader_qos().times.initialAcknackDelay = {0, 0};
    datareader_qos1.reliable_reader_qos().times.heartbeatResponseDelay = {0, 0};
    datareader_qos1.reliable_reader_qos().disable_positive_ACKs.enabled = false;*/
    
    
    
    
    datareader_qos1.endpoint().unicast_locator_list.clear();
    
    eprosima::fastrtps::rtps::Locator_t sub_unicast_locator1;
    sub_unicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_unicast_locator1, "192.168.1.52"); // Own IP address
    sub_unicast_locator1.port = 7663; // Set the appropriate port number
    datareader_qos1.endpoint().unicast_locator_list.push_back(sub_unicast_locator1);
    
    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator1;
    sub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator1, 239, 255, 0, 7);
    sub_multicast_locator1.port = 7900;
    datareader_qos1.endpoint().multicast_locator_list.push_back(sub_multicast_locator1);
    
    
    
       
    TopicQos topic_qos7 = TOPIC_QOS_DEFAULT;
    topic_qos7.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    readtopic7_ = participant7_->create_topic("7", type_.get_type_name(), topic_qos7);
    writetopic7_ = participant7_->create_topic("1", type_.get_type_name(), topic_qos7);
    
    reader7_ = subscriber7_->create_datareader(readtopic7_, datareader_qos1, nullptr); // reader at 7 topic participant 7
    
    
    
    DataReaderQos datareader_qos2 = DATAREADER_QOS_DEFAULT;
    //apply_datareader_qos(datareader_qos2); // Function to apply your specific QoS settings
    
    datareader_qos2.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos2.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos2.durability().kind = VOLATILE_DURABILITY_QOS;
    /*datareader_qos2.latency_budget().duration = {0, 0};
    datareader_qos2.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    datareader_qos2.reliable_reader_qos().times.initialAcknackDelay = {0, 0};
    datareader_qos2.reliable_reader_qos().times.heartbeatResponseDelay = {0, 0};
    datareader_qos2.reliable_reader_qos().disable_positive_ACKs.enabled = false;*/
    
    
    
    
    
    
    
    
    
    
    datareader_qos2.endpoint().unicast_locator_list.clear();
    
    eprosima::fastrtps::rtps::Locator_t sub_unicast_locator2;
    sub_unicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_unicast_locator2, "192.168.1.52"); // Own IP address
    sub_unicast_locator2.port = 7663; // Set the appropriate port number
    datareader_qos2.endpoint().unicast_locator_list.push_back(sub_unicast_locator2);
    
    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator2;
    sub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator2, 239, 255, 0, 1);
    sub_multicast_locator2.port = 7900;
    datareader_qos2.endpoint().multicast_locator_list.push_back(sub_multicast_locator2);
    
    
    
    
    
    TopicQos topic_qos1 = TOPIC_QOS_DEFAULT;
    topic_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    
    readtopic1_ = participant1_->create_topic("7", type_.get_type_name(), topic_qos1);
    writetopic1_ = participant1_->create_topic("1", type_.get_type_name(), topic_qos1);
    
    reader1_ = subscriber1_->create_datareader(readtopic1_, datareader_qos2, nullptr);




    // Create and configure publishers and data writers
    publisher7_ = participant7_->create_publisher(publisher_qos);
    publisher1_ = participant1_->create_publisher(publisher_qos);

    DataWriterQos datawriter_qos1 = DATAWRITER_QOS_DEFAULT;
    //apply_datawriter_qos(datawriter_qos1); // Function to apply your specific QoS settings
    datawriter_qos1.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    
    
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator1;
    pub_multicast_locator1.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator1, 239, 255, 0, 7);
    pub_multicast_locator1.port = 7900;
    datawriter_qos1.endpoint().multicast_locator_list.push_back(pub_multicast_locator1);
    
    
    writer7_ = publisher7_->create_datawriter(writetopic7_, datawriter_qos1, nullptr);
    
    
    
    
    
    DataWriterQos datawriter_qos2 = DATAWRITER_QOS_DEFAULT;
    //apply_datawriter_qos(datawriter_qos2); // Function to apply your specific QoS settings
    datawriter_qos2.reliability().kind = RELIABLE_RELIABILITY_QOS;
    
    
    
    eprosima::fastrtps::rtps::Locator_t pub_multicast_locator2;
    pub_multicast_locator2.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator2, 239, 255, 0, 8);
    pub_multicast_locator2.port = 7900;
    datawriter_qos2.endpoint().multicast_locator_list.push_back(pub_multicast_locator2);
    
    writer1_ = publisher1_->create_datawriter(writetopic1_, datawriter_qos2, nullptr);
    

    listener7_ = new PubSubListener(writer1_);  // Forward data from reader7_ to writer1_
    listener1_ = new PubSubListener(writer7_);  // Forward data from reader1_ to writer7_

    // Attach listeners to DataReaders (using pointers)
    reader7_->set_listener(listener7_, StatusMask::all());
    reader1_->set_listener(listener1_, StatusMask::all());


    return true;
}

/*void CombinedPubSub::apply_datareader_qos(DataReaderQos &qos)
{
    qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    qos.history().kind = KEEP_ALL_HISTORY_QOS;
    qos.durability().kind = VOLATILE_DURABILITY_QOS;
    qos.latency_budget().duration = {0, 0};
    qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    qos.reliable_reader_qos().times.initialAcknackDelay = {0, 0};
    qos.reliable_reader_qos().times.heartbeatResponseDelay = {0, 0};
    qos.reliable_reader_qos().disable_positive_ACKs.enabled = false;
}

void CombinedPubSub::apply_datawriter_qos(DataWriterQos &qos)
{
    qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
}*/

void CombinedPubSub::run() {
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal) {
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
    std::cout << "\nStopped" << std::endl;
}

int main() {
    CombinedPubSub combined_pubsub;
    if (combined_pubsub.init()) {
        combined_pubsub.run();
    } else {
        std::cerr << "Failed to initialize CombinedPubSub." << std::endl;
        return 1;
    }

    return 0;
}

