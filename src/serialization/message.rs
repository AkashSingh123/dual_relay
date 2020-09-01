use std::io;

use crate::{
  structure::{sequence_number::SequenceNumber, guid::GuidPrefix},
  serialization::submessage::SubMessage,
  submessages::*,
  messages::header::Header,
};
use speedy::{Readable, Writable, Endianness, Reader, Context, Writer};
use enumflags2::BitFlags;

#[derive(Debug)]
pub struct Message {
  pub header: Header,
  pub submessages: Vec<SubMessage>,
}


impl<'a> Message {
  pub fn deserialize_header(context: Endianness, buffer: &'a [u8]) -> Header {
    Header::read_from_buffer_with_ctx(context, buffer).unwrap()
  }

  pub fn serialize_header(self) -> Vec<u8> {
    let buffer = self.header.write_to_vec_with_ctx(Endianness::LittleEndian);
    buffer.unwrap()
  }

  pub fn add_submessage(&mut self, submessage: SubMessage) {
    self.submessages.push(submessage);
  }

  pub fn remove_submessage(mut self, index: usize) {
    self.submessages.remove(index);
  }

  pub fn submessages(self) -> Vec<SubMessage> {
    self.submessages
  }

  //fn submessages_borrow(&self) -> &Vec<SubMessage> {
  //  &self.submessages
  //}

  pub fn set_header(&mut self, header: Header) {
    self.header = header;
  }

  pub fn get_data_sub_message_sequence_numbers(&self) -> Vec<SequenceNumber> {
    let mut sequence_numbers: Vec<SequenceNumber> = vec![];
    for mes in self.submessages.iter() {
      if let Some(EntitySubmessage::Data(data_subm , _ )) = &mes.submessage {
        sequence_numbers.push(data_subm.writer_sn);
      }
    }
    sequence_numbers
  }

  // We implement this instead of Speedy trait Readable, because
  // we need to run-time decide which endianness we input. Speedy requires the
  // top level to fix that. And there seems to be no reasonable way to change endianness.
  // TODO: The error type should be something better
  pub fn read_from_buffer(buffer: &'a [u8]) -> io::Result<Message> {
    
    //let endianess = reader.endianness();
    // message.header = SubMessage::deserialize_header(C, reader.)

    // The Header deserializes the same
    let rtps_header = Header::read_from_buffer(buffer)
      .map_err( |e| io::Error::new(io::ErrorKind::Other,e))?;

    let mut message = Message::new(rtps_header);

    let mut submessages_left = &buffer[ 40 .. ]; // header is 40 bytes

    // submessage loop
    while submessages_left.len() > 0 {
      let sub_header = SubmessageHeader::read_from_buffer(submessages_left)
        .map_err( |e| io::Error::new(io::ErrorKind::Other,e))?;

      // Try to figure out how large this submessage is.
      let sub_header_length = 4; // 4 bytes
      let sub_content_length = 
        if sub_header.content_length == 0 {
          // RTPS spec 2.3, section 9.4.5.1.3:
          //           In case octetsToNextHeader==0 and the kind of Submessage is
          // NOT PAD or INFO_TS, the Submessage is the last Submessage in the Message
          // and extends up to the end of the Message. This makes it possible to send
          // Submessages larger than 64k (the size that can be stored in the
          // octetsToNextHeader field), provided they are the last Submessage in the
          // Message. In case the octetsToNextHeader==0 and the kind of Submessage is
          // PAD or INFO_TS, the next Submessage header starts immediately after the
          // current Submessage header OR the PAD or INFO_TS is the last Submessage
          // in the Message.
          match sub_header.kind {
            SubmessageKind::PAD | 
            SubmessageKind::INFO_TS => 0,
            _not_pad_or_info_ts => submessages_left.len() - sub_header_length ,
          }
        }
        else { sub_header.content_length as usize };

      let (sub_buffer,submessages_left) = 
        submessages_left.split_at( sub_header_length + sub_content_length );

      let e = endianness_flag( sub_header.flags );

      let mk_e_subm = move |s| 
        {Ok ( SubMessage { 
                header:sub_header ,
                submessage: Some (s),
                intepreterSubmessage: None,
              }
            ) 
        };
      let mk_i_subm = move |s| 
        {Ok ( SubMessage { 
                header:sub_header ,
                submessage: None,
                intepreterSubmessage: Some(s),
              }
            ) 
        };

      let new_submessage_result : io::Result<SubMessage> = 
        match sub_header.kind {

          SubmessageKind::DATA => {
            let f = BitFlags::<Submessage_DATA_Flags>::from_bits_truncate(sub_header.flags);
            mk_e_subm( EntitySubmessage::Data(Data::deserialize_data(sub_buffer,e,f)? , f ) ) 
          },

          SubmessageKind::HEARTBEAT => {
            let f = BitFlags::<Submessage_HEARTBEAT_Flags>::from_bits_truncate(sub_header.flags);
            mk_e_subm( EntitySubmessage::Heartbeat(Heartbeat::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },
          
          SubmessageKind::GAP => {
            let f = BitFlags::<Submessage_GAP_Flags>::from_bits_truncate(sub_header.flags);
            mk_e_subm( EntitySubmessage::Gap(Gap::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },

          SubmessageKind::ACKNACK => {
            let f = BitFlags::<Submessage_ACKNACK_Flags>::from_bits_truncate(sub_header.flags);
            mk_e_subm( EntitySubmessage::AckNack(AckNack::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },

          SubmessageKind::NACK_FRAG => {
            let f = BitFlags::<Submessage_NACKFRAG_Flags>::from_bits_truncate(sub_header.flags);
            mk_e_subm( EntitySubmessage::NackFrag(NackFrag::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },

          // interpreter submessages

          SubmessageKind::INFO_DST => {
            let f = BitFlags::<Submessage_INFODESTINATION_Flags>::from_bits_truncate(sub_header.flags);
            mk_i_subm( InterpreterSubmessage::InfoDestination(InfoDestination::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },
          SubmessageKind::INFO_SRC => {
            let f = BitFlags::<Submessage_INFOSOURCE_Flags>::from_bits_truncate(sub_header.flags);
            mk_i_subm( InterpreterSubmessage::InfoSource(InfoSource::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },
          SubmessageKind::INFO_TS => {
            let f = BitFlags::<Submessage_INFOTIMESTAMP_Flags>::from_bits_truncate(sub_header.flags);
            mk_i_subm( InterpreterSubmessage::InfoTimestamp(InfoTimestamp::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },
          SubmessageKind::INFO_REPLY => {
            let f = BitFlags::<Submessage_INFOREPLY_Flags>::from_bits_truncate(sub_header.flags);
            mk_i_subm( InterpreterSubmessage::InfoReply(InfoReply::read_from_buffer_with_ctx(e, sub_buffer)? , f ) ) 
          },
          SubmessageKind::PAD => {
            continue // nothing to do here
          }
          unknown_kind => {
            println!("Received unknown submessage kind {:?}", unknown_kind);
            continue
          }
        }; // match

      message.submessages.push(new_submessage_result?)
    } // loop

    Ok(message)    
  }

}

impl Message {
  pub fn new(header: Header) -> Message {
    Message {
      header,
      submessages: vec![],
    }
  }
}

impl Default for Message {
  fn default() -> Self {
    Message {
      header: Header::new(GuidPrefix::GUIDPREFIX_UNKNOWN),
      submessages: vec![],
    }
  }
}

impl<C: Context> Writable<C> for Message {
  fn write_to<'a, T: ?Sized + Writer<C>>(&'a self, writer: &mut T) -> Result<(), C::Error> {
    writer.write_value(&self.header)?;
    for x in &self.submessages {
      writer.write_value(&x)?;
    }
    Ok(())
  }
}


#[cfg(test)]

mod tests {
  use super::*;
  use crate::speedy::{Writable, Readable};

  #[test]

  fn RTPS_message_test_shapes_demo_message_deserialization() {
    // Data message should contain Shapetype values.
    // caprured with wireshark from shapes demo.
    // packet with INFO_DST, INFO_TS, DATA, HEARTBEAT
    let bits1: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x03, 0x01, 0x0f, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x0c, 0x00, 0x01, 0x03, 0x00, 0x0c, 0x29, 0x2d,
      0x31, 0xa2, 0x28, 0x20, 0x02, 0x08, 0x09, 0x01, 0x08, 0x00, 0x1a, 0x15, 0xf3, 0x5e, 0x00,
      0xcc, 0xfb, 0x13, 0x15, 0x05, 0x2c, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x07,
      0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x52, 0x45, 0x44, 0x00, 0x69, 0x00, 0x00, 0x00, 0x17, 0x00,
      0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x07, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00,
      0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x5b, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00,
    ];
    let rtps = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits1).unwrap();
    println!("{:?}", rtps);

    let serialized = rtps
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits1, serialized);
  }
  #[test]
  fn RTPS_message_test_shapes_demo_DataP() {
    // / caprured with wireshark from shapes demo.
    // packet with DATA(p)
    let bits2: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x04, 0x01, 0x03, 0x01, 0x03, 0x00, 0x0c, 0x29, 0x2d, 0x31,
      0xa2, 0x28, 0x20, 0x02, 0x08, 0x15, 0x05, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x00, 0x00, 0x00,
      0x03, 0x00, 0x00, 0x77, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x15, 0x00, 0x04, 0x00, 0x02, 0x04, 0x00, 0x00, 0x50, 0x00, 0x10,
      0x00, 0x01, 0x03, 0x00, 0x0c, 0x29, 0x2d, 0x31, 0xa2, 0x28, 0x20, 0x02, 0x08, 0x00, 0x00,
      0x01, 0xc1, 0x16, 0x00, 0x04, 0x00, 0x01, 0x03, 0x00, 0x00, 0x44, 0x00, 0x04, 0x00, 0x3f,
      0x0c, 0x00, 0x00, 0x58, 0x00, 0x04, 0x00, 0x3f, 0x0c, 0x00, 0x00, 0x32, 0x00, 0x18, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x9f, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x50, 0x8e, 0xc9, 0x32, 0x00, 0x18, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x9f, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0xc0, 0xa8, 0x45, 0x14, 0x32, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x9f, 0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0xac, 0x11, 0x00, 0x01, 0x33, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xea, 0x1c,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef,
      0xff, 0x00, 0x01, 0x31, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0x39, 0x30, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00,
      0x01, 0x48, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0x39, 0x30, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x01, 0x34,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0xb0, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x02, 0x00, 0x08, 0x00, 0x2c, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00,
    ];

    let rtps_data = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits2).unwrap();

    let serialized_data = rtps_data
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits2, serialized_data);
  }

  #[test]
  fn RTPS_message_test_shapes_demo_info_TS_dataP() {
    // caprured with wireshark from shapes demo.
    // rtps packet with info TS and Data(p)
    let bits1: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x03, 0x01, 0x0f, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x01, 0x08, 0x00, 0x0e, 0x15, 0xf3, 0x5e, 0x00, 0x28,
      0x74, 0xd2, 0x15, 0x05, 0xa8, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0xc7, 0x00,
      0x01, 0x00, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x15, 0x00, 0x04, 0x00, 0x02, 0x03, 0x00, 0x00, 0x16, 0x00, 0x04, 0x00, 0x01, 0x0f, 0x00,
      0x00, 0x50, 0x00, 0x10, 0x00, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0xc1, 0x32, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf4,
      0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x0a, 0x50, 0x8e, 0x68, 0x31, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf5, 0x1c, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x50,
      0x8e, 0x68, 0x02, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58,
      0x00, 0x04, 0x00, 0x3f, 0x0c, 0x3f, 0x0c, 0x62, 0x00, 0x18, 0x00, 0x14, 0x00, 0x00, 0x00,
      0x66, 0x61, 0x73, 0x74, 0x72, 0x74, 0x70, 0x73, 0x50, 0x61, 0x72, 0x74, 0x69, 0x63, 0x69,
      0x70, 0x61, 0x6e, 0x74, 0x00, 0x01, 0x00, 0x00, 0x00,
    ];

    let rtps = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits1).unwrap();
    println!("{:?}", rtps);

    let serialized = rtps
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits1, serialized);
  }

  #[test]
  fn RTPS_message_test_shapes_demo_info_TS_AckNack() {
    // caprured with wireshark from shapes demo.
    // rtps packet with info TS three AckNacks
    let bits1: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x04, 0x01, 0x03, 0x01, 0x03, 0x00, 0x0c, 0x29, 0x2d, 0x31,
      0xa2, 0x28, 0x20, 0x02, 0x08, 0x0e, 0x01, 0x0c, 0x00, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x03, 0x18, 0x00, 0x00, 0x00, 0x03, 0xc7, 0x00,
      0x00, 0x03, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x01, 0x00, 0x00, 0x00, 0x06, 0x03, 0x18, 0x00, 0x00, 0x00, 0x04, 0xc7, 0x00, 0x00, 0x04,
      0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x06, 0x03, 0x18, 0x00, 0x00, 0x02, 0x00, 0xc7, 0x00, 0x02, 0x00, 0xc2, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    ];

    let rtps = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits1).unwrap();
    println!("{:?}", rtps);

    let serialized = rtps
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits1, serialized);
  }

  #[test]
  fn RTPS_message_info_ts_and_dataP() {
    // caprured with wireshark from shapes demo.
    // rtps packet with info TS and data(p)
    let bits1: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x03, 0x01, 0x0f, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x09, 0x01, 0x08, 0x00, 0x0e, 0x15, 0xf3, 0x5e, 0x00, 0x28,
      0x74, 0xd2, 0x15, 0x05, 0xa8, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0xc7, 0x00,
      0x01, 0x00, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
      0x15, 0x00, 0x04, 0x00, 0x02, 0x03, 0x00, 0x00, 0x16, 0x00, 0x04, 0x00, 0x01, 0x0f, 0x00,
      0x00, 0x50, 0x00, 0x10, 0x00, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00, 0x00, 0x01, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x01, 0xc1, 0x32, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf4,
      0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x0a, 0x50, 0x8e, 0x68, 0x31, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf5, 0x1c, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x50,
      0x8e, 0x68, 0x02, 0x00, 0x08, 0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x58,
      0x00, 0x04, 0x00, 0x3f, 0x0c, 0x3f, 0x0c, 0x62, 0x00, 0x18, 0x00, 0x14, 0x00, 0x00, 0x00,
      0x66, 0x61, 0x73, 0x74, 0x72, 0x74, 0x70, 0x73, 0x50, 0x61, 0x72, 0x74, 0x69, 0x63, 0x69,
      0x70, 0x61, 0x6e, 0x74, 0x00, 0x01, 0x00, 0x00, 0x00,
    ];

    let rtps = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits1).unwrap();
    println!("{:?}", rtps);

    let serialized = rtps
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits1, serialized);
  }

  #[test]
  fn RTPS_message_infoDST_infoTS_Data_w_heartbeat() {
    // caprured with wireshark from shapes demo.
    // rtps packet with InfoDST InfoTS Data(w) Heartbeat
    // This datamessage serialized payload maybe contains topic name (square) and its type (shapetype)
    // look https://www.omg.org/spec/DDSI-RTPS/2.3/PDF page 185
    let bits1: Vec<u8> = vec![
      0x52, 0x54, 0x50, 0x53, 0x02, 0x03, 0x01, 0x0f, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x0e, 0x01, 0x0c, 0x00, 0x01, 0x03, 0x00, 0x0c, 0x29, 0x2d,
      0x31, 0xa2, 0x28, 0x20, 0x02, 0x08, 0x09, 0x01, 0x08, 0x00, 0x12, 0x15, 0xf3, 0x5e, 0x00,
      0xc8, 0xa9, 0xfa, 0x15, 0x05, 0x0c, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x03, 0xc7,
      0x00, 0x00, 0x03, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
      0x00, 0x2f, 0x00, 0x18, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf5, 0x1c, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x50, 0x8e, 0x68, 0x50,
      0x00, 0x10, 0x00, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x01, 0xc1, 0x05, 0x00, 0x0c, 0x00, 0x07, 0x00, 0x00, 0x00, 0x53, 0x71, 0x75,
      0x61, 0x72, 0x65, 0x00, 0x00, 0x07, 0x00, 0x10, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x53, 0x68,
      0x61, 0x70, 0x65, 0x54, 0x79, 0x70, 0x65, 0x00, 0x00, 0x00, 0x70, 0x00, 0x10, 0x00, 0x01,
      0x0f, 0x99, 0x06, 0x78, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
      0x5a, 0x00, 0x10, 0x00, 0x01, 0x0f, 0x99, 0x06, 0x78, 0x34, 0x00, 0x00, 0x01, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x01, 0x02, 0x60, 0x00, 0x04, 0x00, 0x5f, 0x01, 0x00, 0x00, 0x15, 0x00,
      0x04, 0x00, 0x02, 0x03, 0x00, 0x00, 0x16, 0x00, 0x04, 0x00, 0x01, 0x0f, 0x00, 0x00, 0x1d,
      0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x08, 0x00, 0xff, 0xff, 0xff, 0x7f,
      0xff, 0xff, 0xff, 0xff, 0x27, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x1b, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff,
      0xff, 0xff, 0x1a, 0x00, 0x0c, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9a,
      0x99, 0x99, 0x19, 0x2b, 0x00, 0x08, 0x00, 0xff, 0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xff,
      0x1f, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x01, 0x1c, 0x00, 0x00, 0x00, 0x03, 0xc7, 0x00, 0x00,
      0x03, 0xc2, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    ];

    let rtps = Message::read_from_buffer_with_ctx(Endianness::LittleEndian, &bits1).unwrap();
    println!("{:?}", rtps);

    let entitySubmessage = rtps.submessages[2].submessage.as_ref().unwrap();
    let dataSubmessage = entitySubmessage.get_data_submessage().unwrap();
    let serializedPayload = dataSubmessage.serialized_payload.value.clone();
    println!();
    println!();
    println!("{:x?}", serializedPayload);

    let serialized = rtps
      .write_to_vec_with_ctx(Endianness::LittleEndian)
      .unwrap();
    assert_eq!(bits1, serialized);
  }

  #[test]
  fn test_RTPS_submessage_flags_helper() {
    let fla: SubmessageFlag = SubmessageFlag {
      flags: 0b00000001_u8,
    };
    let mut helper = SubmessageFlagHelper::get_submessage_flags_helper_from_submessage_flag(
      &SubmessageKind::DATA,
      &fla,
    );
    println!("{:?}", &helper);
    assert_eq!(helper.EndiannessFlag, true);
    assert_eq!(helper.InlineQosFlag, false);
    assert_eq!(helper.DataFlag, false);
    assert_eq!(helper.NonStandardPayloadFlag, false);
    assert_eq!(helper.FinalFlag, false);
    assert_eq!(helper.InvalidateFlag, false);
    assert_eq!(helper.KeyFlag, false);
    assert_eq!(helper.LivelinessFlag, false);
    assert_eq!(helper.MulticastFlag, false);

    let fla_dese = SubmessageFlagHelper::create_submessage_flags_from_flag_helper(
      &SubmessageKind::DATA,
      &helper,
    );
    assert_eq!(fla, fla_dese);

    let fla2: SubmessageFlag = SubmessageFlag {
      flags: 0b00011111_u8,
    };
    helper = SubmessageFlagHelper::get_submessage_flags_helper_from_submessage_flag(
      &SubmessageKind::DATA,
      &fla2,
    );
    println!("{:?}", &helper);
    assert_eq!(helper.EndiannessFlag, true);
    assert_eq!(helper.InlineQosFlag, true);
    assert_eq!(helper.DataFlag, true);
    assert_eq!(helper.NonStandardPayloadFlag, true);
    assert_eq!(helper.FinalFlag, false);
    assert_eq!(helper.InvalidateFlag, false);
    assert_eq!(helper.KeyFlag, true);
    assert_eq!(helper.LivelinessFlag, false);
    assert_eq!(helper.MulticastFlag, false);

    let fla2_dese = SubmessageFlagHelper::create_submessage_flags_from_flag_helper(
      &SubmessageKind::DATA,
      &helper,
    );
    assert_eq!(fla2, fla2_dese);

    let fla3: SubmessageFlag = SubmessageFlag {
      flags: 0b00001010_u8,
    };
    helper = SubmessageFlagHelper::get_submessage_flags_helper_from_submessage_flag(
      &SubmessageKind::DATA,
      &fla3,
    );
    println!("{:?}", &helper);
    assert_eq!(helper.EndiannessFlag, false);
    assert_eq!(helper.InlineQosFlag, true);
    assert_eq!(helper.DataFlag, false);
    assert_eq!(helper.NonStandardPayloadFlag, false);
    assert_eq!(helper.FinalFlag, false);
    assert_eq!(helper.InvalidateFlag, false);
    assert_eq!(helper.KeyFlag, true);
    assert_eq!(helper.LivelinessFlag, false);
    assert_eq!(helper.MulticastFlag, false);

    let fla3_dese = SubmessageFlagHelper::create_submessage_flags_from_flag_helper(
      &SubmessageKind::DATA,
      &helper,
    );
    assert_eq!(fla3, fla3_dese);
  }
}
