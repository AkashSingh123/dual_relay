#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>
#include "NetboxMessage1PubSubTypes.h"
#include "globals.hpp"
#include "NetboxMessage2PubSubTypes.h"
using namespace eprosima::fastdds::dds;

class ShapePublisher
{
public:
    ShapePublisher();

    ~ShapePublisher();

    bool init(bool with_security);

    void run();

private:
    DomainParticipant *participant_;
    Publisher *publisher_;
    Topic *topic_;
    DataWriter *writer_;
    TypeSupport type_;
    //std::thread tshark_thread_; // 0x0005Thread for capturing data

    class SubscriberListener : public DataWriterListener
    {
    public:
        void on_publication_matched(DataWriter *writer, const PublicationMatchedStatus &info) override;
        int matched = 0;
    } listener_;
    
    //static void capture_tshark();
   // void publish_data(const std::vector<uint8_t>& payload);

};


/*
using namespace eprosima::fastdds::dds;

class ShapePublisher
{
public:
    ShapePublisher();
    ~ShapePublisher();
    bool init(bool with_security);
    void run();

private:
    Topic *topic2;
    Topic *topic3;
    TypeSupport type_;
  //  std::thread tshark_thread_; // Thread for capturing data

    class SubscriberListener : public DataWriterListener
    {
    public:
        void on_publication_matched(DataWriter *writer, const PublicationMatchedStatus &info) override;
        int matched = 0;
    } listener_;

   // static void capture_tshark();
};

/*
using namespace eprosima::fastdds::dds;

class ShapePublisher
{
public:
    ShapePublisher();
    ~ShapePublisher();
    bool init(bool with_security);
    void run();

private:
    Topic *topic1;
    Topic *topic2;
    Topic *topic3;
    TypeSupport type_;
  //  std::thread tshark_thread_; // Thread for capturing data

    class SubscriberListener : public DataWriterListener
    {
    public:
        void on_publication_matched(DataWriter *writer, const PublicationMatchedStatus &info) override;
        int matched = 0;
    } listener_;

   // static void capture_tshark();
};
*/




/*

#ifndef SHAPE_PUBLISHER_HPP
#define SHAPE_PUBLISHER_HPP

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/qos/TopicQos.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/core/policy/QosPolicies.hpp>
#include <iostream>
#include <vector>
#include <string>
#include "NetboxMessagePubSubTypes.h"
#include <json/json.h> // Include the JSON library header
#include <condition_variable>
#include <pcap.h>

using namespace eprosima::fastdds::dds;

/*struct Config {
    int source_domain_id;
    std::string source_destination_ip;
    int updated_domain_id;
    std::string updated_destination_ip;
};

struct WriterData {
    DataWriter* writer;
    std::string source_destination_ip;
    std::string updated_destination_ip;
};

struct ParticipantData {
    DomainParticipant* participant;
    Publisher* publisher;
    Topic* topic;
    std::vector<WriterData> writers_data;
};
*/
/*
class ShapePublisher
{
public:
    ShapePublisher();
    ~ShapePublisher();
    bool init(bool with_security);
    void run();
    void load_configs_from_file(const std::string& filename);
private:
    //void capture_rtps();
    TypeSupport type_;
    //std::vector<ParticipantData> participants_data;
    //static std::vector<Config> configs; // Declare the static member

    
    class SubscriberListener : public DataWriterListener
    {
    public:
        void on_publication_matched(
                DataWriter* writer,
                const PublicationMatchedStatus& info) override;
        int matched = 0;
    } listener_;

    void print_all_participants_data() const;

};

#endif // SHAPE_PUBLISHER_HPP 
*/

