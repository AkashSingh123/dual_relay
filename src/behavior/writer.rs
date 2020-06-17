use crate::structure::cache_change::CacheChange;
use crate::structure::change_kind::ChangeKind_t;
use crate::structure::data::Data;
use crate::structure::duration::Duration_t;
use crate::structure::instance_handle::InstanceHandle_t;
use crate::structure::sequence_number::SequenceNumber_t;

pub struct WriterAttributes {
  pub push_mode: bool,
  pub heartbeat_period: Duration_t,
  pub nack_response_delay: Duration_t,
  pub nack_suppression_duration: Duration_t,
  pub last_change_sequence_number: SequenceNumber_t,
}

pub trait Writer {
  fn as_writer(&self) -> &WriterAttributes;
  fn new_change(&mut self, kind: ChangeKind_t, data: Data, handle: InstanceHandle_t)
    -> CacheChange;
}
