use mio::{Poll, Event, Events, Token, Ready, PollOpt};
use mio_extras::channel as mio_channel;
extern crate chrono;
//use chrono::Duration;
use std::collections::HashMap;
use std::{
  sync::{Arc, RwLock},
};

use crate::dds::message_receiver::MessageReceiver;
use crate::dds::reader::Reader;
use crate::dds::writer::Writer;
use crate::network::udp_listener::UDPListener;
use crate::network::constant::*;
use crate::structure::guid::{GuidPrefix, GUID, EntityId};
use crate::structure::entity::Entity;
use crate::{
  common::timed_event_handler::{TimedEventHandler},
  discovery::discovery_db::DiscoveryDB,
  structure::{cache_change::ChangeKind, dds_cache::DDSCache, topic_kind::TopicKind},
  submessages::AckNack,
};
use super::{typedesc::TypeDesc, rtps_reader_proxy::RtpsReaderProxy};

pub struct DPEventWrapper {
  domain_participants_guid: GUID,
  poll: Poll,
  ddscache: Arc<RwLock<DDSCache>>,
  discovery_db: Arc<RwLock<DiscoveryDB>>,
  udp_listeners: HashMap<Token, UDPListener>,
  send_targets: HashMap<Token, mio_channel::Sender<Vec<u8>>>,
  message_receiver: MessageReceiver,

  // Adding readers
  add_reader_receiver: TokenReceiverPair<Reader>,
  remove_reader_receiver: TokenReceiverPair<GUID>,

  // Writers
  add_writer_receiver: TokenReceiverPair<Writer>,
  remove_writer_receiver: TokenReceiverPair<GUID>,
  writer_timed_event_reciever: HashMap<Token, mio_channel::Receiver<TimerMessageType>>,

  stop_poll_receiver: mio_channel::Receiver<()>,
  // GuidPrefix sent in this channel needs to be RTPSMessage source_guid_prefix. Writer needs this to locate RTPSReaderProxy if negative acknack.
  ack_nack_reciever: mio_channel::Receiver<(GuidPrefix, AckNack)>,

  writers: HashMap<GUID, Writer>,

  discovery_update_notification_receiver: mio_channel::Receiver<DiscoveryNotificationType>,
}

impl DPEventWrapper {
  // This pub(crate) , because it should be constructed only by DomainParticipant.
  pub(crate) fn new(
    domain_participants_guid: GUID,
    udp_listeners: HashMap<Token, UDPListener>,
    ddscache: Arc<RwLock<DDSCache>>,
    discovery_db: Arc<RwLock<DiscoveryDB>>,
    send_targets: HashMap<Token, mio_channel::Sender<Vec<u8>>>,
    participant_guid_prefix: GuidPrefix,
    add_reader_receiver: TokenReceiverPair<Reader>,
    remove_reader_receiver: TokenReceiverPair<GUID>,
    add_writer_receiver: TokenReceiverPair<Writer>,
    remove_writer_receiver: TokenReceiverPair<GUID>,
    stop_poll_receiver: mio_channel::Receiver<()>,
    discovery_update_notification_receiver: mio_channel::Receiver<DiscoveryNotificationType>,
  ) -> DPEventWrapper {
    let poll = Poll::new().expect("Unable to create new poll.");
    let (acknack_sender, acknack_reciever) = mio_channel::channel::<(GuidPrefix, AckNack)>();
    let mut udp_listeners = udp_listeners;
    for (token, listener) in &mut udp_listeners {
      poll
        .register(
          listener.mio_socket(),
          token.clone(),
          Ready::readable(),
          PollOpt::edge(),
        )
        .expect("Failed to register listener.");
    }

    poll
      .register(
        &add_reader_receiver.receiver,
        add_reader_receiver.token.clone(),
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register reader adder.");

    poll
      .register(
        &remove_reader_receiver.receiver,
        remove_reader_receiver.token.clone(),
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register reader remover.");
    poll
      .register(
        &add_writer_receiver.receiver,
        add_writer_receiver.token,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register add writer channel");

    poll
      .register(
        &remove_writer_receiver.receiver,
        remove_writer_receiver.token,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register remove writer channel");

    poll
      .register(
        &stop_poll_receiver,
        STOP_POLL_TOKEN,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register stop poll channel");

    poll
      .register(
        &acknack_reciever,
        ACKNACK_MESSGAGE_TO_LOCAL_WRITER_TOKEN,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register AckNack submessage sending from MessageReciever to DPEventLoop");

    poll
      .register(
        &discovery_update_notification_receiver,
        DISCOVERY_UPDATE_NOTIFICATION_TOKEN,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register reader update notification.");

    DPEventWrapper {
      domain_participants_guid,
      poll,
      ddscache,
      discovery_db,
      udp_listeners,
      send_targets,
      message_receiver: MessageReceiver::new(participant_guid_prefix, acknack_sender),
      add_reader_receiver,
      remove_reader_receiver,
      add_writer_receiver,
      remove_writer_receiver,
      writer_timed_event_reciever: HashMap::new(),
      stop_poll_receiver,
      writers: HashMap::new(),
      ack_nack_reciever: acknack_reciever,
      discovery_update_notification_receiver,
    }
  }

  pub fn event_loop(self) {
    // TODO: Use the dp to access stuff we need, e.g. historycache
    let mut ev_wrapper = self;
    loop {
      let mut events = Events::with_capacity(1024);
      ev_wrapper
        .poll
        .poll(&mut events, None)
        .expect("Failed in waiting of poll.");
      for event in events.into_iter() {
        if event.token() == STOP_POLL_TOKEN {
          return;
        } else if DPEventWrapper::is_udp_traffic(&event) {
          ev_wrapper.handle_udp_traffic(&event);
        } else if DPEventWrapper::is_reader_action(&event) {
          ev_wrapper.handle_reader_action(&event);
        } else if DPEventWrapper::is_writer_action(&event) {
          ev_wrapper.handle_writer_action(&event);
        } else if ev_wrapper.is_writer_timed_event_action(&event) {
          ev_wrapper.handle_writer_timed_event(&event);
        } else if DPEventWrapper::is_writer_acknack_action(&event) {
          ev_wrapper.handle_writer_acknack_action(&event);
        } else if DPEventWrapper::is_discovery_update_notification(&event) {
          let notification_type = match ev_wrapper.discovery_update_notification_receiver.try_recv()
          {
            Ok(nt) => nt,
            _ => continue,
          };

          match notification_type {
            DiscoveryNotificationType::ReadersInfoUpdated => ev_wrapper.update_readers(),
            DiscoveryNotificationType::WritersInfoUpdated => ev_wrapper.update_writers(),
            DiscoveryNotificationType::TopicsInfoUpdated => ev_wrapper.update_topics(),
          }
        }
      }
    }
  }

  pub fn is_udp_traffic(event: &Event) -> bool {
    event.token() == DISCOVERY_LISTENER_TOKEN
      || event.token() == DISCOVERY_MUL_LISTENER_TOKEN
      || event.token() == USER_TRAFFIC_LISTENER_TOKEN
      || event.token() == USER_TRAFFIC_MUL_LISTENER_TOKEN
  }

  pub fn is_reader_action(event: &Event) -> bool {
    event.token() == ADD_READER_TOKEN || event.token() == REMOVE_READER_TOKEN
  }

  /// Writer action can be add writer remove writer or some not predefined token.
  /// if not predefined token -> EntityIdToken can be calculated and if entityKind is 0xC2 then it is writer action.
  pub fn is_writer_action(event: &Event) -> bool {
    if EntityId::from_usize(event.token().0).is_some() {
      let maybeWriterKind: EntityId = EntityId::from_usize(event.token().0).unwrap();
      if maybeWriterKind.get_kind() == 0xC2 {
        return true;
      }
    }
    event.token() == ADD_WRITER_TOKEN || event.token() == REMOVE_WRITER_TOKEN
  }

  /// Writer timed events can be Heartbeats or cache cleaning actions.
  pub fn is_writer_timed_event_action(&self, event: &Event) -> bool {
    if self
      .writer_timed_event_reciever
      .contains_key(&event.token())
    {
      return true;
    }
    return false;
  }

  pub fn is_writer_acknack_action(event: &Event) -> bool {
    event.token() == ACKNACK_MESSGAGE_TO_LOCAL_WRITER_TOKEN
  }

  pub fn is_discovery_update_notification(event: &Event) -> bool {
    event.token() == DISCOVERY_UPDATE_NOTIFICATION_TOKEN
  }

  pub fn handle_udp_traffic(&mut self, event: &Event) {
    let listener = self.udp_listeners.get(&event.token());
    match listener {
      Some(l) => loop {
        let data = l.get_message();
        if data.is_empty() {
          //println!("UDP data is empty!");
          return;
        }

        if event.token() == DISCOVERY_LISTENER_TOKEN {
          self.message_receiver.handle_discovery_msg(data);
        } else if event.token() == USER_TRAFFIC_LISTENER_TOKEN {
          self.message_receiver.handle_user_msg(data);
        }
      },
      None => {
        print!(
          "Cannot handle upd traffic! No listener with token {:?}",
          &event.token()
        );
        return;
      }
    };
  }

  pub fn handle_reader_action(&mut self, event: &Event) {
    match event.token() {
      ADD_READER_TOKEN => {
        let new_reader = self
          .add_reader_receiver
          .receiver
          .try_recv()
          .expect("Can't receive new reader");
        self.message_receiver.add_reader(new_reader);
      }
      REMOVE_READER_TOKEN => {
        let old_reader_guid = self.remove_reader_receiver.receiver.try_recv().unwrap();
        self.message_receiver.remove_reader(old_reader_guid);
      }
      _ => {}
    }
  }

  pub fn handle_writer_action(&mut self, event: &Event) {
    match event.token() {
      ADD_WRITER_TOKEN => {
        let mut new_writer = self
          .add_writer_receiver
          .receiver
          .try_recv()
          .expect("Failed to receive new add writer message.");
        &self.poll.register(
          new_writer.cache_change_receiver(),
          new_writer.get_entity_token(),
          Ready::readable(),
          PollOpt::edge(),
        );
        let (timed_action_sender, timed_action_receiver) =
          mio_channel::channel::<TimerMessageType>();
        let time_handler: TimedEventHandler = TimedEventHandler::new(timed_action_sender.clone());
        new_writer.add_timed_event_handler(time_handler);

        self
          .poll
          .register(
            &timed_action_receiver,
            new_writer.get_timed_event_entity_token(),
            Ready::readable(),
            PollOpt::edge(),
          )
          .expect("Writer heartbeat timer channel registeration failed!!");
        self.writer_timed_event_reciever.insert(
          new_writer.get_timed_event_entity_token(),
          timed_action_receiver,
        );
        self.writers.insert(new_writer.as_entity().guid, new_writer);
      }
      REMOVE_WRITER_TOKEN => {
        let writer = self.writers.remove(
          &self
            .remove_writer_receiver
            .receiver
            .try_recv()
            .expect("Failed to get writer reciver guid"),
        );
        if let Some(w) = writer {
          &self.poll.deregister(w.cache_change_receiver());
        };
      }
      t => {
        let found_writer = self
          .writers
          .iter_mut()
          .find(|p| p.1.get_entity_token() == t);

        match found_writer {
          Some((_guid, w)) => {
            let cache_change = w.cache_change_receiver().try_recv();

            match cache_change {
              Ok(cc) => {
                if cc.change_kind == ChangeKind::NOT_ALIVE_DISPOSED {
                  w.handle_not_alive_disposed_cache_change(cc);
                } else if cc.change_kind == ChangeKind::ALIVE {
                  w.insert_to_history_cache(cc);
                  w.send_all_unsend_messages();
                }
              }
              _ => (),
            }
          }
          None => {}
        }
      }
    }
  }

  /// Writer timed events can be heatrbeats or cache cleaning events.
  /// events are distinguished by TimerMessageType which is send via mio channel. Channel token in
  pub fn handle_writer_timed_event(&mut self, event: &Event) {
    let reciever = self
      .writer_timed_event_reciever
      .get(&event.token())
      .expect("Did not found heartbeat reciever ");
    let mut message_queue: Vec<TimerMessageType> = vec![];
    loop {
      let res = reciever.try_recv();
      if res.is_ok() {
        message_queue.push(res.unwrap());
        break;
      }
      if res.is_err() {
        panic!("Writer timed event message error! {:?}", res)
      }
    }

    for timer_message in message_queue {
      if timer_message == TimerMessageType::writer_heartbeat {
        let found_writer_with_heartbeat = self
          .writers
          .iter_mut()
          .find(|p| p.1.get_timed_event_entity_token() == event.token());
        match found_writer_with_heartbeat {
          Some((_guid, w)) => {
            w.handle_heartbeat_tick();
          }
          None => {}
        }
      } else if timer_message == TimerMessageType::writer_cache_cleaning {
        let found_writer_to_clean_some_cache = self
          .writers
          .iter_mut()
          .find(|p| p.1.get_timed_event_entity_token() == event.token());
        match found_writer_to_clean_some_cache {
          Some((_guid, w)) => {
            w.handle_cache_cleaning();
          }
          None => {}
        }
      }
    }
  }

  pub fn handle_writer_acknack_action(&mut self, _event: &Event) {
    println!("is writer acknack action!");
    let recieved = self.ack_nack_reciever.try_recv();

    if recieved.is_ok() {
      let (acknack_sender_prefix, acknack_message) = recieved.unwrap();
      let target_writer_entity_id = { acknack_message.writer_id };
      let writer_guid = GUID::new_with_prefix_and_id(
        self.domain_participants_guid.guidPrefix,
        target_writer_entity_id,
      );
      let found_writer = self.writers.get_mut(&writer_guid);
      if found_writer.is_some() {
        found_writer
          .unwrap()
          .handle_ack_nack(&acknack_sender_prefix, acknack_message)
      } else {
        panic!(
          "Couldn't handle acknack! did not find local rtps writer with GUID: {:?}",
          writer_guid
        );
      }
    }
  }

  pub fn update_writers(&mut self) {
    match self.discovery_db.read() {
      Ok(db) => {
        for (_, writer) in self.writers.iter_mut() {
          if *writer.get_entity_id() == EntityId::ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER {
            // if writer is participant writer
            writer.readers = db
              .get_participants()
              .map(|p| p.as_reader_proxy(true).clone())
              .collect();
          } else if *writer.get_entity_id() == EntityId::ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER
          {
            writer.readers = db
              .get_participants()
              .map(|p| p.as_reader_proxy(true))
              .map(|mut p| {
                p.remote_reader_guid.entityId =
                  EntityId::ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER;
                p
              })
              .collect();
          } else if *writer.get_entity_id() == EntityId::ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER {
            writer.readers = db
              .get_participants()
              .map(|p| p.as_reader_proxy(true))
              .map(|mut p| {
                p.remote_reader_guid.entityId = EntityId::ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER;
                p
              })
              .collect();
          } else {
            let proxies = db.get_writers_reader_proxies(writer.get_guid());
            match proxies {
              Some(v) => {
                writer.readers = v
                  .iter()
                  .map(|&p| RtpsReaderProxy::from_discovered_reader_data(p).unwrap())
                  .collect();
              }
              None => (),
            };
          }
        }
      }
      _ => panic!("DiscoveryDB is poisoned."),
    }
  }

  pub fn update_readers(&mut self) {
    match self.discovery_db.read() {
      Ok(db) => {
        for reader in self.message_receiver.available_readers.iter_mut() {
          let proxies = db.get_readers_writer_proxies(reader.get_guid());
          match proxies {
            Some(v) => {
              reader.clear_matched_writers();
              v.iter().for_each(|p| {
                let guid = match p.writer_proxy.remote_writer_guid {
                  Some(g) => g.clone(),
                  None => return,
                };
                // TODO: get actual group entityId
                reader.matched_writer_add(
                  guid,
                  EntityId::ENTITYID_UNKNOWN,
                  p.writer_proxy.unicast_locator_list.clone(),
                  p.writer_proxy.multicast_locator_list.clone(),
                );
              })
            }
            None => (),
          }
        }
      }
      _ => panic!("DiscoveryDB is poisoned"),
    }
  }

  pub fn update_topics(&mut self) {
    match self.discovery_db.read() {
      Ok(db) => match self.ddscache.write() {
        Ok(mut ddsc) => {
          for topic in db.get_all_topics() {
            let topic_name = match &topic.topic_data.name {
              Some(td) => td,
              None => continue,
            };
            // TODO: how do you know when topic is keyed and is not
            let topic_kind = match &topic.topic_data.key {
              Some(_) => TopicKind::WITH_KEY,
              None => TopicKind::NO_KEY,
            };
            let topic_data_type = match &topic.topic_data.type_name {
              Some(tn) => tn.clone(),
              None => continue,
            };
            ddsc.add_new_topic(topic_name, topic_kind, &TypeDesc::new(topic_data_type));
          }
        }
        _ => panic!("DDSCache is poisoned"),
      },
      _ => panic!("DiscoveryDB is poisoned"),
    }
  }
}

#[cfg(test)]
mod tests {
  use super::*;
  use std::thread;
  use std::time::Duration;
  use mio::{Ready, PollOpt};
  use crate::structure::entity::Entity;
  use crate::structure::dds_cache::DDSCache;

  #[test]
  fn dpew_add_and_remove_readers() {
    // Adding readers
    let (sender_add_reader, receiver_add) = mio_channel::channel::<Reader>();
    let (sender_remove_reader, receiver_remove) = mio_channel::channel::<GUID>();

    let (_add_writer_sender, add_writer_receiver) = mio_channel::channel();
    let (_remove_writer_sender, remove_writer_receiver) = mio_channel::channel();

    let (_stop_poll_sender, stop_poll_receiver) = mio_channel::channel();

    let (_discovery_update_notification_sender, discovery_update_notification_receiver) =
      mio_channel::channel();

    let ddshc = Arc::new(RwLock::new(DDSCache::new()));
    let discovery_db = Arc::new(RwLock::new(DiscoveryDB::new()));

    let domain_participant_guid = GUID::new();
    let dp_event_wrapper = DPEventWrapper::new(
      domain_participant_guid,
      HashMap::new(),
      ddshc,
      discovery_db,
      HashMap::new(),
      GuidPrefix::default(),
      TokenReceiverPair {
        token: ADD_READER_TOKEN,
        receiver: receiver_add,
      },
      TokenReceiverPair {
        token: REMOVE_READER_TOKEN,
        receiver: receiver_remove,
      },
      TokenReceiverPair {
        token: ADD_WRITER_TOKEN,
        receiver: add_writer_receiver,
      },
      TokenReceiverPair {
        token: REMOVE_WRITER_TOKEN,
        receiver: remove_writer_receiver,
      },
      stop_poll_receiver,
      discovery_update_notification_receiver,
    );

    let (sender_stop, receiver_stop) = mio_channel::channel::<i32>();
    dp_event_wrapper
      .poll
      .register(
        &receiver_stop,
        STOP_POLL_TOKEN,
        Ready::readable(),
        PollOpt::edge(),
      )
      .expect("Failed to register receivers.");

    let child = thread::spawn(move || DPEventWrapper::event_loop(dp_event_wrapper));

    let n = 3;

    let mut reader_guids = Vec::new();
    for i in 0..n {
      let new_guid = GUID::new();

      let (send, _rec) = mio_channel::sync_channel::<()>(100);
      let new_reader = Reader::new(
        new_guid,
        send,
        Arc::new(RwLock::new(DDSCache::new())),
        "test".to_string(),
      );

      reader_guids.push(new_reader.get_guid().clone());
      println!("\nSent reader number {}: {:?}\n", i, &new_reader);
      sender_add_reader.send(new_reader).unwrap();
      std::thread::sleep(Duration::new(0, 100));
    }

    println!("\npoistetaan toka\n");
    let some_guid = reader_guids[1].clone();
    sender_remove_reader.send(some_guid).unwrap();
    std::thread::sleep(Duration::new(0, 100));

    println!("\nLopetustoken lähtee\n");
    sender_stop.send(0).unwrap();
    child.join().unwrap();
  }
}
