#include "ShapePublisher.hpp"
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
#include <queue>
#include <atomic>
#include <unordered_map>
#include <deque>
using namespace eprosima::fastdds::dds;

// Global queue and mutex
std::deque<NetboxMessage1> message_queue;
std::mutex queue_mutex;

// Global map to track which writers are associated with which message IDs
std::unordered_map<uint64_t, std::vector<DataWriter*>> writer_map;

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
    void process_queue();

    std::string unicast_ip_;
    std::vector<DomainParticipant*> participants_;
    std::vector<Publisher*> publishers_;
    std::vector<Subscriber*> subscribers_;
    std::vector<Topic*> topics_;
    std::vector<DataReader*> readers_;
    std::vector<DataWriter*> writers_;
    TypeSupport type_;

    std::vector<Config> configs_;
   // std::atomic<bool> stop_signal_{false};

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(std::vector<DataWriter*> writers, uint64_t msg_id)
            : writers_(writers), msg_id_(msg_id) {}

        void on_data_available(DataReader* reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;

            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
            
                 std::cout << "Message received and queued with new ID: " << sample.id() << std::endl;
                // Process only if the message ID is 0
                if (sample.id() == 0)
                {
                    // Modify the message ID and enqueue the message
                    sample.id(msg_id_);

                    {
                        std::lock_guard<std::mutex> lock(queue_mutex);
                        message_queue.push_back(sample);
                        std::cout << "Message received and queued with new ID: " << sample.id() << std::endl;
                    }
                }
                else
                {
                    std::cout << "Message ID does not match (ID: " << sample.id() << "). Skipping processing." << std::endl;
                }
            }
            else
            {
                std::cerr << "Failed to take sample or invalid data." << std::endl;
            }
        }

    private:
        std::vector<DataWriter*> writers_;
        uint64_t msg_id_;
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

    unicast_ip_ = root["unicaste_ip"].asString();

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
    if (!load_config(config_file))
    {
        return false;
    }

    uint16_t unicast_port = 7446; // Starting port for unicast locators

    for (size_t config_index = 0; config_index < configs_.size(); ++config_index)
    {
        const Config& config = configs_[config_index];
        int domain_id = config.source_domain_id;

        // Create DomainParticipant for the reader
        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant_" + std::to_string(config_index));
        DomainParticipant* sub_participant = DomainParticipantFactory::get_instance()->create_participant(domain_id, sub_participant_qos);
        if (!sub_participant)
        {
            std::cerr << "Failed to create subscriber participant for config " << config_index << std::endl;
            return false;
        }
        participants_.push_back(sub_participant);
        type_.register_type(sub_participant);

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
       
       
       
            datareader_qos.resource_limits().max_samples = 30000000000; // Set a high number to accumulate more data
            datareader_qos.resource_limits().max_instances = 1000000;  // Adjust as needed
            datareader_qos.resource_limits().max_samples_per_instance = 135421;
      //  datareader_qos.latency_budget().duration = {0, 0}; // Zero latency
      
      
                  datareader_qos.reliable_reader_qos().disable_positive_ACKs.enabled = false;
 
            datareader_qos.reliable_reader_qos().times.initialAcknackDelay = {0, 70};

            datareader_qos.reliable_reader_qos().times.heartbeatResponseDelay = {0, 500}; // 5 milliseconds

        datareader_qos.endpoint().unicast_locator_list.clear();
        eprosima::fastrtps::rtps::Locator_t unicast_locator;
        unicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
        unicast_locator.port = unicast_port + 4*config_index;
        datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);

        eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
        sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip);
        
        int base_port = 7400;
        int offset = 1;
        int multicast_port = base_port + (250 * domain_id) + offset;
        sub_multicast_locator.port = multicast_port;
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

        std::vector<DataWriter*> writers;
        uint64_t self_generated_id = 999 + config_index;

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
          //  datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

            // Set heartbeat period to 1 second (can be adjusted as needed)
         //   datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

            // Set nack response delay to a low value for quick response
          //  datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds

            eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
            pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);

            int base_port = 7400;
            int offset = 1;
            int multicast_port = base_port + (250 * domain_id) + offset;
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

            // Store the writers in the global map associated with this message ID
            writer_map[self_generated_id] = writers;

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

    // Start the queue processing thread
    std::thread processing_thread(&CombinedPubSub::process_queue, this);
    processing_thread.detach();

    return true;
}

void CombinedPubSub::process_queue()
{
    while (!stop_signal_)
    {

        if (!message_queue.empty())
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            NetboxMessage1 message = message_queue.front();  // Get the first message
            message_queue.pop_front();  // Remove the message from the deque
            lock.unlock();

            uint64_t msg_id = message.id();
            if (writer_map.find(msg_id) != writer_map.end())
            {
                for (auto writer : writer_map[msg_id])
                {
                    writer->write(&message);
                    std::cout << "Message written by writer with ID: " << msg_id << std::endl;
                }
            }
            else
            {
                std::cerr << "No writers found for message ID: " << msg_id << std::endl;
            }
        }
        else
        {
         //   lock.unlock();
         //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void CombinedPubSub::run()
{
    signal(SIGINT, [](int){ stop_signal_ = true; });

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
       // std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <fstream>


using json = nlohmann::json;
std::mutex mtx;  // Mutex for thread-safe access to the deque
std::deque<NetboxMessage1> message_queue;  // Deque to store messages

using namespace eprosima::fastdds::dds;

class CombinedPubSub
{
public:
    CombinedPubSub(const std::string& unicast_ip, const std::vector<Config>& configs);
    ~CombinedPubSub();
    bool init();
    void run();

private:
    std::string unicast_ip_;
    std::vector<Config> configs_;

    std::vector<DomainParticipant*> participants_;
    std::vector<Subscriber*> subscribers_;
    std::vector<Publisher*> publishers_;
    std::vector<DataReader*> readers_;
    std::vector<DataWriter*> writers_;
    std::vector<Topic*> topics_;
    std::vector<Topic*> pub_topics_;

    TypeSupport type1_;
    int unicast_port_base_ = 9161;  // Starting unicast port

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(DataWriter* writer, int msg_id)
            : writer_(writer), msg_id_(msg_id), received_samples(0) {}

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
                sample.id(msg_id_);  // Assign custom ID
                message_queue.push_back(sample);  // Add to deque

                std::cout << "Message received and queued with new ID: " << sample.id() << std::endl;
                ++received_samples;
            }
            else
            {
                std::cout << "Read failed with error code." << std::endl;
            }
        }

        DataWriter* writer_;
        int msg_id_;
        uint32_t received_samples;
    };

    std::vector<PubSubListener> listeners_;

    static volatile sig_atomic_t stop_signal;
    static void handle_interrupt(int) { stop_signal = 1; }
    
    void process_queue();  // Separate function to process the queue

    int calculate_multicast_port(int domain_id);
    int get_next_unicast_port();
};

volatile sig_atomic_t CombinedPubSub::stop_signal = 0;

CombinedPubSub::CombinedPubSub(const std::string& unicast_ip, const std::vector<Config>& configs)
    : unicast_ip_(unicast_ip), configs_(configs), type1_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    for (size_t i = 0; i < readers_.size(); ++i)
    {
        if (readers_[i] && subscribers_[i])
        {
            subscribers_[i]->delete_datareader(readers_[i]);
        }
    }
    for (size_t i = 0; i < subscribers_.size(); ++i)
    {
        if (subscribers_[i] && participants_[i])
        {
            participants_[i]->delete_subscriber(subscribers_[i]);
        }
    }
    for (size_t i = 0; i < topics_.size(); ++i)
    {
        if (topics_[i] && participants_[i])
        {
            participants_[i]->delete_topic(topics_[i]);
        }
    }
    for (size_t i = 0; i < publishers_.size(); ++i)
    {
        if (publishers_[i] && participants_[i])
        {
            participants_[i]->delete_publisher(publishers_[i]);
        }
    }
    for (size_t i = 0; i < pub_topics_.size(); ++i)
    {
        if (pub_topics_[i] && participants_[i])
        {
            participants_[i]->delete_topic(pub_topics_[i]);
        }
    }
    for (size_t i = 0; i < participants_.size(); ++i)
    {
        if (participants_[i])
        {
            DomainParticipantFactory::get_instance()->delete_participant(participants_[i]);
        }
    }
}

int CombinedPubSub::calculate_multicast_port(int domain_id)
{
    int base_port = 7400;
    int offset = 1;
    return base_port + (250 * domain_id) + offset;
}

int CombinedPubSub::get_next_unicast_port()
{
    int current_port = unicast_port_base_;
    unicast_port_base_ += 3;  // Increment for the next reader
    return current_port;
}

bool CombinedPubSub::init()
{
    for (const auto& config : configs_)
    {
        // Create Subscriber
        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant");
        DomainParticipant* sub_participant = DomainParticipantFactory::get_instance()->create_participant(config.source_domain_id, sub_participant_qos);
        if (!sub_participant)
            return false;
        participants_.push_back(sub_participant);
        type1_.register_type(sub_participant);

        DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
        datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        
        
        datareader_qos.endpoint().unicast_locator_list.clear();
        eprosima::fastrtps::rtps::Locator_t unicast_locator;
        
        
        unicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
        unicast_locator.port = get_next_unicast_port();
        datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);
        
        
        eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
        sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip.c_str());
        sub_multicast_locator.port = calculate_multicast_port(config.source_domain_id);
        datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);
        
        
        
        Subscriber* subscriber = sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        if (!subscriber)
            return false;
        subscribers_.push_back(subscriber);

        Topic* topic = sub_participant->create_topic("7", type1_.get_type_name(), TOPIC_QOS_DEFAULT);
        if (!topic)
            return false;
        topics_.push_back(topic);

        DataReader* reader = subscriber->create_datareader(topic, datareader_qos, nullptr);
        if (!reader)
            return false;
        readers_.push_back(reader);

        // Create Publishers for each updated domain
        for (size_t i = 0; i < config.updated_domain_ids.size(); ++i)
        {
            DomainParticipantQos pub_participant_qos;
            pub_participant_qos.name("publisher_participant");
            DomainParticipant* pub_participant = DomainParticipantFactory::get_instance()->create_participant(config.updated_domain_ids[i], pub_participant_qos);
            if (!pub_participant)
                return false;
            participants_.push_back(pub_participant);
            type1_.register_type(pub_participant);

            Publisher* publisher = pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
            if (!publisher)
                return false;
            publishers_.push_back(publisher);

            Topic* pub_topic = pub_participant->create_topic("7", type1_.get_type_name(), TOPIC_QOS_DEFAULT);
            if (!pub_topic)
                return false;
            pub_topics_.push_back(pub_topic);

            DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
            datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};
            datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};
            datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};
            
            
            datawriter_qos.endpoint().multicast_locator_list.clear();
            eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
            pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, config.updated_destination_ips[i].c_str());
            pub_multicast_locator.port = calculate_multicast_port(config.updated_domain_ids[i]);
            datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

            DataWriter* writer = publisher->create_datawriter(pub_topic, datawriter_qos, nullptr);
            if (!writer)
                return false;
            writers_.push_back(writer);

            // Link reader and writer with a listener
            PubSubListener listener(writer, 999 + static_cast<int>(i));  // Custom message ID
            reader->set_listener(&listener);
            listeners_.push_back(listener);
        }
    }

    return true;
}

void CombinedPubSub::process_queue()
{
    while (!stop_signal)
    {
        if (!message_queue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    processing_thread.join();  // Ensure the processing thread is joined before exiting
    std::cout << "\nStopped" << std::endl;
}

int main()
{
    // Load configuration from JSON
    std::ifstream config_file("config2.json");
    json j;
    config_file >> j;
    config_file.close();

    std::vector<Config> configs;
    std::string unicast_ip = j["unicaste_ip"];

    for (const auto& config : j["configs"])
    {
        Config conf;
        conf.source_domain_id = config["source_domain_id"];
        conf.source_destination_ip = config["source_destination_ip"];
        for (const auto& id : config["updated_domain_ids"])
        {
            conf.updated_domain_ids.push_back(id);
        }
        for (const auto& ip : config["updated_destination_ips"])
        {
            conf.updated_destination_ips.push_back(ip);
        }
        configs.push_back(conf);
    }

    CombinedPubSub combined_pubsub(unicast_ip, configs);
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
*/
