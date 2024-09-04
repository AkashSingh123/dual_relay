#ifndef SUBSCRIBER_HPP_
#define SUBSCRIBER_HPP_

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>

class RawDataSubscriber
{
public:
    RawDataSubscriber();
    virtual ~RawDataSubscriber();
    bool init(bool with_security);
    void run();

private:
    eprosima::fastdds::dds::DomainParticipant *participant_;
    eprosima::fastdds::dds::Subscriber *subscriber_;
    eprosima::fastdds::dds::Topic *topic_;
    eprosima::fastdds::dds::DataReader *reader_;
    class SubscriberListener : public eprosima::fastdds::dds::DataReaderListener
    {
    public:
        SubscriberListener() : matched(0), received_samples(0) {}
        ~SubscriberListener() override {}
        void on_data_available(eprosima::fastdds::dds::DataReader *reader) override;
        void on_subscription_matched(eprosima::fastdds::dds::DataReader *reader, const eprosima::fastdds::dds::SubscriptionMatchedStatus &info) override;

        int matched;
        uint32_t received_samples;
    } listener_;
};

#endif
