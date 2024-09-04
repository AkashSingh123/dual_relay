/*#include "ShapeSubscriber.hpp"
#include "ShapePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

#include <unistd.h>
#include <signal.h>
#include <thread>

using namespace eprosima::fastdds::dds;

ShapeSubscriber::ShapeSubscriber() : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new ShapeTypePubSubType()), listener_() {}

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

    if (with_security)
    {
        using namespace std;
        string example_security_configuration_path = "file://../../examples/security_configuration_files/";
        string dds_sec = "dds.sec.";
        string auth = dds_sec + "auth.";
        string auth_plugin = "builtin.PKI-DH";
        string auth_prefix = auth + auth_plugin + ".";
        string access = dds_sec + "access.";
        string access_plugin = "builtin.Access-Permissions";
        string access_prefix = access + access_plugin + ".";
        string crypto = dds_sec + "crypto.";
        string crypto_plugin = "builtin.AES-GCM-GMAC";
        string plugin = "plugin";

        std::vector<pair<string, string>> security_properties = {
            pair<string, string>(auth + plugin, auth_plugin),
            pair<string, string>(access + plugin, access_plugin),
            pair<string, string>(crypto + plugin, crypto_plugin),
            pair<string, string>(auth_prefix + "identity_ca", example_security_configuration_path + "identity_ca.cert.pem"),
            pair<string, string>(auth_prefix + "identity_certificate", example_security_configuration_path + "cert.pem"),
            pair<string, string>(auth_prefix + "private_key", example_security_configuration_path + "key.pem"),
            pair<string, string>(access_prefix + "permissions_ca", example_security_configuration_path + "permissions_ca.cert.pem"),
            pair<string, string>(access_prefix + "governance", example_security_configuration_path + "governance.p7s"),
            pair<string, string>(access_prefix + "permissions", example_security_configuration_path + "permissions.p7s"),
        };

        for (pair<string, string> property : security_properties)
        {
            participant_qos.properties().properties().emplace_back(property.first, property.second);
        }
    }

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;

    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;

    if (participant_)
    {
        topic_ = participant_->create_topic("Square", type_.get_type_name(), topic_qos);
    }

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;

    if (subscriber_ && topic_)
    {
        reader_ = subscriber_->create_datareader(topic_, datareader_qos, &listener_);
    }

    if (reader_ && topic_ && subscriber_ && participant_)
    {
        std::cout << "DataReader created for the topic Square." << std::endl;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader *reader)
{
    ShapeType sample;
    SampleInfo info;

    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;
        std::cout << "Sample received [" << sample.color() << ": (" << sample.x() << "," << sample.y() << ")], count=" << received_samples << std::endl;
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
}*/
/*
#include "ShapeSubscriber.hpp"
#include "UInt8TypePubSubTypes.h"
#include "ShapePubSubTypes.h"
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
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>
#include <fastdds/rtps/common/Locator.h>

using namespace eprosima::fastdds::dds;

ShapeSubscriber::ShapeSubscriber() : participant_(nullptr), subscriber_(nullptr), topic_(nullptr), reader_(nullptr), type_(new UInt8TypePubSubType()), listener_() {}

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

    if (with_security)
    {
        using namespace std;
        string example_security_configuration_path = "file://../../examples/security_configuration_files/";
        string dds_sec = "dds.sec.";
        string auth = dds_sec + "auth.";
        string auth_plugin = "builtin.PKI-DH";
        string auth_prefix = auth + auth_plugin + ".";
        string access = dds_sec + "access.";
        string access_plugin = "builtin.Access-Permissions";
        string access_prefix = access + access_plugin + ".";
        string crypto = dds_sec + "crypto.";
        string crypto_plugin = "builtin.AES-GCM-GMAC";
        string plugin = "plugin";

        std::vector<pair<string, string>> security_properties = {
            pair<string, string>(auth + plugin, auth_plugin),
            pair<string, string>(access + plugin, access_plugin),
            pair<string, string>(crypto + plugin, crypto_plugin),
            pair<string, string>(auth_prefix + "identity_ca", example_security_configuration_path + "identity_ca.cert.pem"),
            pair<string, string>(auth_prefix + "identity_certificate", example_security_configuration_path + "cert.pem"),
            pair<string, string>(auth_prefix + "private_key", example_security_configuration_path + "key.pem"),
            pair<string, string>(access_prefix + "permissions_ca", example_security_configuration_path + "permissions_ca.cert.pem"),
            pair<string, string>(access_prefix + "governance", example_security_configuration_path + "governance.p7s"),
            pair<string, string>(access_prefix + "permissions", example_security_configuration_path + "permissions.p7s"),
        };

        for (pair<string, string> property : security_properties)
        {
            participant_qos.properties().properties().emplace_back(property.first, property.second);
        }
    }

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;

    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;

    if (participant_)
    {
        topic_ = participant_->create_topic("swamp_gcs", type_.get_type_name(), topic_qos);
    }

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;

    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
    multicast_locator.port = 7900; // Multicast port


    datareader_qos.endpoint().multicast_locator_list.push_back(multicast_locator);


    if (subscriber_ && topic_)
    {
        reader_ = subscriber_->create_datareader(topic_, datareader_qos, &listener_);
    }
    
    if (reader_ && topic_ && subscriber_ && participant_)
    {
        std::cout << "DataReader created for the topic swamp_gcs." << std::endl;
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader *reader)
{
    UInt8Type sample;
    SampleInfo info;

    while (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK)
    {
        if (info.sample_state == SampleState::NOT_READ_SAMPLE_STATE)
        {
            ++received_samples;
            std::cout << "Sample received with value: " << static_cast<int>(sample.value()) << ", count=" << received_samples << std::endl;
        }
    }
}


void ShapeSubscriber::SubscriberListener::on_subscription_matched(DataReader *, const SubscriptionMatchedStatus &info)
{
    matched = info.current_count;
    std::cout << "Number of matched writers matched: " << matched << std::endl;
}
*/




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



/*void print_qos(const DataReaderQos& qos) {
    std::cout << "Reliability: " << (qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
    std::cout << "Durability: " << (qos.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS ? "TRANSIENT_LOCAL" : "VOLATILE") << std::endl;
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;
    std::cout << "Liveliness: " << (qos.liveliness().kind == AUTOMATIC_LIVELINESS_QOS ? "AUTOMATIC" : "MANUAL") << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;
    std::cout << "History: " << (qos.history().kind == KEEP_ALL_HISTORY_QOS ? "KEEP_ALL" : "KEEP_LAST") << " Depth: " << qos.history().depth << std::endl;
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples << " Max Instances: " << qos.resource_limits().max_instances << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}

void print_qos(const TopicQos& qos) {
    std::cout << "Reliability: " << (qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
    std::cout << "Durability: " << (qos.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS ? "TRANSIENT_LOCAL" : "VOLATILE") << std::endl;
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;
    std::cout << "Liveliness: " << (qos.liveliness().kind == AUTOMATIC_LIVELINESS_QOS ? "AUTOMATIC" : "MANUAL") << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;
    std::cout << "History: " << (qos.history().kind == KEEP_ALL_HISTORY_QOS ? "KEEP_ALL" : "KEEP_LAST") << " Depth: " << qos.history().depth << std::endl;
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples << " Max Instances: " << qos.resource_limits().max_instances << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}*/





bool ShapeSubscriber::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("subscriber_participant");

    if (with_security)
    {
        using namespace std;
        string example_security_configuration_path = "file://../../examples/security_configuration_files/";
        string dds_sec = "dds.sec.";
        string auth = dds_sec + "auth.";
        string auth_plugin = "builtin.PKI-DH";
        string auth_prefix = auth + auth_plugin + ".";
        string access = dds_sec + "access.";
        string access_plugin = "builtin.Access-Permissions";
        string access_prefix = access + access_plugin + ".";
        string crypto = dds_sec + "crypto.";
        string crypto_plugin = "builtin.AES-GCM-GMAC";
        string plugin = "plugin";

        std::vector<pair<string, string>> security_properties = {
            pair<string, string>(auth + plugin, auth_plugin),
            pair<string, string>(access + plugin, access_plugin),
            pair<string, string>(crypto + plugin, crypto_plugin),
            pair<string, string>(auth_prefix + "identity_ca", example_security_configuration_path + "identity_ca.cert.pem"),
            pair<string, string>(auth_prefix + "identity_certificate", example_security_configuration_path + "cert.pem"),
            pair<string, string>(auth_prefix + "private_key", example_security_configuration_path + "key.pem"),
            pair<string, string>(access_prefix + "permissions_ca", example_security_configuration_path + "permissions_ca.cert.pem"),
            pair<string, string>(access_prefix + "governance", example_security_configuration_path + "governance.p7s"),
            pair<string, string>(access_prefix + "permissions", example_security_configuration_path + "permissions.p7s"),
        };

        for (pair<string, string> property : security_properties)
        {
            participant_qos.properties().properties().emplace_back(property.first, property.second);
        }
    }

    participant_ = DomainParticipantFactory::get_instance()->create_participant(1, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }
    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    std::cout << "Default DataReader QoS Settings:" << std::endl;


    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    subscriber_qos.entity_factory().autoenable_created_entities = true; //false fails
    //subscriber_qos.partition().names().push_back("partitionA");
    //subscriber_qos.presentation().access_scope = PresentationQosPolicyAccessScopeKind::GROUP_PRESENTATION_QOS;
    subscriber_qos.presentation().coherent_access = true;
    subscriber_qos.presentation().ordered_access = true;
 //   subscriber_qos.partition().max_size = 3; // Maximum size for partition names list
 //   subscriber_qos.partition().names().push_back("Partition1");
 //   subscriber_qos.partition().names().push_back("Partition2");
 //   subscriber_qos.partition().names().push_back("Partition3");

//To maintain the compatibility between DurabilityQosPolicy in DataReaders and DataWriters when they have different kind values, the DataWriter kind must be higher or equal to the DataReader kind. And the order between the different kinds is:VOLATILE_DURABILITY_QOS < TRANSIENT_LOCAL_DURABILITY_QOS < TRANSIENT_DURABILITY_QOS < PERSISTENT_DURABILITY_QOS


    
    
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS; //starts sub
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    //datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    //datareader_qos.history().depth = 80; // Set the history depth to 10
    //datareader_qos.deadline().period = eprosima::fastrtps::Duration_t(1, 0); // Set deadline period to 5 seconds

    datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    //datareader_qos.liveliness().kind= AUTOMATIC_LIVELINESS_QOS;
    datareader_qos.liveliness().lease_duration = eprosima::fastrtps::c_TimeInfinite; // Set an appropriate lease duration
    datareader_qos.resource_limits().max_samples = 300000000; // Set a high number to accumulate more data
    datareader_qos.resource_limits().max_instances = 1110;  // Adjust as needed
    datareader_qos.resource_limits().max_samples_per_instance = 13542;
    //datareader_qos.liveliness().kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    //datareader_qos.destination_order().kind = BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS;
   // datareader_qos.deadline().period = Duration_t(0, 0.1000000); // 0 seconds and 500 million nanoseconds (500 ms)

    //datareader_qos.resource_limits().max_samples_per_instance = 10000; // Set a high number to accumulate more data

   // datareader_qos.deadline().period = Duration_t(2, 0); // 2 seconds
   // datareader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS; // Add durability setting failed/stops reader, Transient local doesnt start taking data
   // datareader_qos.liveliness().kind = MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;//Failes
    print_qos(datareader_qos);


    //Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
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
        topic_ = participant_->create_topic("swamp_gcs", type_.get_type_name(), topic_qos);
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


























/*#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
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

void print_qos(const DataReaderQos& qos) {
    std::cout << "Reliability: " << (qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
    std::cout << "Durability: " << (qos.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS ? "TRANSIENT_LOCAL" : "VOLATILE") << std::endl;
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;
    std::cout << "Liveliness: " << (qos.liveliness().kind == AUTOMATIC_LIVELINESS_QOS ? "AUTOMATIC" : "MANUAL") << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;
    std::cout << "History: " << (qos.history().kind == KEEP_ALL_HISTORY_QOS ? "KEEP_ALL" : "KEEP_LAST") << " Depth: " << qos.history().depth << std::endl;
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples << " Max Instances: " << qos.resource_limits().max_instances << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}

void print_qos(const TopicQos& qos) {
    std::cout << "Reliability: " << (qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
    std::cout << "Durability: " << (qos.durability().kind == TRANSIENT_LOCAL_DURABILITY_QOS ? "TRANSIENT_LOCAL" : "VOLATILE") << std::endl;
    std::cout << "Deadline: " << qos.deadline().period.seconds << "s " << qos.deadline().period.nanosec << "ns" << std::endl;
    std::cout << "Latency Budget: " << qos.latency_budget().duration.seconds << "s " << qos.latency_budget().duration.nanosec << "ns" << std::endl;
    std::cout << "Liveliness: " << (qos.liveliness().kind == AUTOMATIC_LIVELINESS_QOS ? "AUTOMATIC" : "MANUAL") << " Lease Duration: " << qos.liveliness().lease_duration.seconds << "s " << qos.liveliness().lease_duration.nanosec << "ns" << std::endl;
    std::cout << "History: " << (qos.history().kind == KEEP_ALL_HISTORY_QOS ? "KEEP_ALL" : "KEEP_LAST") << " Depth: " << qos.history().depth << std::endl;
    std::cout << "Resource Limits: Max Samples: " << qos.resource_limits().max_samples << " Max Instances: " << qos.resource_limits().max_instances << " Max Samples Per Instance: " << qos.resource_limits().max_samples_per_instance << std::endl;
}

bool ShapeSubscriber::init(bool with_security)
{
    DomainParticipantQos participant_qos;
    participant_qos.name("subscriber_participant");

    if (with_security)
    {
        using namespace std;
        string example_security_configuration_path = "file://../../examples/security_configuration_files/";
        string dds_sec = "dds.sec.";
        string auth = dds_sec + "auth.";
        string auth_plugin = "builtin.PKI-DH";
        string auth_prefix = auth + auth_plugin + ".";
        string access = dds_sec + "access.";
        string access_plugin = "builtin.Access-Permissions";
        string access_prefix = access + access_plugin + ".";
        string crypto = dds_sec + "crypto.";
        string crypto_plugin = "builtin.AES-GCM-GMAC";
        string plugin = "plugin";

        std::vector<pair<string, string>> security_properties = {
            pair<string, string>(auth + plugin, auth_plugin),
            pair<string, string>(access + plugin, access_plugin),
            pair<string, string>(crypto + plugin, crypto_plugin),
            pair<string, string>(auth_prefix + "identity_ca", example_security_configuration_path + "identity_ca.cert.pem"),
            pair<string, string>(auth_prefix + "identity_certificate", example_security_configuration_path + "cert.pem"),
            pair<string, string>(auth_prefix + "private_key", example_security_configuration_path + "key.pem"),
            pair<string, string>(access_prefix + "permissions_ca", example_security_configuration_path + "permissions_ca.cert.pem"),
            pair<string, string>(access_prefix + "governance", example_security_configuration_path + "governance.p7s"),
            pair<string, string>(access_prefix + "permissions", example_security_configuration_path + "permissions.p7s"),
        };

        for (pair<string, string> property : security_properties)
        {
            participant_qos.properties().properties().emplace_back(property.first, property.second);
        }
    }

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participant_qos);

    if (participant_)
    {
        type_.register_type(participant_);
    }
    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    std::cout << "Default DataReader QoS Settings:" << std::endl;
    print_qos(datareader_qos);

    SubscriberQos subscriber_qos = SUBSCRIBER_QOS_DEFAULT;
    datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    // Configure multicast settings
    eprosima::fastrtps::rtps::Locator_t multicast_locator;
    multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(multicast_locator, 239, 255, 0, 1); // Multicast address
    multicast_locator.port = 7900; // Multicast port

    if (participant_)
    {
        subscriber_ = participant_->create_subscriber(subscriber_qos);
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    topic_qos.history().kind = KEEP_ALL_HISTORY_QOS;

    print_qos(topic_qos);

    if (participant_)
    {
        topic_ = participant_->create_topic("pos", type_.get_type_name(), topic_qos);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "\nStopped" << std::endl;
}

void ShapeSubscriber::SubscriberListener::on_data_available(DataReader* reader)
{
    NetboxMessage sample; // Use the custom type
    SampleInfo info;

    ReturnCode_t return_code = reader->take_next_sample(&sample, &info);
    if (return_code == ReturnCode_t::RETCODE_OK)
    {
        ++received_samples;

        const std::vector<uint8_t>& data = sample.data(); 
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
}*/

