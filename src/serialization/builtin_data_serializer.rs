use crate::{
  structure::{
    locator::{LocatorList, LocatorData},
    parameter_id::ParameterId,
    guid::{GUID, GUIDData},
    builtin_endpoint::{
      BuiltinEndpointQosData, BuiltinEndpointSetData, BuiltinEndpointSet, BuiltinEndpointQos,
    },
    duration::{Duration, DurationData},
  },
  discovery::{
    content_filter_property::{ContentFilterPropertyData, ContentFilterProperty},
    data_types::{
      topic_data::{
        SubscriptionBuiltinTopicData, ReaderProxy, DiscoveredReaderData, WriterProxy,
        PublicationBuiltinTopicData, DiscoveredWriterData, TopicBuiltinTopicData,
      },
      spdp_participant_data::SPDPDiscoveredParticipantData,
    },
  },
  messages::{
    vendor_id::{VendorId, VendorIdData},
    protocol_version::{ProtocolVersion, ProtocolVersionData},
  },
  dds::qos::policy::{
    Deadline, Durability, LatencyBudget, Liveliness, Reliability, Ownership, DestinationOrder,
    TimeBasedFilter, Presentation, Lifespan, History, ResourceLimits,
  },
};
use serde::{Serialize, Serializer, ser::SerializeStruct, Deserialize};

#[derive(Serialize, Deserialize)]
struct StringData {
  parameter_id: ParameterId,
  parameter_length: u16,
  string_data: String,
}

impl StringData {
  pub fn new(parameter_id: ParameterId, string_data: &String) -> StringData {
    let parameter_length = string_data.len() as u16;
    let parameter_length = parameter_length + (4 - parameter_length % 4) + 4;
    StringData {
      parameter_id,
      parameter_length,
      string_data: string_data.clone(),
    }
  }
}

#[derive(Serialize, Deserialize)]
struct U32Data {
  parameter_id: ParameterId,
  parameter_length: u16,
  data: u32,
}

impl U32Data {
  pub fn new(parameter_id: ParameterId, data: u32) -> U32Data {
    U32Data {
      parameter_id,
      parameter_length: 4,
      data,
    }
  }
}

#[derive(Serialize, Deserialize)]
struct QosData<D>
where
  D: Serialize,
{
  parameter_id: ParameterId,
  parameter_length: u16,
  qos_param: D,
}

impl<D> QosData<D>
where
  D: Serialize + Clone,
{
  pub fn new(parameter_id: ParameterId, qosparam: &D) -> QosData<D> {
    QosData {
      parameter_id,
      parameter_length: 4,
      qos_param: qosparam.clone(),
    }
  }
}

#[derive(Serialize, Deserialize)]
struct ExpectsInlineQos {
  parameter_id: ParameterId,
  parameter_length: u16,
  expects_inline_qos: bool,
}

#[derive(Serialize, Deserialize)]
struct ManualLivelinessCount {
  parameter_id: ParameterId,
  parameter_length: u16,
  manual_liveliness_count: i32,
}

#[derive(Debug, Serialize, Deserialize)]
struct EntityName {
  parameter_id: ParameterId,
  parameter_length: u16,
  entity_name: String,
}

pub struct BuiltinDataSerializer<'a> {
  // Participant Data
  pub protocol_version: Option<&'a ProtocolVersion>,
  pub vendor_id: Option<&'a VendorId>,
  pub expects_inline_qos: Option<&'a bool>,
  pub participant_guid: Option<&'a GUID>,
  pub metatraffic_unicast_locators: Option<&'a LocatorList>,
  pub metatraffic_multicast_locators: Option<&'a LocatorList>,
  pub default_unicast_locators: Option<&'a LocatorList>,
  pub default_multicast_locators: Option<&'a LocatorList>,
  pub available_builtin_endpoints: Option<&'a BuiltinEndpointSet>,
  pub lease_duration: Option<&'a Duration>,
  pub manual_liveliness_count: Option<&'a i32>,
  pub builtin_enpoint_qos: Option<&'a BuiltinEndpointQos>,
  pub entity_name: Option<&'a String>,

  pub endpoint_guid: Option<&'a GUID>,

  // Reader Proxy
  pub unicast_locator_list: Option<&'a LocatorList>,
  pub multicast_locator_list: Option<&'a LocatorList>,

  // Writer Proxy
  pub data_max_size_serialized: Option<&'a u32>,

  // topic data
  pub topic_name: Option<&'a String>,
  pub type_name: Option<&'a String>,
  pub durability: Option<&'a Durability>,
  pub deadline: Option<&'a Deadline>,
  pub latency_budget: Option<&'a LatencyBudget>,
  pub liveliness: Option<&'a Liveliness>,
  pub reliability: Option<&'a Reliability>,
  pub ownership: Option<&'a Ownership>,
  pub destination_order: Option<&'a DestinationOrder>,
  pub time_based_filter: Option<&'a TimeBasedFilter>,
  pub presentation: Option<&'a Presentation>,
  pub lifespan: Option<&'a Lifespan>,
  pub history: Option<&'a History>,
  pub resource_limits: Option<&'a ResourceLimits>,

  pub content_filter_property: Option<&'a ContentFilterProperty>,
}

impl<'a> BuiltinDataSerializer<'a> {
  pub fn merge(mut self, other: BuiltinDataSerializer<'a>) -> BuiltinDataSerializer<'a> {
    self.protocol_version = match other.protocol_version {
      Some(v) => Some(v),
      None => self.protocol_version,
    };
    self.vendor_id = match other.vendor_id {
      Some(v) => Some(v),
      None => self.vendor_id,
    };
    self.expects_inline_qos = match other.expects_inline_qos {
      Some(v) => Some(v),
      None => self.expects_inline_qos,
    };
    self.participant_guid = match other.participant_guid {
      Some(v) => Some(v),
      None => self.participant_guid,
    };
    self.metatraffic_unicast_locators = match other.metatraffic_unicast_locators {
      Some(v) => Some(v),
      None => self.metatraffic_unicast_locators,
    };
    self.metatraffic_multicast_locators = match other.metatraffic_multicast_locators {
      Some(v) => Some(v),
      None => self.metatraffic_multicast_locators,
    };
    self.default_unicast_locators = match other.default_unicast_locators {
      Some(v) => Some(v),
      None => self.default_unicast_locators,
    };
    self.default_multicast_locators = match other.default_multicast_locators {
      Some(v) => Some(v),
      None => self.default_multicast_locators,
    };
    self.available_builtin_endpoints = match other.available_builtin_endpoints {
      Some(v) => Some(v),
      None => self.available_builtin_endpoints,
    };
    self.lease_duration = match other.lease_duration {
      Some(v) => Some(v),
      None => self.lease_duration,
    };
    self.manual_liveliness_count = match other.manual_liveliness_count {
      Some(v) => Some(v),
      None => self.manual_liveliness_count,
    };
    self.builtin_enpoint_qos = match other.builtin_enpoint_qos {
      Some(v) => Some(v),
      None => self.builtin_enpoint_qos,
    };
    self.entity_name = match other.entity_name {
      Some(v) => Some(v),
      None => self.entity_name,
    };
    self.endpoint_guid = match other.endpoint_guid {
      Some(v) => Some(v),
      None => self.endpoint_guid,
    };
    self.unicast_locator_list = match other.unicast_locator_list {
      Some(v) => Some(v),
      None => self.unicast_locator_list,
    };
    self.multicast_locator_list = match other.multicast_locator_list {
      Some(v) => Some(v),
      None => self.multicast_locator_list,
    };
    self.data_max_size_serialized = match other.data_max_size_serialized {
      Some(v) => Some(v),
      None => self.data_max_size_serialized,
    };
    self.topic_name = match other.topic_name {
      Some(v) => Some(v),
      None => self.topic_name,
    };
    self.type_name = match other.type_name {
      Some(v) => Some(v),
      None => self.type_name,
    };
    self.durability = match other.durability {
      Some(v) => Some(v),
      None => self.durability,
    };
    self.deadline = match other.deadline {
      Some(v) => Some(v),
      None => self.deadline,
    };
    self.latency_budget = match other.latency_budget {
      Some(v) => Some(v),
      None => self.latency_budget,
    };
    self.liveliness = match other.liveliness {
      Some(v) => Some(v),
      None => self.liveliness,
    };
    self.reliability = match other.reliability {
      Some(v) => Some(v),
      None => self.reliability,
    };
    self.ownership = match other.ownership {
      Some(v) => Some(v),
      None => self.ownership,
    };
    self.destination_order = match other.destination_order {
      Some(v) => Some(v),
      None => self.destination_order,
    };
    self.time_based_filter = match other.time_based_filter {
      Some(v) => Some(v),
      None => self.time_based_filter,
    };
    self.presentation = match other.presentation {
      Some(v) => Some(v),
      None => self.presentation,
    };
    self.lifespan = match other.lifespan {
      Some(v) => Some(v),
      None => self.lifespan,
    };
    self.history = match other.history {
      Some(v) => Some(v),
      None => self.history,
    };
    self.resource_limits = match other.resource_limits {
      Some(v) => Some(v),
      None => self.resource_limits,
    };
    self.content_filter_property = match other.content_filter_property {
      Some(v) => Some(v),
      None => self.content_filter_property,
    };

    self
  }

  pub fn from_participant_data(
    participant_data: &'a SPDPDiscoveredParticipantData,
  ) -> BuiltinDataSerializer<'a> {
    BuiltinDataSerializer {
      protocol_version: participant_data.protocol_version.as_ref(),
      vendor_id: participant_data.vendor_id.as_ref(),
      expects_inline_qos: participant_data.expects_inline_qos.as_ref(),
      participant_guid: participant_data.participant_guid.as_ref(),
      metatraffic_unicast_locators: Some(&participant_data.metatraffic_unicast_locators),
      metatraffic_multicast_locators: Some(&participant_data.metatraffic_multicast_locators),
      default_unicast_locators: Some(&participant_data.default_unicast_locators),
      default_multicast_locators: Some(&participant_data.default_multicast_locators),
      available_builtin_endpoints: participant_data.available_builtin_endpoints.as_ref(),
      lease_duration: participant_data.lease_duration.as_ref(),
      manual_liveliness_count: participant_data.manual_liveliness_count.as_ref(),
      builtin_enpoint_qos: participant_data.builtin_enpoint_qos.as_ref(),
      entity_name: participant_data.entity_name.as_ref(),
      endpoint_guid: None,
      unicast_locator_list: None,
      multicast_locator_list: None,
      data_max_size_serialized: None,
      topic_name: None,
      type_name: None,
      durability: None,
      deadline: None,
      latency_budget: None,
      liveliness: None,
      reliability: None,
      ownership: None,
      destination_order: None,
      time_based_filter: None,
      presentation: None,
      lifespan: None,
      history: None,
      resource_limits: None,
      content_filter_property: None,
    }
  }

  pub fn from_reader_proxy(reader_proxy: &'a ReaderProxy) -> BuiltinDataSerializer<'a> {
    BuiltinDataSerializer {
      protocol_version: None,
      vendor_id: None,
      expects_inline_qos: reader_proxy.expects_inline_qos.as_ref(),
      participant_guid: None,
      metatraffic_unicast_locators: None,
      metatraffic_multicast_locators: None,
      default_unicast_locators: None,
      default_multicast_locators: None,
      available_builtin_endpoints: None,
      lease_duration: None,
      manual_liveliness_count: None,
      builtin_enpoint_qos: None,
      entity_name: None,
      endpoint_guid: reader_proxy.remote_reader_guid.as_ref(),
      unicast_locator_list: Some(&reader_proxy.unicast_locator_list),
      multicast_locator_list: Some(&reader_proxy.multicast_locator_list),
      data_max_size_serialized: None,
      topic_name: None,
      type_name: None,
      durability: None,
      deadline: None,
      latency_budget: None,
      liveliness: None,
      reliability: None,
      ownership: None,
      destination_order: None,
      time_based_filter: None,
      presentation: None,
      lifespan: None,
      history: None,
      resource_limits: None,
      content_filter_property: None,
    }
  }

  pub fn from_writer_proxy(writer_proxy: &'a WriterProxy) -> BuiltinDataSerializer<'a> {
    BuiltinDataSerializer {
      protocol_version: None,
      vendor_id: None,
      expects_inline_qos: None,
      participant_guid: None,
      metatraffic_unicast_locators: None,
      metatraffic_multicast_locators: None,
      default_unicast_locators: None,
      default_multicast_locators: None,
      available_builtin_endpoints: None,
      lease_duration: None,
      manual_liveliness_count: None,
      builtin_enpoint_qos: None,
      entity_name: None,
      endpoint_guid: writer_proxy.remote_writer_guid.as_ref(),
      unicast_locator_list: Some(&writer_proxy.unicast_locator_list),
      multicast_locator_list: Some(&writer_proxy.multicast_locator_list),
      data_max_size_serialized: writer_proxy.data_max_size_serialized.as_ref(),
      topic_name: None,
      type_name: None,
      durability: None,
      deadline: None,
      latency_budget: None,
      liveliness: None,
      reliability: None,
      ownership: None,
      destination_order: None,
      time_based_filter: None,
      presentation: None,
      lifespan: None,
      history: None,
      resource_limits: None,
      content_filter_property: None,
    }
  }

  pub fn from_subscription_topic_data(
    subscription_topic_data: &'a SubscriptionBuiltinTopicData,
  ) -> BuiltinDataSerializer<'a> {
    BuiltinDataSerializer {
      protocol_version: None,
      vendor_id: None,
      expects_inline_qos: None,
      participant_guid: subscription_topic_data.participant_key.as_ref(),
      metatraffic_unicast_locators: None,
      metatraffic_multicast_locators: None,
      default_unicast_locators: None,
      default_multicast_locators: None,
      available_builtin_endpoints: None,
      lease_duration: None,
      manual_liveliness_count: None,
      builtin_enpoint_qos: None,
      entity_name: None,
      endpoint_guid: subscription_topic_data.key.as_ref(),
      unicast_locator_list: None,
      multicast_locator_list: None,
      data_max_size_serialized: None,
      topic_name: subscription_topic_data.topic_name.as_ref(),
      type_name: subscription_topic_data.type_name.as_ref(),
      durability: subscription_topic_data.durability.as_ref(),
      deadline: subscription_topic_data.deadline.as_ref(),
      latency_budget: subscription_topic_data.latency_budget.as_ref(),
      liveliness: subscription_topic_data.liveliness.as_ref(),
      reliability: subscription_topic_data.reliability.as_ref(),
      ownership: subscription_topic_data.ownership.as_ref(),
      destination_order: subscription_topic_data.destination_order.as_ref(),
      time_based_filter: subscription_topic_data.time_based_filter.as_ref(),
      presentation: subscription_topic_data.presentation.as_ref(),
      lifespan: subscription_topic_data.lifespan.as_ref(),
      history: None,
      resource_limits: None,
      content_filter_property: None,
    }
  }

  pub fn from_publication_topic_data(
    publication_topic_data: &'a PublicationBuiltinTopicData,
  ) -> BuiltinDataSerializer {
    BuiltinDataSerializer {
      protocol_version: None,
      vendor_id: None,
      expects_inline_qos: None,
      participant_guid: publication_topic_data.participant_key.as_ref(),
      metatraffic_unicast_locators: None,
      metatraffic_multicast_locators: None,
      default_unicast_locators: None,
      default_multicast_locators: None,
      available_builtin_endpoints: None,
      lease_duration: None,
      manual_liveliness_count: None,
      builtin_enpoint_qos: None,
      entity_name: None,
      endpoint_guid: publication_topic_data.key.as_ref(),
      unicast_locator_list: None,
      multicast_locator_list: None,
      data_max_size_serialized: None,
      topic_name: publication_topic_data.topic_name.as_ref(),
      type_name: publication_topic_data.type_name.as_ref(),
      durability: publication_topic_data.durability.as_ref(),
      deadline: publication_topic_data.deadline.as_ref(),
      latency_budget: publication_topic_data.latency_budget.as_ref(),
      liveliness: publication_topic_data.liveliness.as_ref(),
      reliability: publication_topic_data.reliability.as_ref(),
      ownership: publication_topic_data.ownership.as_ref(),
      destination_order: publication_topic_data.destination_order.as_ref(),
      time_based_filter: publication_topic_data.time_based_filter.as_ref(),
      presentation: publication_topic_data.presentation.as_ref(),
      lifespan: publication_topic_data.lifespan.as_ref(),
      history: None,
      resource_limits: None,
      content_filter_property: None,
    }
  }

  pub fn from_topic_data(topic_data: &'a TopicBuiltinTopicData) -> BuiltinDataSerializer<'a> {
    BuiltinDataSerializer {
      protocol_version: None,
      vendor_id: None,
      expects_inline_qos: None,
      participant_guid: None,
      metatraffic_unicast_locators: None,
      metatraffic_multicast_locators: None,
      default_unicast_locators: None,
      default_multicast_locators: None,
      available_builtin_endpoints: None,
      lease_duration: None,
      manual_liveliness_count: None,
      builtin_enpoint_qos: None,
      entity_name: None,
      endpoint_guid: topic_data.key.as_ref(),
      unicast_locator_list: None,
      multicast_locator_list: None,
      data_max_size_serialized: None,
      topic_name: topic_data.name.as_ref(),
      type_name: topic_data.type_name.as_ref(),
      durability: topic_data.durability.as_ref(),
      deadline: topic_data.deadline.as_ref(),
      latency_budget: topic_data.latency_budget.as_ref(),
      liveliness: topic_data.liveliness.as_ref(),
      reliability: topic_data.reliability.as_ref(),
      ownership: topic_data.ownership.as_ref(),
      destination_order: topic_data.destination_order.as_ref(),
      time_based_filter: None,
      presentation: topic_data.presentation.as_ref(),
      lifespan: topic_data.lifespan.as_ref(),
      history: topic_data.history.as_ref(),
      resource_limits: topic_data.resource_limits.as_ref(),
      content_filter_property: None,
    }
  }

  pub fn from_discovered_reader_data(
    discovered_reader_data: &'a DiscoveredReaderData,
  ) -> BuiltinDataSerializer<'a> {
    let bds_rp = BuiltinDataSerializer::from_reader_proxy(&discovered_reader_data.reader_proxy);
    let bds_std = BuiltinDataSerializer::from_subscription_topic_data(
      &discovered_reader_data.subscription_topic_data,
    );
    let mut bds_merged = bds_rp.merge(bds_std);
    bds_merged.content_filter_property = discovered_reader_data.content_filter.as_ref();
    bds_merged
  }

  pub fn from_discovered_writer_data(
    discovered_writer_data: &'a DiscoveredWriterData,
  ) -> BuiltinDataSerializer<'a> {
    let bds_wp = BuiltinDataSerializer::from_writer_proxy(&discovered_writer_data.writer_proxy);
    let bds_ptd = BuiltinDataSerializer::from_publication_topic_data(
      &discovered_writer_data.publication_topic_data,
    );
    let bds_merged = bds_wp.merge(bds_ptd);
    bds_merged
  }

  pub fn serialize<S: Serializer>(
    self,
    serializer: S,
    add_sentinel: bool,
  ) -> Result<S::Ok, S::Error> {
    let mut s = serializer
      .serialize_struct("SPDPParticipantData", self.fields_amount())
      .unwrap();

    self.add_protocol_version::<S>(&mut s);
    self.add_vendor_id::<S>(&mut s);
    self.add_expects_inline_qos::<S>(&mut s);
    self.add_participant_guid::<S>(&mut s);
    self.add_metatraffic_unicast_locators::<S>(&mut s);
    self.add_metatraffic_multicast_locators::<S>(&mut s);
    self.add_default_unicast_locators::<S>(&mut s);
    self.add_default_multicast_locators::<S>(&mut s);
    self.add_available_builtin_endpoint_set::<S>(&mut s);
    self.add_lease_duration::<S>(&mut s);
    self.add_manual_liveliness_count::<S>(&mut s);
    self.add_builtin_endpoint_qos::<S>(&mut s);
    self.add_entity_name::<S>(&mut s);

    self.add_endpoint_guid::<S>(&mut s);
    self.add_unicast_locator_list::<S>(&mut s);
    self.add_multicast_locator_list::<S>(&mut s);

    self.add_data_max_size_serialized::<S>(&mut s);

    self.add_topic_name::<S>(&mut s);
    self.add_type_name::<S>(&mut s);
    self.add_durability::<S>(&mut s);
    self.add_deadline::<S>(&mut s);
    self.add_latency_budget::<S>(&mut s);
    self.add_liveliness::<S>(&mut s);
    self.add_reliability::<S>(&mut s);
    self.add_ownership::<S>(&mut s);
    self.add_destination_order::<S>(&mut s);
    self.add_time_based_filter::<S>(&mut s);
    self.add_presentation::<S>(&mut s);
    self.add_lifespan::<S>(&mut s);
    self.add_history::<S>(&mut s);
    self.add_resource_limits::<S>(&mut s);

    self.add_content_filter_property::<S>(&mut s);

    if add_sentinel {
      s.serialize_field("sentinel", &(1 as u32)).unwrap();
    }

    s.end()
  }

  fn fields_amount(&self) -> usize {
    let mut count: usize = 0;

    let empty_ll = LocatorList::new();
    count = count + self.protocol_version.is_some() as usize;
    count = count + self.vendor_id.is_some() as usize;
    count = count + self.expects_inline_qos.is_some() as usize;
    count = count + self.participant_guid.is_some() as usize;
    count = count + self.metatraffic_unicast_locators.unwrap_or(&empty_ll).len();
    count = count
      + self
        .metatraffic_multicast_locators
        .unwrap_or(&empty_ll)
        .len();
    count = count + self.default_unicast_locators.unwrap_or(&empty_ll).len();
    count = count + self.default_multicast_locators.unwrap_or(&empty_ll).len();
    count = count + self.available_builtin_endpoints.is_some() as usize;
    count = count + self.lease_duration.is_some() as usize;
    count = count + self.manual_liveliness_count.is_some() as usize;
    count = count + self.builtin_enpoint_qos.is_some() as usize;
    count = count + self.entity_name.is_some() as usize;

    count = count + self.endpoint_guid.is_some() as usize;
    count = count + self.unicast_locator_list.unwrap_or(&empty_ll).len();
    count = count + self.multicast_locator_list.unwrap_or(&empty_ll).len();

    count = count + self.data_max_size_serialized.is_some() as usize;

    count = count + self.topic_name.is_some() as usize;
    count = count + self.type_name.is_some() as usize;
    count = count + self.durability.is_some() as usize;
    count = count + self.deadline.is_some() as usize;
    count = count + self.latency_budget.is_some() as usize;
    count = count + self.liveliness.is_some() as usize;
    count = count + self.reliability.is_some() as usize;
    count = count + self.ownership.is_some() as usize;
    count = count + self.destination_order.is_some() as usize;
    count = count + self.time_based_filter.is_some() as usize;
    count = count + self.presentation.is_some() as usize;
    count = count + self.lifespan.is_some() as usize;
    count = count + self.history.is_some() as usize;
    count = count + self.resource_limits.is_some() as usize;

    count = count + self.content_filter_property.is_some() as usize;

    count
  }

  fn add_protocol_version<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.protocol_version.as_ref() {
      Some(pv) => {
        s.serialize_field("protocol_version", &ProtocolVersionData::from(&pv))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_vendor_id<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.vendor_id.as_ref() {
      Some(vid) => {
        s.serialize_field("vendor_id", &VendorIdData::from(&vid))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_expects_inline_qos<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.expects_inline_qos.as_ref() {
      Some(iqos) => {
        let iq = ExpectsInlineQos {
          parameter_id: ParameterId::PID_EXPECTS_INLINE_QOS,
          parameter_length: 4,
          expects_inline_qos: *iqos.clone(),
        };
        s.serialize_field("expects_inline_qos", &iq).unwrap();
      }
      None => (),
    }
  }

  fn add_participant_guid<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.participant_guid.as_ref() {
      Some(guid) => {
        s.serialize_field(
          "participant_guid",
          &GUIDData::from(guid, &ParameterId::PID_PARTICIPANT_GUID),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_metatraffic_unicast_locators<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.metatraffic_unicast_locators {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "metatraffic_unicast_locators",
        &LocatorData::from(locator, ParameterId::PID_METATRAFFIC_UNICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_metatraffic_multicast_locators<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.metatraffic_multicast_locators {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "metatraffic_multicast_locators",
        &LocatorData::from(locator, ParameterId::PID_METATRAFFIC_MULTICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_default_unicast_locators<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.default_unicast_locators {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "default_unicast_locators",
        &LocatorData::from(locator, ParameterId::PID_DEFAULT_UNICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_default_multicast_locators<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.default_multicast_locators {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "default_multicast_locators",
        &LocatorData::from(locator, ParameterId::PID_DEFAULT_MULTICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_available_builtin_endpoint_set<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.available_builtin_endpoints.as_ref() {
      Some(eps) => {
        s.serialize_field(
          "available_builtin_endpoints",
          &BuiltinEndpointSetData::from(eps, &ParameterId::PID_BUILTIN_ENDPOINT_SET),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_lease_duration<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.lease_duration.as_ref() {
      Some(dur) => {
        s.serialize_field("lease_duration", &DurationData::from(dur))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_manual_liveliness_count<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.manual_liveliness_count.as_ref() {
      Some(liv) => {
        let mliv = ManualLivelinessCount {
          parameter_id: ParameterId::PID_PARTICIPANT_MANUAL_LIVELINESS_COUNT,
          parameter_length: 4,
          manual_liveliness_count: *liv.clone(),
        };
        s.serialize_field("manual_liveliness_count", &mliv).unwrap();
      }
      None => (),
    }
  }

  fn add_builtin_endpoint_qos<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.builtin_enpoint_qos.as_ref() {
      Some(qos) => {
        s.serialize_field("builtin_endpoint_qos", &BuiltinEndpointQosData::from(qos))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_entity_name<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.entity_name.as_ref() {
      Some(name) => {
        let ename = EntityName {
          parameter_id: ParameterId::PID_ENTITY_NAME,
          // adding 4 bytes for string lenght and 1 byte for terminate
          parameter_length: name.len() as u16 + 5,
          entity_name: name.to_string(),
        };
        s.serialize_field("entity_name", &ename).unwrap();
      }
      None => (),
    }
  }

  fn add_endpoint_guid<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.endpoint_guid.as_ref() {
      Some(guid) => {
        s.serialize_field(
          "endpoint_guid",
          &GUIDData::from(guid, &ParameterId::PID_ENDPOINT_GUID),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_unicast_locator_list<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.unicast_locator_list {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "default_unicast_locators",
        &LocatorData::from(locator, ParameterId::PID_UNICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_multicast_locator_list<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    let locators = match self.multicast_locator_list {
      Some(l) => l,
      None => return,
    };

    // TODO: make sure this works with multiple locators
    for locator in locators {
      s.serialize_field(
        "default_unicast_locators",
        &LocatorData::from(locator, ParameterId::PID_MULTICAST_LOCATOR),
      )
      .unwrap();
    }
  }

  fn add_topic_name<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.topic_name.as_ref() {
      Some(name) => {
        s.serialize_field(
          "topic_name",
          &StringData::new(ParameterId::PID_TOPIC_NAME, name),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_type_name<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.type_name.as_ref() {
      Some(name) => {
        s.serialize_field(
          "type_name",
          &StringData::new(ParameterId::PID_TYPE_NAME, name),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_durability<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.durability.as_ref() {
      Some(dur) => {
        s.serialize_field(
          "durability",
          &QosData::new(ParameterId::PID_DURABILITY, dur),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_deadline<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.deadline.as_ref() {
      Some(dl) => {
        s.serialize_field("deadline", &QosData::new(ParameterId::PID_DEADLINE, dl))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_latency_budget<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.latency_budget.as_ref() {
      Some(lb) => {
        s.serialize_field(
          "latency_budget",
          &QosData::new(ParameterId::PID_LATENCY_BUDGET, lb),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_liveliness<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.liveliness.as_ref() {
      Some(l) => {
        s.serialize_field("liveliness", &QosData::new(ParameterId::PID_LIVELINESS, l))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_reliability<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.reliability.as_ref() {
      Some(rel) => {
        s.serialize_field(
          "reliability",
          &QosData::new(ParameterId::PID_RELIABILITY, rel),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_ownership<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.ownership.as_ref() {
      Some(own) => {
        s.serialize_field("ownership", &QosData::new(ParameterId::PID_OWNERSHIP, own))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_destination_order<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.destination_order.as_ref() {
      Some(deor) => {
        s.serialize_field(
          "destination_order",
          &QosData::new(ParameterId::PID_DESTINATION_ORDER, deor),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_time_based_filter<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.time_based_filter.as_ref() {
      Some(tbf) => {
        s.serialize_field(
          "time_based_filter",
          &QosData::new(ParameterId::PID_TIME_BASED_FILTER, tbf),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_presentation<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.presentation.as_ref() {
      Some(p) => {
        s.serialize_field(
          "presentation",
          &QosData::new(ParameterId::PID_PRESENTATION, p),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_lifespan<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.lifespan.as_ref() {
      Some(ls) => {
        s.serialize_field("lifespan", &QosData::new(ParameterId::PID_LIFESPAN, ls))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_history<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.history.as_ref() {
      Some(hs) => {
        s.serialize_field("history", &QosData::new(ParameterId::PID_HISTORY, hs))
          .unwrap();
      }
      None => (),
    }
  }

  fn add_resource_limits<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.resource_limits.as_ref() {
      Some(rl) => {
        s.serialize_field(
          "resource_limits",
          &QosData::new(ParameterId::PID_RESOURCE_LIMITS, rl),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_content_filter_property<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.content_filter_property.as_ref() {
      Some(cfp) => {
        s.serialize_field(
          "content_filter_property",
          &ContentFilterPropertyData::new(cfp),
        )
        .unwrap();
      }
      None => (),
    }
  }

  fn add_data_max_size_serialized<S: Serializer>(&self, s: &mut S::SerializeStruct) {
    match self.data_max_size_serialized.as_ref() {
      Some(&&dmss) => {
        s.serialize_field(
          "data_max_size_serialized",
          &U32Data::new(ParameterId::PID_TYPE_MAX_SIZE_SERIALIZED, dmss),
        )
        .unwrap();
      }
      None => (),
    }
  }
}
