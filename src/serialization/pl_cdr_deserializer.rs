use std::marker::PhantomData;

use serde::{de::DeserializeOwned, Deserializer};

use crate::{
  dds::traits::{
    serde_adapters::{no_key, with_key, no_key::DeserializerAdapter,},
    Keyed,
  },
  messages::submessages::submessage_elements::serialized_payload::RepresentationIdentifier,
  serialization::{
    cdr_deserializer::CDRDeserializerAdapter,
    error::{Error, Result}
  },
};

pub struct PlCdrDeserializerAdapter<D> {
  phantom: PhantomData<D>,
}

const REPR_IDS: [RepresentationIdentifier; 4] = [
  // CDR_* are only added for random interoperability
  RepresentationIdentifier::CDR_BE,
  RepresentationIdentifier::CDR_LE,
  // PL_CDR_* are expected
  RepresentationIdentifier::PL_CDR_BE,
  RepresentationIdentifier::PL_CDR_LE,
];

impl<D> no_key::DeserializerAdapter<D> for PlCdrDeserializerAdapter<D>
where
  D: DeserializeOwned,
{
  fn supported_encodings() -> &'static [RepresentationIdentifier] {
    &REPR_IDS
  }

  fn from_bytes(input_bytes: &[u8], encoding: RepresentationIdentifier) -> Result<D> {
    match encoding {
      RepresentationIdentifier::PL_CDR_LE | RepresentationIdentifier::CDR_LE => {
        PlCdrDeserializer::from_little_endian_bytes::<D>(input_bytes)
      }
      RepresentationIdentifier::PL_CDR_BE | RepresentationIdentifier::CDR_BE => {
        PlCdrDeserializer::from_big_endian_bytes::<D>(input_bytes)
      }
      repr_id => Err(Error::Message(format!(
        "Unknown representation identifier {:?}",
        repr_id
      ))),
    }
  }
}

impl<D> with_key::DeserializerAdapter<D> for PlCdrDeserializerAdapter<D>
where
  D: Keyed + DeserializeOwned,
  <D as Keyed>::K: DeserializeOwned, // why is this not inferred from D:Keyed ?
{
  fn key_from_bytes(input_bytes: &[u8], encoding: RepresentationIdentifier) -> Result<D::K> {
    // Horrible hack warning:
    //
    // Here we just change encoding without any good definitional justification.
    // It just often happens that PL_CDR-encoded data has its Key encoded in
    // plain CDR.
    // 
    // The only known use for this is in Discovery, where they Key is always a GUID, if any.
    // And the GUID is serializes as-is, without any encoding.

    CDRDeserializerAdapter::<D::K>::from_bytes(input_bytes, encoding)
  }
}

pub struct PlCdrDeserializer<'de> {
  endianness: RepresentationIdentifier,
  input: &'de [u8],
}

impl<'de> PlCdrDeserializer<'de> {
  pub fn new(s: &'de [u8], endianness: RepresentationIdentifier) -> PlCdrDeserializer {
    PlCdrDeserializer {
      endianness,
      input: s,
    }
  }

  pub fn from_little_endian_bytes<T: DeserializeOwned>(s: &[u8]) -> Result<T> {
    let deserializer = PlCdrDeserializer::new(s, RepresentationIdentifier::PL_CDR_LE);
    T::deserialize(deserializer)
  }

  pub fn from_big_endian_bytes<T: DeserializeOwned>(s: &[u8]) -> Result<T> {
    eprintln!("from_big_endian_bytes");
    let deserializer = PlCdrDeserializer::new(s, RepresentationIdentifier::PL_CDR_BE);
    T::deserialize(deserializer)
  }

  fn custom_deserialize_any<V>(self, visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    match self.endianness {
      RepresentationIdentifier::PL_CDR_LE | RepresentationIdentifier::PL_CDR_BE => {
        visitor.visit_bytes(&[&self.endianness.to_bytes(), self.input].concat())
      }
      e => Err(Error::Message(format!("Unsupported endianness {:?}", e))),
    }
  }
}

impl<'de> Deserializer<'de> for PlCdrDeserializer<'de> {
  type Error = Error;

  fn deserialize_any<V>(self, visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    //eprintln!("deserialize_any ");
    self.custom_deserialize_any(visitor)
  }

  fn deserialize_bool<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_i8<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_i16<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_i32<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_i64<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_u8<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_u16<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_u32<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_u64<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_f32<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_f64<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_char<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_str<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_string<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_bytes<V>(self, visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    visitor.visit_bytes(self.input)
  }

  fn deserialize_byte_buf<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_option<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_unit<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_unit_struct<V>(self, _name: &'static str, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_newtype_struct<V>(self, _name: &'static str, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_seq<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_tuple<V>(self, _len: usize, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_tuple_struct<V>(
    self,
    _name: &'static str,
    _len: usize,
    _visitor: V,
  ) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_map<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_struct<V>(
    self,
    _name: &'static str,
    _fields: &'static [&'static str],
    visitor: V,
  ) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    self.deserialize_any(visitor)
  }

  fn deserialize_enum<V>(
    self,
    _name: &'static str,
    _variants: &'static [&'static str],
    _visitor: V,
  ) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_identifier<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }

  fn deserialize_ignored_any<V>(self, _visitor: V) -> Result<V::Value>
  where
    V: serde::de::Visitor<'de>,
  {
    eprintln!("Hit unimplemented function: ");
    unimplemented!()
  }
}
