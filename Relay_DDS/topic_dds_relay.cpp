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
#include <unordered_set>  // Add this line

std::thread reader_thread_;
std::thread writer_thread_;

using namespace eprosima::fastdds::dds;

// Global queue and mutex
std::deque<NetboxMessage1> message_queue;
std::mutex queue_mutex;
                                                                                                               
// Global set to store generated message IDs for filtering
std::unordered_set<int64_t> generated_ids; // Global variable


// Global map to track which writers are associated with which message IDs
std::unordered_map<uint64_t, std::vector<DataWriter*>> writer_map;

class CombinedPubSub
{
public:
    CombinedPubSub();
    ~CombinedPubSub();
    bool init();

    void run();

private:
    struct Config
    {
        int source_domain_id;
        std::string source_destination_ip;
        std::vector<int> updated_domain_ids;
        std::vector<std::string> updated_destination_ips;
        std::vector<std::string> topics;  // List of topics
        Json::Value reader_qos_settings;

        // QoS settings for DataWriter
        Json::Value writer_qos_settings;
        Json::Value topic_qos_settings;   // QoS settings for Topic
    };

    //bool load_config(const std::string& config_file);
    bool load_config();

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
   

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(std::vector<DataWriter*> writers, uint64_t msg_id)
            : writers_(writers), msg_id_(msg_id), received_count_(0){}

        void on_data_available(DataReader* reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;
         //   std::cout << "Message received and queued with new ID: " << sample.id() << std::endl;
            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
           // if (reader->take(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {

                if (generated_ids.find(sample.id()) == generated_ids.end())
                  {
                  
                   sample.id(msg_id_);
            // Process the message if its ID is not in the generated list
                  std::lock_guard<std::mutex> lock(queue_mutex);          
                  message_queue.push_back(sample);
                  received_count_++;  // Increment the counter
                  std::cout << "Message received and queued with ID: " << sample.id() 
                          << " | Total messages received: " << received_count_ << std::endl;

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
        uint64_t received_count_;  // Counter for received messages

    };

    std::vector<PubSubListener*> listeners_;
    static volatile sig_atomic_t stop_signal_;
    static void handle_interrupt(int) { stop_signal_ = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal_ = 0;

CombinedPubSub::CombinedPubSub() : type_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    stop_signal_ = 1;

    if (writer_thread_.joinable())
        writer_thread_.join();

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

bool CombinedPubSub::load_config()
{
    std::string config_file = "../config_pubsub_relay.json"; // Adjust path as needed

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
        for (const auto& topic : config["topics"])
        {
            cfg.topics.push_back(topic.asString());
        }
        
        
        // Load reader QoS settings
        cfg.reader_qos_settings = config["reader_qos_settings"];

        // Load writer QoS settings
        cfg.writer_qos_settings = config["writer_qos_settings"];
        
        cfg.topic_qos_settings = config["topic_qos_settings"]; // Load Topic QoS settings


        configs_.push_back(cfg);
    }
    return true;
}


bool CombinedPubSub::init()
{
    if (!load_config())
    {
        return false;
    }


    uint16_t unicast_port = 7446;

    for (size_t config_index = 0; config_index < configs_.size(); ++config_index)
    {
        const Config& config = configs_[config_index];
        int domain_id = config.source_domain_id;

        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant_" + std::to_string(config_index));
       // sub_participant_qos.transport().send_socket_buffer_size = 3048576;  // Set send buffer to 1MB
      //  sub_participant_qos.transport().listen_socket_buffer_size = 28194304;  // Set receive buffer to 4MB
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

        for (const auto& topic_name : config.topics)
        {
            DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
            
            
        if (config.reader_qos_settings.isMember("reliability")) {
            if (config.reader_qos_settings["reliability"].asString() == "RELIABLE_RELIABILITY_QOS")
                datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            else if (config.reader_qos_settings["reliability"].asString() == "BEST_EFFORT_RELIABILITY_QOS")
                datareader_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        }

        if (config.reader_qos_settings.isMember("durability")) {
            if (config.reader_qos_settings["durability"].asString() == "VOLATILE_DURABILITY_QOS")
                datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
            else if (config.reader_qos_settings["durability"].asString() == "TRANSIENT_LOCAL_DURABILITY_QOS")
                datareader_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        }

        if (config.reader_qos_settings.isMember("history")) {
            if (config.reader_qos_settings["history"].asString() == "KEEP_LAST_HISTORY_QOS")
                datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
            else if (config.reader_qos_settings["history"].asString() == "KEEP_ALL_HISTORY_QOS")
                datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
        }

        if (config.reader_qos_settings.isMember("history_depth")) {
            datareader_qos.history().depth = config.reader_qos_settings["history_depth"].asInt();
        }
        
        
        

            std::cout << "DataReader Reliability QoS: " << (datareader_qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;


            std::cout << "DataReader History QoS: " << (datareader_qos.history().kind == KEEP_LAST_HISTORY_QOS ? "KEEP_LAST" : "KEEP_ALL") << std::endl;

 
            std::cout << "DataReader History Depth: " << datareader_qos.history().depth << std::endl;


            std::cout << "DataReader Durability QoS: " << (datareader_qos.durability().kind == VOLATILE_DURABILITY_QOS ? "VOLATILE" : "TRANSIENT_LOCAL") << std::endl;

            
            datareader_qos.latency_budget().duration = {0, 200000000}; // No latency budget

         //   datareader_qos.deadline().period = eprosima::fastrtps::Duration_t(1, 0); // 1 second

         
          //  datareader_qos.latency_budget().duration = {0, 0};
          //  datareader_qos.durability().kind = TRANSIENT_DURABILITY_QOS;

            datareader_qos.resource_limits().max_samples = 30000000000; // Set a high number to accumulate more data
            datareader_qos.resource_limits().max_instances = 1000000;  // Adjust as needed
            datareader_qos.resource_limits().max_samples_per_instance = 135421;

            datareader_qos.reliable_reader_qos().disable_positive_ACKs.enabled = false;
 
            datareader_qos.reliable_reader_qos().times.initialAcknackDelay = {0, 70000000};
            

            datareader_qos.reliable_reader_qos().times.heartbeatResponseDelay = {0, 5000000}; // 5 milliseconds



            datareader_qos.endpoint().unicast_locator_list.clear();
            eprosima::fastrtps::rtps::Locator_t unicast_locator;
            unicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
            

            unicast_locator.port = unicast_port;
            unicast_port=unicast_port+2;
            
            std::cout << "unicaste port " << unicast_locator.port << " for Topic: " << topic_name << " in Config Index: " << config_index << std::endl;
            datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);


            datareader_qos.endpoint().multicast_locator_list.clear();
            eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
            sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip);
            
            int base_port = 7400;
            int offset = 1;
            int multicast_port = base_port + (250 * domain_id) + offset;
            sub_multicast_locator.port = multicast_port;
            datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

            TopicQos topic_qos = TOPIC_QOS_DEFAULT;
            
            if (config.topic_qos_settings.isMember("reliability")) {
              if (config.topic_qos_settings["reliability"].asString() == "RELIABLE_RELIABILITY_QOS")
                topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
              else if (config.topic_qos_settings["reliability"].asString() == "BEST_EFFORT_RELIABILITY_QOS")
                topic_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
             }
            
            
            if (config.topic_qos_settings.isMember("durability")) {
               if (config.topic_qos_settings["durability"].asString() == "VOLATILE_DURABILITY_QOS") {
                 topic_qos.durability().kind = VOLATILE_DURABILITY_QOS;
            } else if (config.topic_qos_settings["durability"].asString() == "TRANSIENT_LOCAL_DURABILITY_QOS") {
               topic_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
             }
            }

            std::cout << " TOPIC Reliability QoS: " << (topic_qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;
            std::cout << " TOPIC Durability QoS: " << (topic_qos.durability().kind == VOLATILE_DURABILITY_QOS ? "VOLATILE" : "TRANSIENT_LOCAL") << std::endl;

            
            Topic* topic = sub_participant->create_topic(topic_name, type_.get_type_name(), topic_qos);
            if (!topic)
            {
                std::cerr << "Failed to create topic for config " << config_index << std::endl;
                return false;
            }
            
            topics_.push_back(topic);

            std::vector<DataWriter*> writers;
            
            int64_t self_generated_id = -static_cast<int64_t>(std::hash<std::string>{}(std::to_string(config_index) + topic_name));
            
            if (self_generated_id > 0)
            {
                self_generated_id = -self_generated_id;
            }

            std::cout << "Generated msg_id: " << self_generated_id << " for Topic: " << topic_name << " in Config Index: " << config_index << std::endl;

            // Add the generated ID to the global set
            generated_ids.insert(self_generated_id);



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

                Topic* pub_topic = pub_participant->create_topic(topic_name, type_.get_type_name(), topic_qos);
                if (!pub_topic)
                {
                    std::cerr << "Failed to create publisher topic for config " << config_index << ", destination " << i << std::endl;
                    return false;
                }
                topics_.push_back(pub_topic);

                DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
                
            if (config.writer_qos_settings.isMember("reliability")) {
                if (config.writer_qos_settings["reliability"].asString() == "RELIABLE_RELIABILITY_QOS")
                    datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
                else if (config.writer_qos_settings["reliability"].asString() == "BEST_EFFORT_RELIABILITY_QOS")
                    datawriter_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
            }

            if (config.writer_qos_settings.isMember("durability")) {
                if (config.writer_qos_settings["durability"].asString() == "VOLATILE_DURABILITY_QOS")
                    datawriter_qos.durability().kind = VOLATILE_DURABILITY_QOS;
                else if (config.writer_qos_settings["durability"].asString() == "TRANSIENT_LOCAL_DURABILITY_QOS")
                    datawriter_qos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
            }

            if (config.writer_qos_settings.isMember("history")) {
                if (config.writer_qos_settings["history"].asString() == "KEEP_LAST_HISTORY_QOS")
                    datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS;
                else if (config.writer_qos_settings["history"].asString() == "KEEP_ALL_HISTORY_QOS")
                    datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;
            }

            if (config.writer_qos_settings.isMember("history_depth")) {
                datawriter_qos.history().depth = config.writer_qos_settings["history_depth"].asInt();
            }
            
            
             //   datawriter_qos.history().depth = 10;  // Keep the last 10 samples
                
                

             std::cout << "DataWriter Reliability QoS: " << (datawriter_qos.reliability().kind == RELIABLE_RELIABILITY_QOS ? "RELIABLE" : "BEST_EFFORT") << std::endl;


             std::cout << "DataWriter History QoS: " << (datawriter_qos.history().kind == KEEP_LAST_HISTORY_QOS ? "KEEP_LAST" : "KEEP_ALL") << std::endl;


             std::cout << "DataWriter History Depth: " << datawriter_qos.history().depth << std::endl;


             std::cout << "DataWriter Durability QoS: " << (datawriter_qos.durability().kind == VOLATILE_DURABILITY_QOS ? "VOLATILE" : "TRANSIENT_LOCAL") << std::endl;


                datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};
                datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {2, 0};  
                datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 5000000};  


                datawriter_qos.resource_limits().max_samples = 3000000000000; // Example high limit
                datawriter_qos.resource_limits().max_instances = 1010000;    // Example high limit
                datawriter_qos.resource_limits().max_samples_per_instance = 13540; // Example high limit
 


                eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
                pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
                eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);
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

                writer_map[self_generated_id] = writers;

                std::cout << "Created DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
                                // Print Writer details
                std::cout << "Created DataWriter:" << std::endl;
                std::cout << "  Domain ID: " << domain_id << std::endl;
                std::cout << "  Destination IP: " << destination_ip << std::endl;
                std::cout << "  Topic: " << topic_name << std::endl;

            }

            PubSubListener* listener = new PubSubListener(writers, self_generated_id);
            listeners_.push_back(listener);

            DataReader* reader = subscriber->create_datareader(topic, datareader_qos, listener);
            
            if (!reader)
            {
                std::cerr << "Failed to create DataReader for config " << config_index << std::endl;
                return false;
            }
            readers_.push_back(reader);
            
            std::cout << "Created DataReader:" << std::endl;
            std::cout << "  Domain ID: " << config.source_domain_id << std::endl;
            std::cout << "  Source IP: " << config.source_destination_ip << std::endl;
            std::cout << "  Topic: " << topic_name << std::endl;

        }
    }



    writer_thread_ = std::thread(&CombinedPubSub::process_queue, this);


  //  std::thread processing_thread(&CombinedPubSub::process_queue, this);
 //   processing_thread.detach();

    return true;
}

void CombinedPubSub::process_queue()
{
    while (!stop_signal_)
    {

        if (!message_queue.empty())
        {
        

            NetboxMessage1 message = message_queue.front();
            message_queue.pop_front();
            

            int64_t msg_id = message.id();
     
            if (writer_map.find(msg_id) != writer_map.end())
            {
                for (auto writer : writer_map[msg_id])
                {   
                    //std::lock_guard<std::mutex> lock(queue_mutex);          
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
           // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void CombinedPubSub::run()
{
    signal(SIGINT, [](int){ stop_signal_ = true; });

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
    }
    std::cout << "\nStopped" << std::endl;

    if (writer_thread_.joinable())
        writer_thread_.join();



}

int main(int argc)
{
   /* if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return 1;
    }*/

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




























/*#include "ShapePublisher.hpp"
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
#include <unordered_set>  // Add this line

std::thread reader_thread_;
std::thread writer_thread_;

using namespace eprosima::fastdds::dds;

// Global queue and mutex
std::deque<NetboxMessage1> message_queue;
std::mutex queue_mutex;
                                                                                                               
// Global set to store generated message IDs for filtering
std::unordered_set<int64_t> generated_ids; // Global variable


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
        std::vector<std::string> topics;  // List of topics
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
   

    class PubSubListener : public DataReaderListener
    {
    public:
        PubSubListener(std::vector<DataWriter*> writers, uint64_t msg_id)
            : writers_(writers), msg_id_(msg_id), received_count_(0){}

        void on_data_available(DataReader* reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;
         //   std::cout << "Message received and queued with new ID: " << sample.id() << std::endl;
            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
           // if (reader->take(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {

                if (generated_ids.find(sample.id()) == generated_ids.end())
                  {
                  
                   sample.id(msg_id_);
            // Process the message if its ID is not in the generated list
                  std::lock_guard<std::mutex> lock(queue_mutex);          
                  message_queue.push_back(sample);
                  received_count_++;  // Increment the counter
                  std::cout << "Message received and queued with ID: " << sample.id() 
                          << " | Total messages received: " << received_count_ << std::endl;

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
        uint64_t received_count_;  // Counter for received messages

    };

    std::vector<PubSubListener*> listeners_;
    static volatile sig_atomic_t stop_signal_;
    static void handle_interrupt(int) { stop_signal_ = 1; }
};

volatile sig_atomic_t CombinedPubSub::stop_signal_ = 0;

CombinedPubSub::CombinedPubSub() : type_(new NetboxMessage1PubSubType()) {}

CombinedPubSub::~CombinedPubSub()
{
    stop_signal_ = 1;

    if (writer_thread_.joinable())
        writer_thread_.join();

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
        for (const auto& topic : config["topics"])
        {
            cfg.topics.push_back(topic.asString());
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

    uint16_t unicast_port = 7446;

    for (size_t config_index = 0; config_index < configs_.size(); ++config_index)
    {
        const Config& config = configs_[config_index];
        int domain_id = config.source_domain_id;

        DomainParticipantQos sub_participant_qos;
        sub_participant_qos.name("subscriber_participant_" + std::to_string(config_index));
       // sub_participant_qos.transport().send_socket_buffer_size = 3048576;  // Set send buffer to 1MB
      //  sub_participant_qos.transport().listen_socket_buffer_size = 28194304;  // Set receive buffer to 4MB
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

        for (const auto& topic_name : config.topics)
        {
            DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
            datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
            datareader_qos.history().kind = KEEP_ALL_HISTORY_QOS;
            datareader_qos.history().depth = 10;
            datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;
            datareader_qos.latency_budget().duration = {0, 0}; // No latency budget

         //   datareader_qos.deadline().period = eprosima::fastrtps::c_TimeInfinite;  // Match infinite deadline
          //  datareader_qos.latency_budget().duration = {0, 0};
          //  datareader_qos.durability().kind = TRANSIENT_DURABILITY_QOS;

            datareader_qos.resource_limits().max_samples = 30000000000; // Set a high number to accumulate more data
            datareader_qos.resource_limits().max_instances = 1000000;  // Adjust as needed
            datareader_qos.resource_limits().max_samples_per_instance = 135421;

            datareader_qos.reliable_reader_qos().disable_positive_ACKs.enabled = false;
 
            datareader_qos.reliable_reader_qos().times.initialAcknackDelay = {0, 70000000};
            
          //  datareader_qos.reliable_reader_qos().times.AcknackDelay = {0, 70000000};

            datareader_qos.reliable_reader_qos().times.heartbeatResponseDelay = {0, 5000000}; // 5 milliseconds



        //    datareader_qos.reliable_reader_qos().times.nackResponseDelay = {0, 5000000};  // 5 milliseconds





            datareader_qos.endpoint().unicast_locator_list.clear();
            eprosima::fastrtps::rtps::Locator_t unicast_locator;
            unicast_locator.kind = LOCATOR_KIND_UDPv4;
            eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, unicast_ip_);
            

            unicast_locator.port = unicast_port;
            unicast_port=unicast_port+2;
            
            std::cout << "unicaste port " << unicast_locator.port << " for Topic: " << topic_name << " in Config Index: " << config_index << std::endl;
            datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);


            datareader_qos.endpoint().multicast_locator_list.clear();
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
            
            
            Topic* topic = sub_participant->create_topic(topic_name, type_.get_type_name(), topic_qos);
            if (!topic)
            {
                std::cerr << "Failed to create topic for config " << config_index << std::endl;
                return false;
            }
            
            topics_.push_back(topic);

            std::vector<DataWriter*> writers;
            
            int64_t self_generated_id = -static_cast<int64_t>(std::hash<std::string>{}(std::to_string(config_index) + topic_name));
            
            if (self_generated_id > 0)
            {
                self_generated_id = -self_generated_id;
            }

            std::cout << "Generated msg_id: " << self_generated_id << " for Topic: " << topic_name << " in Config Index: " << config_index << std::endl;

            // Add the generated ID to the global set
            generated_ids.insert(self_generated_id);



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

                Topic* pub_topic = pub_participant->create_topic(topic_name, type_.get_type_name(), topic_qos);
                if (!pub_topic)
                {
                    std::cerr << "Failed to create publisher topic for config " << config_index << ", destination " << i << std::endl;
                    return false;
                }
                topics_.push_back(pub_topic);

                DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
                datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
                //datawriter_qos.history().kind = KEEP_ALL_HISTORY_QOS;
                datawriter_qos.history().kind = KEEP_LAST_HISTORY_QOS;
                datawriter_qos.history().depth = 10;  // Keep the last 10 samples

                datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};
                datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {2, 0};  
                datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 5000000};  


                datawriter_qos.resource_limits().max_samples = 3000000000000; // Example high limit
                datawriter_qos.resource_limits().max_instances = 1010000;    // Example high limit
                datawriter_qos.resource_limits().max_samples_per_instance = 13540; // Example high limit
 


                eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
                pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
                eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);
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

                writer_map[self_generated_id] = writers;

                std::cout << "Created DataWriter for Domain ID: " << domain_id << ", Destination IP: " << destination_ip << std::endl;
                                // Print Writer details
                std::cout << "Created DataWriter:" << std::endl;
                std::cout << "  Domain ID: " << domain_id << std::endl;
                std::cout << "  Destination IP: " << destination_ip << std::endl;
                std::cout << "  Topic: " << topic_name << std::endl;

            }

            PubSubListener* listener = new PubSubListener(writers, self_generated_id);
            listeners_.push_back(listener);

            DataReader* reader = subscriber->create_datareader(topic, datareader_qos, listener);
            
            if (!reader)
            {
                std::cerr << "Failed to create DataReader for config " << config_index << std::endl;
                return false;
            }
            readers_.push_back(reader);
            
            std::cout << "Created DataReader:" << std::endl;
            std::cout << "  Domain ID: " << config.source_domain_id << std::endl;
            std::cout << "  Source IP: " << config.source_destination_ip << std::endl;
            std::cout << "  Topic: " << topic_name << std::endl;

        }
    }



    writer_thread_ = std::thread(&CombinedPubSub::process_queue, this);


  //  std::thread processing_thread(&CombinedPubSub::process_queue, this);
 //   processing_thread.detach();

    return true;
}

void CombinedPubSub::process_queue()
{
    while (!stop_signal_)
    {

        if (!message_queue.empty())
        {
        

            NetboxMessage1 message = message_queue.front();
            message_queue.pop_front();
            

            int64_t msg_id = message.id();
     
            if (writer_map.find(msg_id) != writer_map.end())
            {
                for (auto writer : writer_map[msg_id])
                {   
                    //std::lock_guard<std::mutex> lock(queue_mutex);          
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
           // std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void CombinedPubSub::run()
{
    signal(SIGINT, [](int){ stop_signal_ = true; });

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
    }
    std::cout << "\nStopped" << std::endl;

    if (writer_thread_.joinable())
        writer_thread_.join();



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
}*/

