use speedy::{Readable, Writable};

#[derive(Debug, PartialOrd, PartialEq, Ord, Eq, Readable, Writable)]
pub struct ProtocolVersion {
  pub major: u8,
  pub minor: u8,
}

impl ProtocolVersion {
  pub const PROTOCOLVERSION: ProtocolVersion = ProtocolVersion::PROTOCOLVERSION_2_4;

  pub const PROTOCOLVERSION_1_0: ProtocolVersion = ProtocolVersion { major: 1, minor: 0 };
  pub const PROTOCOLVERSION_1_1: ProtocolVersion = ProtocolVersion { major: 1, minor: 1 };
  pub const PROTOCOLVERSION_2_0: ProtocolVersion = ProtocolVersion { major: 2, minor: 0 };
  pub const PROTOCOLVERSION_2_1: ProtocolVersion = ProtocolVersion { major: 2, minor: 1 };
  pub const PROTOCOLVERSION_2_2: ProtocolVersion = ProtocolVersion { major: 2, minor: 2 };
  pub const PROTOCOLVERSION_2_4: ProtocolVersion = ProtocolVersion { major: 2, minor: 4 };
}

impl Default for ProtocolVersion {
  fn default() -> Self {
    ProtocolVersion::PROTOCOLVERSION
  }
}

#[cfg(test)]
mod tests {
  use super::*;

  serialization_test!( type = ProtocolVersion,
  {
      protocol_version,
      ProtocolVersion::PROTOCOLVERSION,
      le = [0x02, 0x04],
      be = [0x02, 0x04]
  },
  {
      protocol_version_default,
      ProtocolVersion::default(),
      le = [0x02, 0x04],
      be = [0x02, 0x04]
  },
  {
      protocol_version_1_0,
      ProtocolVersion::PROTOCOLVERSION_1_0,
      le = [0x01, 0x00],
      be = [0x01, 0x00]
  },
  {
      protocol_version_1_1,
      ProtocolVersion::PROTOCOLVERSION_1_1,
      le = [0x01, 0x01],
      be = [0x01, 0x01]
  },
  {
      protocol_version_2_0,
      ProtocolVersion::PROTOCOLVERSION_2_0,
      le = [0x02, 0x00],
      be = [0x02, 0x00]
  },
  {
      protocol_version_2_1,
      ProtocolVersion::PROTOCOLVERSION_2_1,
      le = [0x02, 0x01],
      be = [0x02, 0x01]
  },
  {
      protocol_version_2_2,
      ProtocolVersion::PROTOCOLVERSION_2_2,
      le = [0x02, 0x02],
      be = [0x02, 0x02]
  },
  {
      protocol_version_2_4,
      ProtocolVersion::PROTOCOLVERSION_2_4,
      le = [0x02, 0x04],
      be = [0x02, 0x04]
  });
}
