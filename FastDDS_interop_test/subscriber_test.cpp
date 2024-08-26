#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <unistd.h>
#include <signal.h>
#include <thread>
#include <iostream>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/common/Locator.h>
#include "ShapeSubscriber.hpp"
#include "NetboxMessagePubSubTypes.h"
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

ShapeSubscriber::ShapeSubscriber() : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new NetboxMessagePubSubType()), listener_() {}

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

void print_qos(const DomainParticipantQos& qos)
{
    std::cout << "DomainParticipant QoS Settings:" << std::endl;

    std::cout << "UserData: ";
    if (qos.user_data().data_vec().empty())
        std::cout << "Empty";
    else
        std::cout << std::string(qos.user_data().data_vec().begin(), qos.user_data().data_vec().end());
    std::cout << std::endl;

    std::cout << "EntityFactory: Autoenable Created Entities: " 
              << (qos.entity_factory().autoenable_created_entities ? "True" : "False") << std::endl;

    std::cout << "TransportConfig: Use Builtin Transports: "
              << (qos.transport().use_builtin_transports ? "True" : "False") << std::endl;

    // Add more QoS settings to print as needed
}
void print_qos(const TopicQos& qos) {
    // Print Reliability
    std::string reliability_str;
    switch (qos.reliability().kind) {
        case RELIABLE_RELIABILITY_QOS:
            reliability_str = "RELIABLE";
            break;
        case BEST_EFFORT_RELIABILITY_QOS:
            reliability_str = "BEST_EFFORT";
            break;
        default:
            reliability_str = "UNKNOWN";
            break;
    }
    std::cout << "Reliability: " << reliability_str << std::endl;

    // Print Durability
    std::string durability_str;
    switch (qos.durability().kind) {
        case TRANSIENT_LOCAL_DURABILITY_QOS:
            durability_str = "TRANSIENT_LOCAL";
            break;
        case VOLATILE_DURABILITY_QOS:
            durability_str = "VOLATILE";
            break;
        case TRANSIENT_DURABILITY_QOS:
            durability_str = "TRANSIENT";
            break;
        case PERSISTENT_DURABILITY_QOS:
            durability_str = "PERSISTENT";
            break;
        default:
            durability_str = "UNKNOWN";
            break;
    }
    std::cout << "Durability: " << durability_str << std::endl;

    // Print Deadline
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;

    // Print Latency Budget
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;

    // Print Liveliness
    std::string liveliness_str;
    switch (qos.liveliness().kind) {
        case AUTOMATIC_LIVELINESS_QOS:
            liveliness_str = "AUTOMATIC";
            break;
        case MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
            liveliness_str = "MANUAL_BY_PARTICIPANT";
            break;
        case MANUAL_BY_TOPIC_LIVELINESS_QOS:
            liveliness_str = "MANUAL_BY_TOPIC";
            break;
        default:
            liveliness_str = "UNKNOWN";
            break;
    }
    std::cout << "Liveliness: " << liveliness_str << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;

    // Print History
    std::string history_str;
    switch (qos.history().kind) {
        case KEEP_ALL_HISTORY_QOS:
            history_str = "KEEP_ALL";
            break;
        case KEEP_LAST_HISTORY_QOS:
            history_str = "KEEP_LAST";
            break;
        default:
            history_str = "UNKNOWN";
            break;
    }
    std::cout << "History: " << history_str << " Depth: " << qos.history().depth << std::endl;

    // Print Resource Limits
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples 
              << " Max Instances: " << qos.resource_limits().max_instances 
              << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}


void print_qos(const DataReaderQos& qos) {
    // Print Reliability
    std::string reliability_str;
    switch (qos.reliability().kind) {
        case RELIABLE_RELIABILITY_QOS:
            reliability_str = "RELIABLE";
            break;
        case BEST_EFFORT_RELIABILITY_QOS:
            reliability_str = "BEST_EFFORT";
            break;
        default:
            reliability_str = "UNKNOWN";
            break;
    }
    std::cout << "Reliability: " << reliability_str << std::endl;

    // Print Durability
    std::string durability_str;
    switch (qos.durability().kind) {
        case TRANSIENT_LOCAL_DURABILITY_QOS:
            durability_str = "TRANSIENT_LOCAL";
            break;
        case VOLATILE_DURABILITY_QOS:
            durability_str = "VOLATILE";
            break;
        case TRANSIENT_DURABILITY_QOS:
            durability_str = "TRANSIENT";
            break;
        case PERSISTENT_DURABILITY_QOS:
            durability_str = "PERSISTENT";
            break;
        default:
            durability_str = "UNKNOWN";
            break;
    }
    std::cout << "Durability: " << durability_str << std::endl;

    // Print Deadline
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;

    // Print Latency Budget
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;

    // Print Liveliness
    std::string liveliness_str;
    switch (qos.liveliness().kind) {
        case AUTOMATIC_LIVELINESS_QOS:
            liveliness_str = "AUTOMATIC";
            break;
        case MANUAL_BY_PARTICIPANT_LIVELINESS_QOS:
            liveliness_str = "MANUAL_BY_PARTICIPANT";
            break;
        case MANUAL_BY_TOPIC_LIVELINESS_QOS:
            liveliness_str = "MANUAL_BY_TOPIC";
            break;
        default:
            liveliness_str = "UNKNOWN";
            break;
    }
    std::cout << "Liveliness: " << liveliness_str << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;

    // Print History
    std::string history_str;
    switch (qos.history().kind) {
        case KEEP_ALL_HISTORY_QOS:
            history_str = "KEEP_ALL";
            break;
        case KEEP_LAST_HISTORY_QOS:
            history_str = "KEEP_LAST";
            break;
        default:
            history_str = "UNKNOWN";
            break;
    }
    std::cout << "History: " << history_str << " Depth: " << qos.history().depth << std::endl;

    // Print Resource Limits
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples 
              << " Max Instances: " << qos.resource_limits().max_instances 
              << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}





bool ShapeSubscriber::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("subscriber_participant");

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


    
    
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS; //starts sub
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    //datareader_qos.liveliness().kind= AUTOMATIC_LIVELINESS_QOS;
    datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite; // Set an appropriate lease duration
    datareader_qos.resource_limits().max_samples = 300000000; // Set a high number to accumulate more data
    datareader_qos.resource_limits().max_instances = 1110;  // Adjust as needed
    datareader_qos.resource_limits().max_samples_per_instance = 13542;
    print_qos(datareader_qos);


    //Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 7); // Multicast address
    multicast_locator.port = 7900; // Multicast port

    //std::cout << "Default DataReader QoS Settings:" << std::endl;


    //SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    //datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;//failed

    datareader_qos.endpoint().multicast_locator_list.push_back(multicast_locator);


    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
   //topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS; // nodiff
    topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    topic_qos.durability().kind = VOLATILE_DURABILITY_QOS;
    topic_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
    //topic_qos.durability().kind = TRANSIENT_DURABILITY_QOS;
   // topic_qos.liveliness().kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;//fails to connect
    
    std::cout << "Default DataReader topic Settings:" << std::endl;
    print_qos(topic_qos);
    if (participant_)
    {
        topic_ = participant_->create_topic("0", type_.get_type_name(), topic_qos);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }
    std::cout << "\nStopped" << std::endl;
}

/*
#include <iostream>
#include <vector>
#include <thread>
#include "ShapeSubscriber.hpp"

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage sample; // Use the custom type
    SampleInfo info;
    std::vector<uint8_t> complete_data; // Buffer to store all data

    while (true) // Run indefinitely
    {
        while (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK)
        {
            ++received_samples;
            const std::vector<uint8_t>& data = sample.data();
            complete_data.insert(complete_data.end(), data.begin(), data.end());

            if (complete_data.size() >= 10) // Wait until the data size becomes 10 bytes
            {
                printf("Size of data vector: %zu\n", complete_data.size());

                // Print the serialized data as bytes
                printf("Serialized data: ");
                for (size_t i = 0; i < complete_data.size(); ++i)
                {
                    printf("%u ", complete_data[i]);
                }
                printf("\n");

                // Print the serialized data in hex format
                printf("Serialized data (hex): ");
                for (size_t i = 0; i < complete_data.size(); ++i)
                {
                    printf("%02X ", complete_data[i]);
                }
                printf("\n");

                complete_data.clear(); // Clear the buffer after processing
            }
        }

        if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_NO_DATA)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Add a small delay to prevent busy-waiting
        }
        else
        {
            std::cout << "Read failed: return code " << return_code() << std::endl;
        }
    }
}
*/


void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage sample; // Use the custom type
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

/*
void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage sample; // Use the custom type
    SampleInfo info;
    //std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Add a delay in the loop
    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    //if (return_code == ReturnCode_t::RETCODE_OK)
    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;

        const std::vector<uint8_t>& data = sample.payload(); 
        printf("Size of data vector: %zu\n", data.size());

        // Print the serialized data as bytes
        printf("Serialized data: ");
        for (size_t i = 0; i < data.size(); ++i)
        {
            printf("%u ", data[i]);
        }
        printf("\n");

        printf("Serialized data (hex): ");
        for (size_t i = 0; i < data.size(); ++i)
        {
            printf("%02X ", data[i]);
        }
        printf("\n");
    }

    

    else
    {
        std::cout << "Read failed: return code " << return_code() << std::endl;
    }
}    */
/*    
   if (reader->take_next_sample(&sample, &info) == eprosima::fastrtps::types::ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;

        // Print the data
        std::cout << "Received data: ";
        for (size_t i = 0; i < 16; ++i)
        {
            std::cout << static_cast<unsigned int>(sample.data()[i]) << " ";
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "Read failed" << std::endl;  //uint data[16]
    }

}
*/

/*
void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage sample; // Use the custom type
    SampleInfo info;

    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    if (return_code == ReturnCode_t::RETCODE_OK)
              {
                    ++received_samples;

                    const std::string& data = sample.data();
                    printf("Size of data string: %zu\n", data.size());

                    // Print the serialized data as a string
                    printf("Serialized data: %s\n", data.c_str());

                    // Print the serialized data in hexadecimal format
                    printf("Serialized data (hex): ");
                    for (size_t i = 0; i < data.size(); ++i)
                    {
                        printf("%02X ", static_cast<unsigned char>(data[i]));
                    }
                    printf("\n");
                }

    else
    {
        std::cout << "Read failed: return code " << return_code() << std::endl;
    }
}
*/

/*
void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    SwampGCSType sample;
    SampleInfo info;
    
    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;

        // Assuming sample.data() returns a std::vector<uint8_t> or similar
        const auto& data = sample.data(); 

        // Print the serialized data as bytes
        printf("Serialized data: ");
        for (size_t i = 0; i < data.size(); ++i)
        {
            printf("%u ", static_cast<unsigned char>(data[i]));
        }
        printf("\n");
    }
    else
    {
        std::cout << "Read failed: return code " << return_code << std::endl;
    }
}*/

void ShapeSubscriber::SubscriberListener::on_subscription_matched(DataReader *, const SubscriptionMatchedStatus &info)
{
    matched = info.current_count;
    std::cout << "Number of matched writers matched: " << matched << std::endl;
}























