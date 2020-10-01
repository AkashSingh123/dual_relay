use atosdds::{
  dds::qos::{
    QosPolicies, policy::Deadline, policy::DestinationOrder, policy::Durability, policy::History,
    policy::LatencyBudget, policy::Lifespan, policy::Liveliness, policy::LivelinessKind,
    policy::Ownership, policy::Reliability,
  },
  dds::traits::key::Key,
  dds::traits::key::Keyed,
  structure::duration::Duration,
  structure::guid::GUID,
  structure::time::Timestamp,
};

use serde::{Serialize, Deserialize};

use std::time::Duration as StdDuration;

pub struct ROSDiscoveryTopic {}

impl ROSDiscoveryTopic {
  const QOS: QosPolicies = QosPolicies {
    durability: Some(Durability::TransientLocal),
    presentation: None,
    deadline: Some(Deadline {
      period: Duration::DURATION_INFINITE,
    }),
    latency_budget: Some(LatencyBudget {
      duration: Duration::DURATION_ZERO,
    }),
    ownership: Some(Ownership::Shared),
    liveliness: Some(Liveliness {
      kind: LivelinessKind::Automatic,
      lease_duration: Duration::DURATION_INFINITE,
    }),
    time_based_filter: None,
    reliability: Some(Reliability::Reliable {
      max_blocking_time: Duration::DURATION_ZERO,
    }),
    destination_order: Some(DestinationOrder::ByReceptionTimestamp),
    history: Some(History::KeepLast { depth: 1 }),
    resource_limits: None,
    lifespan: Some(Lifespan {
      duration: Duration::DURATION_INFINITE,
    }),
  };

  pub fn topic_name() -> String {
    String::from("ros_discovery_info")
  }

  pub fn type_name() -> String {
    String::from("rmw_dds_common::msg::dds_::ParticipantEntitiesInfo_")
  }

  pub fn get_qos() -> QosPolicies {
    ROSDiscoveryTopic::QOS
  }
}

pub struct ParameterEventsTopic {}

impl ParameterEventsTopic {
  const QOS: QosPolicies = QosPolicies {
    durability: Some(Durability::TransientLocal),
    presentation: None,
    deadline: None,
    latency_budget: None,
    ownership: None,
    liveliness: None,
    time_based_filter: None,
    reliability: Some(Reliability::Reliable {
      max_blocking_time: Duration::DURATION_ZERO,
    }),
    destination_order: None,
    history: Some(History::KeepLast { depth: 1 }),
    resource_limits: None,
    lifespan: None,
  };

  pub fn topic_name() -> String {
    String::from("rt/parameter_events")
  }

  pub fn type_name() -> String {
    String::from("rcl_interfaces::msg::dds_::ParameterEvent_")
  }

  pub fn get_qos() -> QosPolicies {
    ParameterEventsTopic::QOS
  }
}

pub struct RosOutTopic {}

impl RosOutTopic {
  const QOS: QosPolicies = QosPolicies {
    durability: Some(Durability::TransientLocal),
    presentation: None,
    deadline: Some(Deadline {
      period: Duration::DURATION_INFINITE,
    }),
    latency_budget: Some(LatencyBudget {
      duration: Duration::DURATION_ZERO,
    }),
    ownership: Some(Ownership::Shared),
    liveliness: Some(Liveliness {
      kind: LivelinessKind::Automatic,
      lease_duration: Duration::DURATION_INFINITE,
    }),
    time_based_filter: None,
    reliability: Some(Reliability::Reliable {
      max_blocking_time: Duration::DURATION_ZERO,
    }),
    destination_order: Some(DestinationOrder::ByReceptionTimestamp),
    history: Some(History::KeepLast { depth: 1 }),
    resource_limits: None,
    lifespan: Some(Lifespan {
      duration: Duration::from_std(StdDuration::from_secs(10)),
    }),
  };

  pub fn topic_name() -> String {
    String::from("rt/rosout")
  }

  pub fn type_name() -> String {
    String::from("rcl_interfaces::msg::dds_::Log_")
  }

  pub fn get_qos() -> QosPolicies {
    RosOutTopic::QOS
  }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash, PartialOrd, Ord, Serialize, Deserialize)]
pub struct Gid {
  data: [u8; 24],
}

impl Gid {
  pub fn from_guid(guid: GUID) -> Gid {
    let mut data: [u8; 24] = [0; 24];
    data[..12].clone_from_slice(&guid.guidPrefix.entityKey);
    data[12..15].clone_from_slice(&guid.entityId.entityKey);
    data[15..16].clone_from_slice(&[guid.entityId.entityKind]);
    Gid { data }
  }
}

impl Key for Gid {}

#[derive(Debug, Clone, PartialEq, Eq, Hash, Serialize, Deserialize)]
pub struct NodeInfo {
  pub node_namespace: String,
  pub node_name: String,
  pub reader_guid: Vec<Gid>,
  pub writer_guid: Vec<Gid>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ROSParticipantInfo {
  guid: Gid,
  nodes: Vec<NodeInfo>,
}

impl ROSParticipantInfo {
  pub fn new(guid: Gid, nodes: Vec<NodeInfo>) -> ROSParticipantInfo {
    ROSParticipantInfo { guid, nodes }
  }

  pub fn guid(&self) -> Gid {
    self.guid
  }

  pub fn nodes(&self) -> &Vec<NodeInfo> {
    &self.nodes
  }
}

impl Keyed for ROSParticipantInfo {
  type K = Gid;

  fn get_key(&self) -> Self::K {
    self.guid
  }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ParameterEvents {
  timestamp: Timestamp,
  // fully qualified path
  node: String,
  new_parameters: Vec<Parameter>,
  changed_parameters: Vec<Parameter>,
  deleted_parameters: Vec<Parameter>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Parameter {
  name: String,
  value: ParameterValue,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct ParameterValue {
  ptype: u8,
  boolean_value: bool,
  int_value: i64,
  double_value: f64,
  string_value: String,
  byte_array: Vec<u8>,
  bool_array: Vec<bool>,
  int_array: Vec<i64>,
  double_array: Vec<f64>,
  string_array: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Log {
  timestamp: Timestamp,
  level: u8,
  name: String,
  msg: String,
  file: String,
  function: String,
  line: u32,
}

#[cfg(test)]
mod tests {
  use std::{fs::File, io::Read};

  use atosdds::{
    dds::traits::serde_adapters::DeserializerAdapter,
    serialization::cdrDeserializer::CDR_deserializer_adapter,
    serialization::cdrSerializer::to_bytes, submessages::RepresentationIdentifier,
  };
  use byteorder::LittleEndian;

  use super::*;

  #[test]
  fn deserialize_ros_participant() {
    const DATA: [u8; 1044] = [
      // Offset 0x00000000 to 0x00001043
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0xc1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00,
      0x00, 0x00, 0x2f, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x74, 0x75, 0x72, 0x74, 0x6c,
      0x65, 0x73, 0x69, 0x6d, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d,
      0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x06, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x04,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x0c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d,
      0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x04, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x11, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x04,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x16, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d,
      0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x04, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x1a, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x04,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d,
      0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x22, 0x04, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x04,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x09, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d,
      0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0b, 0x03, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x13, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d,
      0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x03, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x19,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x1d, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d,
      0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x03, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x27, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d,
      0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x29, 0x03, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x0f, 0x2d, 0x7d, 0xe8, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2b,
      0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];

    let value = CDR_deserializer_adapter::<ROSParticipantInfo>::from_bytes(
      &DATA,
      RepresentationIdentifier::CDR_LE,
    )
    .unwrap();

    println!("{:x?}", value);

    let data2 = to_bytes::<ROSParticipantInfo, LittleEndian>(&value).unwrap();

    println!("\n{:x?}\n", DATA.to_vec());
    println!("{:x?}", data2);
  }

  #[test]
  fn final_node_test() {
    let mut f = File::open("error_bin.bin").unwrap();
    let mut buffer: [u8; 1024] = [0; 1024];
    let len = f.read(&mut buffer).unwrap();

    println!("Buffer: size: {}\n{:?}", len, buffer.to_vec());
    let rpi = CDR_deserializer_adapter::<ROSParticipantInfo>::from_bytes(
      &buffer,
      RepresentationIdentifier::CDR_LE,
    )
    .unwrap();
    println!("RosParticipantInfo: \n{:?}", rpi);
    let data2 = to_bytes::<ROSParticipantInfo, LittleEndian>(&rpi).unwrap();
    println!("Data2: \n{:?}", data2);
  }
}
