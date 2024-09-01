#include "ShapePublisher.hpp"
//#include "ShapeSubscriber.hpp"
#include "NetboxMessage1PubSubTypes.h"
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
#include <fstream>
#include <jsoncpp/json/json.h>
#include <mutex>

// Define a mutex for protecting shared data
using namespace eprosima::fastdds::dds;

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init(const std::string& config_file);
    void run();

private:
    struct Config
    {
        int source_domain_id;
        std::string source_destination_ip;
        std::vector<int> updated_domain_ids;
        std::vector<std::string> updated_destination_ips;
    };

    bool load_config(const std::string& config_file);
    bool setup_reader_writer(const Config& config, size_t config_index);
   
    std::string unicast_ip_;
    std::vector<DomainParticipant*> participants_;
    std::vector<Publisher*> publishers_;
    std::vector<Subscriber*> subscribers_;
    std::vector<Topic*> topics_;
    std::vector<DataReader*> readers_;
    std::vector<DataWriter*> writers_;
    TypeSupport type_;

    std::vector<Config> configs_;

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(const std::vector<DataWriter*>& writers, uint64_t self_generated_id)
            : writers_(writers), received_samples_(0), processing_complete_(true), self_generated_id_(self_generated_id) {}


        void on_data_available(DataReader* reader) override
        {
            static std::mutex sample_mutex;  // Define a local static mutex for this method

            NetboxMessage1 sample;
            SampleInfo info;

            // Take the next sample
            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
                // Only process messages with ID 0 or the self-generated ID
                if (sample.id() == 0 || sample.id() == self_generated_id_)
                {
                    // Wait until the previous message has been processed
                    std::lock_guard<std::mutex> lock(sample_mutex);

                    // Block further processing until this message is handled
                    processing_complete_ = false;
                    ++received_samples_;
                    std::cout << "Message received, ID: " << sample.id() << " | Total: " << received_samples_ << std::endl;

                    // Forward messages with ID 0
                    if (sample.id() == 0)
                    {
                        // Modify the ID to mark it as sent by this system
                        sample.id(self_generated_id_);

                        // Write the modified message back out
                        for (auto writer : writers_)
                        {
                            writer->write(&sample);
                            std::cout << "Message re-sent with new ID: " << sample.id() << std::endl;
                        }

                        std::cout << "Process complete after all writers wrote with ID: " << sample.id() << std::endl;
                    }

                    // Mark the processing as complete to allow the next message
                    processing_complete_ = true;
                }
                else
                {
                    std::cout << "Message ID does not match. Skipping processing. ID: " << sample.id() << std::endl;
                }
            }
        }

    private:
        std::vector<DataWriter*> writers_;
        uint32_t received_samples_;
        std::atomic<bool> processing_complete_;  // Flag to indicate whether processing is complete
        uint64_t self_generated_id_;  // Unique ID for self-generated messages
    };

    std::vector<PubSubListener*> listeners_;

    static volatile sig_atomic_t stop_signal_;
    static void handle_interrupt(int) { stop_signal_ = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal_ = 0;

CombinedPubSub::CombinedPubSub() : type_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    for (auto writer : writers_)
    {
        delete writer;
    }
    for (auto reader : readers_)
    {
        delete reader;
    }
    for (auto topic : topics_)
    {
        delete topic;
    }
    for (auto publisher : publishers_)
    {
        delete publisher;
    }
    for (auto subscriber : subscribers_)
    {
        delete subscriber;
    }
    for (auto participant : participants_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(participant);
    }
    for (auto listener : listeners_)
    {
        delete listener;
    }
}

bool CombinedPubSub::load_config(const std::string& config_file)
{
    std::ifstream file(config_file);
    if (!file.is_open())
    {
        std::cerr << "Could not open config file: " << config_file << std::endl;
        return false;
    }

    Json::Value root;
    file >> root;
    
    unicast_ip_ = root["unicaste_ip"].asString();  // Fetch the unicast IP
    std::cout << "Unicast IP fetched: " << unicast_ip_ << std::endl;  // Print the unicast IP

    for (const auto& config : root["configs"])
    {
        Config cfg;
        cfg.source_domain_id = config["source_domain_id"].asInt();
        cfg.source_destination_ip = config["source_destination_ip"].asString();
        for (const auto& id : config["updated_domain_ids"])
        {
            cfg.updated_domain_ids.push_back(id.asInt());
        }
        for (const auto& ip : config["updated_destination_ips"])
        {
            cfg.updated_destination_ips.push_back(ip.asString());
        }
        configs_.push_back(cfg);
    }
    return true;
}

bool CombinedPubSub::init(const std::string& config_file)
{
    // Load the configuration file
    if (!load_config(config_file))
    {
        return false;
    }

    uint16_t unicast_port = 9161; // Starting port for unicast locators

    // Iterate through each configuration and set up the corresponding reader and writers
    for (size_t config_index = 0; config_index < configs_.size(); ++config_index)
    {
        const Config& config = configs_[config_index];
        int domain_id = config.source_domain_id; // Use the source domain ID for the subscriber

        // Create DomainParticipant for the reader
        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant_" + std::to_string(config_index));
        DomainParticipant* sub_participant = DomainParticipantFactory::get_instance()->create_participant(config.source_domain_id, sub_participant_qos);
        if (!sub_participant)
        {
            std::cerr << "Failed to create subscriber participant for config " << config_index << std::endl;
            return false;
        }
        participants_.push_back(sub_participant);
        type_.register_type(sub_participant);

        // Create DataReader
        Subscriber* subscriber = sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!subscriber)
        {
            std::cerr << "Failed to create subscriber for config " << config_index << std::endl;
            return false;
        }
        subscribers_.push_back(subscriber);

        DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
        datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;

        datareader_qos.resource_limits().max_samples = 300000000; // Set a high number to accumulate more data
        datareader_qos.resource_limits().max_instances = 1110;  // Adjust as needed
        datareader_qos.resource_limits().max_samples_per_instance = 13542;


        datareader_qos.endpoint().unicast_locator_list.clear();
        eprosima::fastrtps::rtps::Locator_t unicast_locator;
        unicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
        unicast_locator.port = unicast_port++;
        std::cout << "  Unicast Port: " << unicast_locator.port << std::endl;
        datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);



        eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
        sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip);
      
        int base_port = 7400;
        int offset = 1;
        int multicast_port = base_port + (250 * domain_id) + offset;
        std::cout << "  Multicast Port: " << multicast_port << std::endl;

        datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        Topic* topic = sub_participant->create_topic("7", type_.get_type_name(), topic_qos);
        if (!topic)
        {
            std::cerr << "Failed to create topic for config " << config_index << std::endl;
            return false;
        }
        topics_.push_back(topic);

        std::cout << "Created DataReader for Domain ID: " << config.source_domain_id << ", Destination IP: " << config.source_destination_ip << std::endl;

        // Create DomainParticipants and DataWriters for each destination
        std::vector<DataWriter*> writers;
        uint64_t self_generated_id = 999 + config_index;  // Unique ID for self-generated messages per listener

        for (size_t i = 0; i < config.updated_domain_ids.size(); ++i)
        {
            int domain_id = config.updated_domain_ids[i];
            const std::string& destination_ip = config.updated_destination_ips[i];

            DomainParticipantQos pub_participant_qos;
            pub_participant_qos.name("publisher_participant_" + std::to_string(config_index) + "_" + std::to_string(i));
            DomainParticipant* pub_participant = DomainParticipantFactory::get_instance()->create_participant(domain_id, pub_participant_qos);
            if (!pub_participant)
            {
                std::cerr << "Failed to create publisher participant for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            participants_.push_back(pub_participant);
            type_.register_type(pub_participant);

            Publisher* publisher = pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
            if (!publisher)
            {
                std::cerr << "Failed to create publisher for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            publishers_.push_back(publisher);

            Topic* pub_topic = pub_participant->create_topic("7", type_.get_type_name(), topic_qos);
            if (!pub_topic)
            {
                std::cerr << "Failed to create publisher topic for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            topics_.push_back(pub_topic);

            DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
            datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

            // Set heartbeat period to 1 second (can be adjusted as needed)
            datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

            // Set nack response delay to a low value for quick response
            datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds

            eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
            pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);

            int base_port = 7400;
            int offset = 1;
            int multicast_port = base_port + (250 * domain_id) + offset;
            std::cout << "  Multicast Port: " << multicast_port << std::endl;

            pub_multicast_locator.port = multicast_port;

            datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

            DataWriter* writer = publisher->create_datawriter(pub_topic, datawriter_qos, nullptr);
            if (!writer)
            {
                std::cerr << "Failed to create DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
                return false;
            }
            writers_.push_back(writer);
            writers.push_back(writer);

            std::cout << "Created DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
        }

        // Create and attach listener to the reader
        PubSubListener* listener = new PubSubListener(writers, self_generated_id);
        listeners_.push_back(listener);

        DataReader* reader = subscriber->create_datareader(topic, datareader_qos, listener);
        if (!reader)
        {
            std::cerr << "Failed to create DataReader for config " << config_index << std::endl;
            return false;
        }
        readers_.push_back(reader);
    }

    return true;
}



void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "\nStopped" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    CombinedPubSub combined_pubsub;
    if (combined_pubsub.init(argv[1]))
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



/*#include "ShapePublisher.hpp"
#include "ShapeSubscriber.hpp"
#include "NetboxMessage1PubSubTypes.h"
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
#include <fstream>
#include <jsoncpp/json/json.h>

using namespace eprosima::fastdds::dds;

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init(const std::string& config_file);
    void run();

private:
    struct Config
    {
        int source_domain_id;
        std::string source_destination_ip;
        std::vector<int> updated_domain_ids;
        std::vector<std::string> updated_destination_ips;
    };

    bool load_config(const std::string& config_file);
    bool setup_reader_writer(const Config& config, size_t config_index);
   
    std::string unicast_ip_;
    std::vector<DomainParticipant*> participants_;
    std::vector<Publisher*> publishers_;
    std::vector<Subscriber*> subscribers_;
    std::vector<Topic*> topics_;
    std::vector<DataReader*> readers_;
    std::vector<DataWriter*> writers_;
    TypeSupport type_;

    std::vector<Config> configs_;

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(const std::vector<DataWriter*>& writers)
            : writers_(writers), received_samples_(0), processing_complete_(true) {}

        void on_data_available(DataReader* reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;

            // Wait until the previous message has been processed
            while (!processing_complete_)
            {
                std::this_thread::yield();  // Yield the current thread to avoid busy waiting
            }

            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
                // Block further processing until this message is handled
                processing_complete_ = false;
                ++received_samples_;
                std::cout << "Message received, ID: " << sample.id() << " | Total: " << received_samples_ << std::endl;

                // Check if the message was sent by this system by examining the ID
                if (sample.id() == 999)  // Example: any id ending in 999 is self-generated
                {
                    std::cout << "Message ID indicates it was generated by this system. Skipping re-send." << std::endl;
                    processing_complete_ = true;  // Mark as processed to allow the next message
                    return;  // Skip sending the message
                }

                // Modify the ID to mark it as sent by this system
                uint64_t new_id = 999;  // Mark this message as generated by this system
                sample.id(new_id);

                // Write the modified message back out
                for (auto writer : writers_)
                {
                    writer->write(&sample);
                }
                std::cout << "Assigned new ID: " << sample.id() << " (after sending)" << std::endl;

                // Mark the processing as complete to allow the next message
                processing_complete_ = true;
            }
        }

    private:
        std::vector<DataWriter*> writers_;
        uint32_t received_samples_;
        std::atomic<bool> processing_complete_;  // Flag to indicate whether processing is complete
    };

    std::vector<PubSubListener*> listeners_;

    static volatile sig_atomic_t stop_signal_;
    static void handle_interrupt(int) { stop_signal_ = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal_ = 0;

CombinedPubSub::CombinedPubSub() : type_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    for (auto writer : writers_)
    {
        delete writer;
    }
    for (auto reader : readers_)
    {
        delete reader;
    }
    for (auto topic : topics_)
    {
        delete topic;
    }
    for (auto publisher : publishers_)
    {
        delete publisher;
    }
    for (auto subscriber : subscribers_)
    {
        delete subscriber;
    }
    for (auto participant : participants_)
    {
        DomainParticipantFactory::get_instance()->delete_participant(participant);
    }
    for (auto listener : listeners_)
    {
        delete listener;
    }
}

bool CombinedPubSub::load_config(const std::string& config_file)
{
    std::ifstream file(config_file);
    if (!file.is_open())
    {
        std::cerr << "Could not open config file: " << config_file << std::endl;
        return false;
    }

    Json::Value root;
    file >> root;
    
    unicast_ip_ = root["unicaste_ip"].asString();  // Fetch the unicast IP
    std::cout << "Unicast IP fetched: " << unicast_ip_ << std::endl;  // Print the unicast IP

    for (const auto& config : root["configs"])
    {
        Config cfg;
        cfg.source_domain_id = config["source_domain_id"].asInt();
        cfg.source_destination_ip = config["source_destination_ip"].asString();
        for (const auto& id : config["updated_domain_ids"])
        {
            cfg.updated_domain_ids.push_back(id.asInt());
        }
        for (const auto& ip : config["updated_destination_ips"])
        {
            cfg.updated_destination_ips.push_back(ip.asString());
        }
        configs_.push_back(cfg);
    }
    return true;
}

bool CombinedPubSub::init(const std::string& config_file)
{
    // Load the configuration file
    if (!load_config(config_file))
    {
        return false;
    }

    uint16_t unicast_port = 7663; // Starting port for unicast locators

    // Iterate through each configuration and set up the corresponding reader and writers
    for (size_t config_index = 0; config_index < configs_.size(); ++config_index)
    {
        const Config& config = configs_[config_index];
        int domain_id = config.source_domain_id; // Use the source domain ID for the subscriber

        // Create DomainParticipant for the reader
        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant_" + std::to_string(config_index));
        DomainParticipant* sub_participant = DomainParticipantFactory::get_instance()->create_participant(config.source_domain_id, sub_participant_qos);
        if (!sub_participant)
        {
            std::cerr << "Failed to create subscriber participant for config " << config_index << std::endl;
            return false;
        }
        participants_.push_back(sub_participant);
        type_.register_type(sub_participant);

        // Create DataReader
        Subscriber* subscriber = sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!subscriber)
        {
            std::cerr << "Failed to create subscriber for config " << config_index << std::endl;
            return false;
        }
        subscribers_.push_back(subscriber);

        DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
        datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;

        datareader_qos.resource_limits().max_samples = 300000000; // Set a high number to accumulate more data
        datareader_qos.resource_limits().max_instances = 1110;  // Adjust as needed
        datareader_qos.resource_limits().max_samples_per_instance = 13542;


        datareader_qos.endpoint().unicast_locator_list.clear();
        eprosima::fastrtps::rtps::Locator_t unicast_locator;
        unicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
        unicast_locator.port = unicast_port++;
        std::cout << "  Unicast Port: " << unicast_locator.port << std::endl;
        datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);



        eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
        sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip);
      
        int base_port = 7400;
        int offset = 1;
        int multicast_port = base_port + (250 * domain_id) + offset;
        std::cout << "  Multicast Port: " << multicast_port << std::endl;

        datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

        TopicQos topic_qos = TOPIC_QOS_DEFAULT;
        topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        Topic* topic = sub_participant->create_topic("7", type_.get_type_name(), topic_qos);
        if (!topic)
        {
            std::cerr << "Failed to create topic for config " << config_index << std::endl;
            return false;
        }
        topics_.push_back(topic);

        std::cout << "Created DataReader for Domain ID: " << config.source_domain_id << ", Destination IP: " << config.source_destination_ip << std::endl;

        // Create DomainParticipants and DataWriters for each destination
        std::vector<DataWriter*> writers;
        for (size_t i = 0; i < config.updated_domain_ids.size(); ++i)
        {
            int domain_id = config.updated_domain_ids[i];
            const std::string& destination_ip = config.updated_destination_ips[i];

            DomainParticipantQos pub_participant_qos;
            pub_participant_qos.name("publisher_participant_" + std::to_string(config_index) + "_" + std::to_string(i));
            DomainParticipant* pub_participant = DomainParticipantFactory::get_instance()->create_participant(domain_id, pub_participant_qos);
            if (!pub_participant)
            {
                std::cerr << "Failed to create publisher participant for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            participants_.push_back(pub_participant);
            type_.register_type(pub_participant);

            Publisher* publisher = pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
            if (!publisher)
            {
                std::cerr << "Failed to create publisher for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            publishers_.push_back(publisher);

            Topic* pub_topic = pub_participant->create_topic("7", type_.get_type_name(), topic_qos);
            if (!pub_topic)
            {
                std::cerr << "Failed to create publisher topic for config " << config_index << ", destination " << i << std::endl;
                return false;
            }
            topics_.push_back(pub_topic);

            DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
            datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

            // Set heartbeat period to 1 second (can be adjusted as needed)
            datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

            // Set nack response delay to a low value for quick response
            datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds

            eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
            pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);

            int base_port = 7400;
            int offset = 1;
            int multicast_port = base_port + (250 * domain_id) + offset;
            std::cout << "  Multicast Port: " << multicast_port << std::endl;

            pub_multicast_locator.port = multicast_port;

            datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

            DataWriter* writer = publisher->create_datawriter(pub_topic, datawriter_qos, nullptr);
            if (!writer)
            {
                std::cerr << "Failed to create DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
                return false;
            }
            writers_.push_back(writer);
            writers.push_back(writer);

            std::cout << "Created DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
        }

        // Create and attach listener to the reader
        PubSubListener* listener = new PubSubListener(writers);
        listeners_.push_back(listener);

        DataReader* reader = subscriber->create_datareader(topic, datareader_qos, listener);
        if (!reader)
        {
            std::cerr << "Failed to create DataReader for config " << config_index << std::endl;
            return false;
        }
        readers_.push_back(reader);
    }

    return true;
}



void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    std::cout << "\nStopped" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }

    CombinedPubSub combined_pubsub;
    if (combined_pubsub.init(argv[1]))
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
*/
