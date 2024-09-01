#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <string>
#include <atomic>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <chrono>
/*struct Config {
    int source_domain_id;
    std::string source_destination_ip;
    int updated_domain_id;
    std::string updated_destination_ip;
};*/
struct Config {
    int source_domain_id;
    std::string source_destination_ip;
    std::vector<int> updated_domain_ids;  // Store multiple domain IDs
    std::vector<std::string> updated_destination_ips;  // Store multiple destination IPs
};

struct WriterData {
    eprosima::fastdds::dds::DataWriter* writer;
    std::string source_destination_ip;
    std::string updated_destination_ip;
};

struct ParticipantData {
    eprosima::fastdds::dds::DomainParticipant* participant;
    eprosima::fastdds::dds::Publisher* publisher;
    eprosima::fastdds::dds::Topic* topic;
    std::vector<WriterData> writers_data;
};


extern std::vector<uint8_t> last_command_payload;
extern std::vector<uint8_t> last_swamp_gcs_payload;
extern std::vector<uint8_t> last_target_update_payload;
extern std::vector<uint8_t> last_zones_payload;
extern std::string captured_data;
extern std::string captured_topic; // Add this line
extern eprosima::fastdds::dds::DomainParticipant* participant1;
extern eprosima::fastdds::dds::DomainParticipant* participant2;
extern eprosima::fastdds::dds::DomainParticipant* participant3;
extern eprosima::fastdds::dds::Publisher* publisher1;
extern eprosima::fastdds::dds::Publisher* publisher2;
extern eprosima::fastdds::dds::Publisher* publisher3;
extern eprosima::fastdds::dds::DataWriter* writer1;
extern eprosima::fastdds::dds::DataWriter* writer2;
extern eprosima::fastdds::dds::DataWriter* writer3;
extern std::string destination_ip; // Declare destination_ip
extern bool new_data;
extern std::vector<Config> configs; // Declare configs as an external variable
extern std::vector<ParticipantData> participants_data; // Declare participants_data as an external variable

//extern std::mutex data_mutex;

#endif // GLOBALS_HPP

