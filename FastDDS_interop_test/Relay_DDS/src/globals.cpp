#include "globals.hpp"
std::vector<uint8_t> last_command_payload;
std::vector<uint8_t> last_swamp_gcs_payload;
std::vector<uint8_t> last_target_update_payload;
std::vector<uint8_t> last_zones_payload;
std::string captured_data;
std::string captured_topic; // Add this line
eprosima::fastdds::dds::DomainParticipant* participant1 = nullptr;
eprosima::fastdds::dds::DomainParticipant* participant2 = nullptr;
eprosima::fastdds::dds::DomainParticipant* participant3 = nullptr;
eprosima::fastdds::dds::Publisher* publisher1 = nullptr;
eprosima::fastdds::dds::Publisher* publisher2 = nullptr;
eprosima::fastdds::dds::Publisher* publisher3 = nullptr;
eprosima::fastdds::dds::DataWriter* writer1 = nullptr;
eprosima::fastdds::dds::DataWriter* writer2 = nullptr;
eprosima::fastdds::dds::DataWriter* writer3 = nullptr;
std::string destination_ip; // Initialize destination_ip
bool new_data = false;
std::vector<Config> configs; // Define the global variable
std::vector<ParticipantData> participants_data; // Define participants_data as an external variable


size_t initial_fragment_start = 0;
size_t subsequent_fragment_start = 0;
size_t submessage_0x15_start = 0;

//std::mutex data_mutex;

