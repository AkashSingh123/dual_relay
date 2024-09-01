#include "ShapePublisher.hpp"
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
    bool setup_reader_writer(const Config& config);

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
            : writers_(writers), received_samples_(0) {}

        void on_data_available(DataReader* reader) override
        {
            NetboxMessage1 sample;
            SampleInfo info;
            if (reader->take_next_sample(&sample, &info) == ReturnCode_t::RETCODE_OK && info.valid_data)
            {
                ++received_samples_;
                std::cout << "Message received, ID: " << sample.id() << " | Total: " << received_samples_ << std::endl;

                for (auto writer : writers_)
                {
                    writer->write(&sample);
                }
            }
        }

    private:
        std::vector<DataWriter*> writers_;
        uint32_t received_samples_;
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

bool CombinedPubSub::setup_reader_writer(const Config& config)
{
    // Create DomainParticipant for the reader
    DomainParticipantQos sub_participant_qos;
    sub_participant_qos.name("subscriber_participant");
    DomainParticipant* sub_participant = DomainParticipantFactory::get_instance()->create_participant(config.source_domain_id, sub_participant_qos);
    if (!sub_participant)
        return false;
    participants_.push_back(sub_participant);
    type_.register_type(sub_participant);

    // Create DataReader
    Subscriber* subscriber = sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (!subscriber)
        return false;
    subscribers_.push_back(subscriber);

    DataReaderQos datareader_qos = DATAREADER_QOS_DEFAULT;
    datareader_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    datareader_qos.history().kind = KEEP_LAST_HISTORY_QOS;
    datareader_qos.durability().kind = VOLATILE_DURABILITY_QOS;


   /* uint16_t port;
     if (updated_domain_id == 8)
     {
       port = 9415;
     }
    else if (updated_domain_id == 9)
    {
      port = 9163;
    }
    else if (updated_domain_id == 7)
    {
      port = 9667;
    }
    else
    {
    // Default port or handle error
      port = 7663; // You can replace this with a suitable default or error handling
    }*/



    datareader_qos.endpoint().unicast_locator_list.clear();
    eprosima::fastrtps::rtps::Locator_t unicast_locator;
    unicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(unicast_locator, "192.168.50.29");
    unicast_locator.port = 7663;
    datareader_qos.endpoint().unicast_locator_list.push_back(unicast_locator);

    eprosima::fastrtps::rtps::Locator_t sub_multicast_locator;
    sub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
    eprosima::fastrtps::rtps::IPLocator::setIPv4(sub_multicast_locator, config.source_destination_ip);
    sub_multicast_locator.port = 7900;
    datareader_qos.endpoint().multicast_locator_list.push_back(sub_multicast_locator);

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    topic_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    Topic* topic = sub_participant->create_topic("7", type_.get_type_name(), topic_qos);
    if (!topic)
        return false;
    topics_.push_back(topic);

    // Create DomainParticipants and DataWriters for each destination
    std::vector<DataWriter*> writers;
    for (size_t i = 0; i < config.updated_domain_ids.size(); ++i)
    {
        int domain_id = config.updated_domain_ids[i];
        const std::string& destination_ip = config.updated_destination_ips[i];

        DomainParticipantQos pub_participant_qos;
        pub_participant_qos.name("publisher_participant");
        DomainParticipant* pub_participant = DomainParticipantFactory::get_instance()->create_participant(domain_id, pub_participant_qos);
        if (!pub_participant)
            return false;
        participants_.push_back(pub_participant);
        type_.register_type(pub_participant);

        Publisher* publisher = pub_participant->create_publisher(PUBLISHER_QOS_DEFAULT);
        if (!publisher)
            return false;
        publishers_.push_back(publisher);

        Topic* pub_topic = pub_participant->create_topic("pos", type_.get_type_name(), topic_qos);
        if (!pub_topic)
            return false;
        topics_.push_back(pub_topic);

        DataWriterQos datawriter_qos = DATAWRITER_QOS_DEFAULT;
        datawriter_qos.reliability().kind = RELIABLE_RELIABILITY_QOS;
         datawriter_qos.reliable_writer_qos().times.initialHeartbeatDelay = {0, 0};

         // Set heartbeat period to 1 second (can be adjusted as needed)
         datawriter_qos.reliable_writer_qos().times.heartbeatPeriod = {1, 0};  // 1 second interval

          // Set nack response delay to a low value for quick response
         datawriter_qos.reliable_writer_qos().times.nackResponseDelay = {0, 10};  // 10 milliseconds */



        eprosima::fastrtps::rtps::Locator_t pub_multicast_locator;
        pub_multicast_locator.kind = LOCATOR_KIND_UDPv4;
        eprosima::fastrtps::rtps::IPLocator::setIPv4(pub_multicast_locator, destination_ip);
        pub_multicast_locator.port = 7900;
        datawriter_qos.endpoint().multicast_locator_list.push_back(pub_multicast_locator);

        DataWriter* writer = publisher->create_datawriter(pub_topic, datawriter_qos, nullptr);
        if (!writer)
            return false;
        writers_.push_back(writer);
        writers.push_back(writer);
    }

    // Create and attach listener to the reader
    PubSubListener* listener = new PubSubListener(writers);
    listeners_.push_back(listener);

    DataReader* reader = subscriber->create_datareader(topic, datareader_qos, listener);
    if (!reader)
        return false;
    readers_.push_back(reader);

    return true;
}

bool CombinedPubSub::init(const std::string& config_file)
{
    if (!load_config(config_file))
    {
        return false;
    }

    for (const auto& config : configs_)
    {
        if (!setup_reader_writer(config))
        {
            return false;
        }
    }

    return true;
}

void CombinedPubSub::run()
{
    signal(SIGINT, handle_interrupt);

    std::cout << "Waiting for data" << std::endl;
    while (!stop_signal_)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
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
