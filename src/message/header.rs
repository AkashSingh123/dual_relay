use common::protocol_id;
use common::protocol_version;
use common::vendor_id;
use common::guid_prefix;
use common::locator;
use common::time;

enum SubmessageKind {
    PAD = 0x01,
    ACKNACK = 0x06,
    HEARTBEAT = 0x07,
    GAP = 0x08,
    INFO_TS = 0x09,
    INFO_SRC = 0x0c,
    INFO_REPLAY_IP4 = 0x0d,
    INFO_DST = 0x0e,
    INFO_REPLAY = 0x0f,
    NACK_FRAG = 0x12,
    HEARTBEAT_FRAG = 0x13,
    DATA = 0x15,
    DATA_FRAG = 0x16
}

struct Header {
    protocol_id: protocol_id::ProtocolId_t,
    protocol_version: protocol_version::ProtocolVersion_t,
    vendor_id: vendor_id::VendorId_t,
    guid_prefix: guid_prefix::GuidPrefix_t,
}

impl Header {
    fn new(guid: guid_prefix::GuidPrefix_t) -> Header {
        Header {
            protocol_id: protocol_id::ProtocolId_t::PROTOCOL_RTPS,
            protocol_version: protocol_version::PROTOCOLVERSION,
            vendor_id: vendor_id::VENDOR_UNKNOWN,
            guid_prefix: guid
        }
    }
}

struct SubmessageHeader {
    submessage_id: SubmessageKind,
    // flags: SubmessageFlag, // TODO: finish type
    submessage_length: usize
}

struct Receiver {
    source_version: protocol_version::ProtocolVersion_t,
    source_vendor_id: vendor_id::VendorId_t,
    source_guid_prefix: guid_prefix::GuidPrefix_t,
    dest_guid_pregix: guid_prefix::GuidPrefix_t,
    unicast_reply_locator_list: locator::Locator_t,
    multicast_reply_locator_list: locator::Locator_t,
    have_timestamp: bool,
    timestapm: time::Time_t
}

impl Receiver {
    fn new(destination_guid_prefix: guid_prefix::GuidPrefix_t,
           unicast_reply_locator: locator::Locator_t,
           multicast_reply_locator: locator::Locator_t) -> Receiver {
        Receiver {
            source_version: protocol_version::PROTOCOLVERSION,
            source_vendor_id: vendor_id::VENDOR_UNKNOWN,
            source_guid_prefix: guid_prefix::GUIDPREFIX_UNKNOWN,
            dest_guid_pregix: destination_guid_prefix,
            unicast_reply_locator_list: locator::Locator_t {
                kind: unicast_reply_locator.kind,
                port: locator::LOCATOR_PORT_INVALID, // TODO: check if it is correct, page 35
                address: unicast_reply_locator.address
            },
            multicast_reply_locator_list: locator::Locator_t {
                kind: multicast_reply_locator.kind,
                port: locator::LOCATOR_PORT_INVALID, // TODO: check if it is correct, page 35
                address: multicast_reply_locator.address
            },
            have_timestamp: false,
            timestapm: time::TIME_INVALID
        }
    }
}
