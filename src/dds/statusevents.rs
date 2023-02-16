//
// Describe the commnucation status changes as events.
//
// These implement a mechanism equivalent to what is described in
// Section 2.2.4 Listeners, Conditions, and Wait-sets
//
// Communcation statues are detailed in Figure 2.13 and tables in Section
// 2.2.4.1 in DDS Specification v1.4
#[allow(unused_imports)]
use log::{debug, error, info, trace, warn};

use std::io;

use mio_06::Evented;
use mio_extras::channel as mio_channel;

use mio_08;
use mio_08::{Registry, Token, Interest, event,};
use crate::mio_source::*;

use crate::dds::qos::QosPolicyId;

/// This trait corresponds to set_listener() of the Entity class in DDS spec.
/// Types implementing this trait can be registered to a poll and
/// polled for status events.
pub trait StatusEvented<E> {
  fn as_status_evented(&mut self) -> &dyn Evented; // This is for polling with mio-0.6.x
  fn as_status_source(&mut self) -> &mut dyn mio_08::event::Source; // This is for polling with mio-0.8.x

  fn try_recv_status(&self) -> Option<E>;
}

// Helper object for various DDS Entities
pub(crate) struct StatusReceiver<E> {
  channel_receiver: StatusChannelReceiver<E>,
  enabled: bool, // if not enabled, we should forward status to parent Entity
  // TODO: enabling not implemented
}

impl<E> StatusReceiver<E> {
  pub fn new(channel_receiver: StatusChannelReceiver<E>) -> Self {
    Self {
      channel_receiver,
      enabled: false,
    }
  }
}

impl<E> StatusEvented<E> for StatusReceiver<E> {
  fn as_status_evented(&mut self) -> &dyn Evented {
    self.enabled = true;
    &self.channel_receiver.actual_receiver
  }

  fn as_status_source(&mut self) -> &mut dyn mio_08::event::Source {
    self.enabled = true;
    &mut self.channel_receiver
  }

  fn try_recv_status(&self) -> Option<E> {
    if self.enabled {
      self.channel_receiver.try_recv().ok()
    } else {
      None
    }
  }
}

// a wrapper(s) for mio_channel augmented with
// poll channel, so that it can be used together with mio-0.8
// This is only used so that a mio-0.6 channel can pose as a
// mio-0.8 event::Source.

pub(crate) fn sync_status_channel<T>(capacity :usize) 
  -> io::Result<(StatusChannelSender<T>, StatusChannelReceiver<T>)>
{
  let (signal_receiver, signal_sender) = make_poll_channel()?;
  let (actual_sender, actual_receiver) = mio_channel::sync_channel(capacity);
  Ok((
    StatusChannelSender{ actual_sender, signal_sender },
    StatusChannelReceiver{ actual_receiver, signal_receiver }
  ))
}

pub(crate) struct StatusChannelSender<T> {
  actual_sender: mio_channel::SyncSender<T>,
  signal_sender: PollEventSender,
}

pub(crate) struct StatusChannelReceiver<T> {
  actual_receiver: mio_channel::Receiver<T>,
  signal_receiver: PollEventSource,
}

impl<T> StatusChannelSender<T> {
  pub fn try_send(&self, t: T) -> Result<(), mio_channel::TrySendError<T>> {
    match self.actual_sender.try_send(t) {
      Ok(()) => {
        self.signal_sender.send();
        Ok(())
      }, 
      Err(mio_channel::TrySendError::Full(tt)) => {
        trace!("StatusChannelSender cannot send new status changes, channel is full.");
        // It is perfectly normal to fail due to full channel, because
        // no-one is required to be listening to these.
        self.signal_sender.send(); // kick the receiver anyway
        Err(mio_channel::TrySendError::Full(tt))
      }
      Err(other_fail) => Err(other_fail),
    }
  }
}

impl<T> StatusChannelReceiver<T> {
  pub fn try_recv(&self) -> Result<T, std::sync::mpsc::TryRecvError> {
    self.signal_receiver.drain();
    self.actual_receiver.try_recv()
  }
}



impl<T> event::Source for StatusChannelReceiver<T>
{
  fn register(&mut self, registry: &Registry, token: Token, interests: Interest) -> io::Result<()>
  {
    self.signal_receiver.register(registry,token,interests)
  }

  fn reregister(&mut self, registry: &Registry, token: Token, interests: Interest) -> io::Result<()>
  {
    self.signal_receiver.reregister(registry, token, interests)
  }

  fn deregister(&mut self, registry: &Registry) -> io::Result<()>
  {
    self.signal_receiver.deregister(registry)
  }

}




#[derive(Debug, Clone)]
pub enum DomainParticipantStatus {
  PublisherStatus(PublisherStatus),
  SubscriberStatus(SubscriberStatus),
  TopicStatus(TopicStatus),
}

#[derive(Debug, Clone)]
pub enum SubscriberStatus {
  DataOnReaders,
  DataReaderStatus(DataReaderStatus),
}

pub type PublisherStatus = DataWriterStatus;

#[derive(Debug, Clone)]
pub enum TopicStatus {
  InconsistentTopic { count: CountWithChange },
}

#[derive(Debug, Clone)]
pub enum DataReaderStatus {
  /// Sample was rejected, because resource limits would have been exeeded.
  SampleRejected {
    count: CountWithChange,
    last_reason: SampleRejectedStatusKind,
    //last_instance_key:
  },
  /// Remote Writer has become active or inactive.
  LivelinessChanged {
    alive_total: CountWithChange,
    not_alive_total: CountWithChange,
    //last_publication_key:
  },
  /// Deadline requested by this DataReader was missed.
  RequestedDeadlineMissed {
    count: CountWithChange,
    //last_instance_key:
  },
  /// This DataReader has requested a QoS policy that is incompatibel with what
  /// is offered.
  RequestedIncompatibleQos {
    count: CountWithChange,
    last_policy_id: QosPolicyId,
    policies: Vec<QosPolicyCount>,
  },
  // DataAvailable is not implemented, as it seems to bring little additional value,
  // because the normal data waiting mechanism already uses the same mio::poll structure,
  // so repeating the functionality here would bring little additional value.
  /// A sample has been lost (never received).
  /// (Whtever this means?)
  SampleLost { count: CountWithChange },

  /// The DataReader has found a DataWriter that matches the Topic and has
  /// compatible QoS, or has ceased to be matched with a DataWriter that was
  /// previously considered to be matched.
  SubscriptionMatched {
    total: CountWithChange,
    current: CountWithChange,
    //last_publication_key:
  },
}

#[derive(Debug, Clone)]
pub enum DataWriterStatus {
  LivelinessLost {
    count: CountWithChange,
  },
  OfferedDeadlineMissed {
    count: CountWithChange,
    //last_instance_key:
  },
  OfferedIncompatibleQos {
    count: CountWithChange,
    last_policy_id: QosPolicyId,
    policies: Vec<QosPolicyCount>,
  },
  PublicationMatched {
    total: CountWithChange,
    current: CountWithChange,
    //last_subscription_key:
  },
}

/// Helper to contain same count actions across statuses
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub struct CountWithChange {
  // 2.3. Platform Specific Model defines these as "long", which appears to be 32-bit signed.
  count: i32,
  count_change: i32,
}

impl CountWithChange {
  pub(crate) fn new(count: i32, count_change: i32) -> Self {
    Self {
      count,
      count_change,
    }
  }

  // ??
  // same as "new" ?
  pub fn start_from(count: i32, count_change: i32) -> Self {
    Self {
      count,
      count_change,
    }
  }

  pub fn count(&self) -> i32 {
    self.count
  }

  pub fn count_change(&self) -> i32 {
    self.count_change
  }

  // does this make sense?
  // pub fn increase(&mut self) {
  //   self.count += 1;
  //   self.count_change += 1;
  // }
}

// sample rejection reasons
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SampleRejectedStatusKind {
  NotRejected,
  ByInstancesLimit,
  BySamplesLimit,
  BySamplesPerInstanceLimit,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct QosPolicyCount {
  policy_id: QosPolicyId,
  count: i32,
}
