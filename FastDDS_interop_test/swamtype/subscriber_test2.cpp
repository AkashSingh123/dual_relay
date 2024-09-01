#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <iostream>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/common/Locator.h>
#include "ShapeSubscriber.hpp"
#include "NetboxMessage1PubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastrtps/types/DynamicData.h>
#include <fastrtps/types/DynamicDataFactory.h>
#include <csignal> // Include for signal handling
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <signal.h>
#include <thread>
#include <iostream>
#include <vector>

using namespace eprosima::fastdds::dds;

//ShapeSubscriber::ShapeSubscriber() : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new newmessagePubSubType()), listener_() {}
ShapeSubscriber::ShapeSubscriber() : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new NetboxMessage1PubSubType()), listener_() {}
ShapeSubscriber::~ShapeSubscriber()
{
    if (reader_ && subscriber_)
    {
        subscriber_->delete_datareader(reader_);
    }

    if (subscriber_ && participant_)
    {
        participant_->delete_subscriber(subscriber_);
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


bool ShapeSubscriber::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("subscriber_participant");
    
    participant_qos.transport().send_socket_buffer_size = 1048576;  // 1 MB send buffer
    participant_qos.transport().listen_socket_buffer_size = 419430444; // 4 MB receive buffer


    participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }
    
    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    datareader_qos.history().depth = 1;
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    
    
    
    
    //datareader_qos.resource_limits().initial_instances = 10;  // Pre-allocate for 10 instances
   // datareader_qos.resource_limits().initial_samples = 200;   // Pre-allocate for 200 samples

    
    //datareader_qos.deadline().period = eprosima::fastrtps::Duration_t(0, 100);
    
    //datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
   // datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite; // Set an appropriate lease duration
    datareader_qos.resource_limits().max_samples = 30000000000; // Set a high number to accumulate more data
    datareader_qos.resource_limits().max_instances = 100000;  // Adjust as needed
    datareader_qos.resource_limits().max_samples_per_instance = 135421;

   // datareader_qos.resource_limits().extra_samples = 590;  // Extra samples to be used when needed

    datareader_qos.reliable_reader_qos().disable_positive_ACKs.enabled = false;

    datareader_qos.reliable_reader_qos().times.initialAcknackDelay = {0, 70};

    // Set the deadline period to ensure timely data arrival
 //   datareader_qos.deadline().period = eprosima::fastrtps::Duration_t(2, 0); // 2 seconds

    // Set the heartbeat response delay to ensure quick ACKNACK
    datareader_qos.reliable_reader_qos().times.heartbeatResponseDelay = {0, 500}; // 5 milliseconds


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
   // datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
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



    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
    multicast_locator.port = 9151;

    datareader_qos.endpoint().multicast_locator_list.push_back(multicast_locator);
    //datareader_qos.endpoint().multicast_locator_list.clear();
    //datareader_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;  // Set Destination Order to ByReceptionTimestamp



    /*eprosima::fastrtps::rtps::Locator_t unicast_locator;
    unicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, "192.168.50.90");
    unicast_locator.port = 7900; // Set the appropriate port number

    datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);*/


    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // nodiff
   // topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
   // topic_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    //topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; 
    //topic_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    //topic_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    //topic_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    //topic_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    //topic_qos.history().depth = 10;
    //topic_qos.liveliness().kind = AUTOMATIC_LIVELINESS_QOS;
    //topic_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite;  
    //topic_qos.destination_order().kind = BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS;
    //topic_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;  // Set Destination Order to ByReceptionTimestamp

    if (participant_)
    {
        topic_ = participant_->create_topic("command", type_.get_type_name(), topic_qos);
    }

    if (subscriber_ && topic_)
    {
        reader_ = subscriber_->create_datareader(topic_, datareader_qos, &listener_);
    }

    if (reader_ && topic_ && subscriber_ && participant_)
    {
        std::cout << "DataReader created for the topic pos." << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}




/*bool ShapeSubscriber::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("subscriber_participant");
    participant_qos.wire_protocol().reliability().kind = RELIABLE_RELIABILITY_QOS;

    participant_ = DomainParticipantFactory::get_instance()->create_participant(7, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }
    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    std::cout << "Default DataReader QoS Settings:" << std::endl;


    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    subscriber_qos.entity_factory().autoenable_created_entities = true; //false fails
    subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;


    
    
    //datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS; //starts sub
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    //datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    //datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;//failed
    //datareader_qos.liveliness().kind= AUTOMATIC_LIVELINESS_QOS;
    datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite; // Set an appropriate lease duration
    //datareader_qos.resource_limits().max_samples = 300000000; // Set a high number to accumulate more data
    //datareader_qos.resource_limits().max_instances = 1110;  // Adjust as needed
    //datareader_qos.resource_limits().max_samples_per_instance = 13542;



    //Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 7); // Multicast address
    multicast_locator.port = 7900; // Multicast port

    //std::cout << "Default DataReader QoS Settings:" << std::endl;


    //SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;

    

    datareader_qos.endpoint().multicast_locator_list.push_back(multicast_locator);


    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    //topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // nodiff
   // topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
   // topic_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    //topic_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    //topic_qos.durability().kind = TRANSIENT_DURABILITY_QOS;
   // topic_qos.liveliness().kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;//fails to connect

        
    std::cout << "Default DataReader topic Settings:" << std::endl;

    if (participant_)
    {
        topic_ = participant_->create_topic("1", type_.get_type_name(), topic_qos);
    }



    if (subscriber_ && topic_)
    {
        reader_ = subscriber_->create_datareader(topic_, datareader_qos, &listener_);
    }

    if (reader_ && topic_ && subscriber_ && participant_)
    {
        std::cout << "DataReader created for the topic pos." << std::endl;
        return true;
    }
    else
    {
        return false;
    }
}*/

// For handling stop signal to break the infinite loop
namespace subscriber_stop
{
    volatile sig_atomic_t stop;
    void handle_interrupt(int)
    {
        stop = 1;
    }
}

void ShapeSubscriber::run()
{
    signal(SIGINT, subscriber_stop::handle_interrupt);

    std::cout << "Waiting for data" << std::endl;

    while (!subscriber_stop::stop)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    std::cout << "\nStopped" << std::endl;
}

/*
void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage1 sample; // Use the custom type
    SampleInfo info;
    //std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Add a delay in the loop
    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    //if (return_code == ReturnCode_t::RETCODE_OK)
    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;
        //const <uint64_t>& data = sample.timestamp(); 
       // printf("Size of data vector: %zu\n", data.size());
        printf("Message ID: %u\n", sample.id());

        const std::vector<uint8_t>& bata = sample.data(); 


        printf("Serialized data (hex): ");
        //printf("Payload (hex): 00 15 00 00 00 ");
        for (size_t i = 0; i < bata.size(); ++i)
        {
            printf("%02X ", bata[i]);
        }
        
        printf("\n");
        printf("\n");
        // Print the serialized data as bytes
        printf("Serialized data: ");
        for (size_t i = 0; i < bata.size(); ++i)
        {
            printf("%u ", bata[i]);
        }
        printf("Size of data vector: %zu\n", bata.size());
        printf("\n");

    }

    

    else
    {
        std::cout << "Read failed: return code " << return_code() << std::endl;
    }
}    
*/

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage1 sample; // Use the custom type
    SampleInfo info;
   // std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Add a delay in the loop
    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);

    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;
        //const <uint64_t>& data = sample.timestamp(); 
       // printf("Size of data vector: %zu\n", data.size());
        std::cout << "Number of samples received: " << received_samples << std::endl;

        printf("ttttt");

        printf("Timestamp (hex): %016lX\n", sample.timestamp());
        uint64_t timestamp = sample.timestamp();
        uint64_t id = sample.id();
        printf("\n");
        printf("ID (hex): 0x");
        for (int i = 0; i <= 56; i += 8)
        {
            printf("%02X ", (id >> i) & 0xFF);
        }
        printf("\n");
        
        printf("Topics (hex): ");
        for (const auto& topic : sample.topics())
        {
            for (const auto& c : topic)
            {
                printf("%02X", static_cast<unsigned char>(c));
                if (&c != &topic.back())
                {
                    printf(" ");
                }
            }
            if (&topic != &sample.topics().back())
            {
                printf(" | ");
            }
        }
        printf("\n");


        printf("Timestamp (hex): 0x");
        for (int i = 0; i <= 56; i += 8)
        {
            printf("%02X ", (timestamp >> i) & 0xFF);
        }


        
        const std::vector<uint8_t>& bata = sample.payload(); 


        printf("Serialized data (hex): ");
        //printf("Payload (hex): 00 15 00 00 00 ");
        for (size_t i = 0; i < bata.size(); ++i)
        {
            printf("%02X ", bata[i]);
        }
        
        printf("\n");
        printf("\n");
        // Print the serialized data as bytes
        printf("Serialized data: ");
        for (size_t i = 0; i < bata.size(); ++i)
        {
            printf("%u ", bata[i]);
        }
        printf("Size of data vector: %zu\n", bata.size());
        printf("\n");

    }

    

    else
    {
        std::cout << "Read failed: return code " << return_code() << std::endl;
    }
}    



void ShapeSubscriber::SubscriberListener::on_subscription_matched(DataReader *, const SubscriptionMatchedStatus &info)
{
    matched = info.current_count;
    std::cout << "Number of matched writers matched: " << matched << std::endl;
}


