#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <iostream>

using namespace eprosima::fastdds::dds;

void print_qos(const DataReaderQos& qos)
{
    std::cout << "Durability: " << qos.durability().kind << std::endl;
    std::cout << "Deadline: " << qos.deadline().period.nanosec << " nanoseconds" << std::endl;
    std::cout << "Latency Budget: " << qos.latency_budget().duration.nanosec << " nanoseconds" << std::endl;
    std::cout << "Liveliness: " << qos.liveliness().kind << std::endl;
    std::cout << "Reliability: " << qos.reliability().kind << std::endl;
    std::cout << "Destination Order: " << qos.destination_order().kind << std::endl;
    std::cout << "History: " << qos.history().kind << " with depth " << qos.history().depth << std::endl;
    std::cout << "Resource Limits: " << qos.resource_limits().max_samples << " max samples" << std::endl;
}

int main()
{
    // Retrieve the default DataReader QoS
    DataReaderQos default_qos = DATAREADER_QOS_DEFAULT;

    // Print the default QoS settings
    std::cout << "Default DataReader QoS Settings:" << std::endl;
    print_qos(default_qos);

    return 0;
}

