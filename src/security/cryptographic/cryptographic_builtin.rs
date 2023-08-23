mod aes_gcm_gmac;
mod crypto_key_exchange;
mod crypto_key_factory;
mod crypto_transform;
mod decode;
mod encode;
mod types;

use std::collections::{HashMap, HashSet};

use crate::{
  security::{
    access_control::types::*,
    authentication::types::*,
    cryptographic::{cryptographic_builtin::types::*, cryptographic_plugin::*, types::*},
    types::*,
  },
  security_error,
};

// A struct implementing the builtin Cryptographic plugin
// See sections 8.5 and 9.5 of the Security specification (v. 1.1)
pub struct CryptographicBuiltin {
  encode_key_materials: HashMap<CryptoHandle, KeyMaterial_AES_GCM_GMAC_seq>,
  decode_key_materials: HashMap<CryptoHandle, KeyMaterial_AES_GCM_GMAC_seq>,
  participant_encrypt_options: HashMap<ParticipantCryptoHandle, ParticipantSecurityAttributes>,
  endpoint_encrypt_options: HashMap<EndpointCryptoHandle, EndpointSecurityAttributes>,
  participant_to_endpoint_info: HashMap<ParticipantCryptoHandle, HashSet<EndpointInfo>>,
  // For reverse lookups
  endpoint_to_participant: HashMap<EndpointCryptoHandle, ParticipantCryptoHandle>,

  // sessions_ ?
  /// For each (local datawriter (/datareader), remote participant) pair, stores
  /// the matched remote datareader (/datawriter)
  matched_remote_endpoint:
    HashMap<EndpointCryptoHandle, HashMap<ParticipantCryptoHandle, EndpointCryptoHandle>>,
  ///For reverse lookups,  for each remote datawriter (/datareader), stores the
  /// matched local datareader (/datawriter)
  matched_local_endpoint: HashMap<EndpointCryptoHandle, EndpointCryptoHandle>,

  crypto_handle_counter: u32,
}

// Combine the trait implementations from the submodules
impl super::Cryptographic for CryptographicBuiltin {}

impl CryptographicBuiltin {
  pub fn new() -> Self {
    CryptographicBuiltin {
      encode_key_materials: HashMap::new(),
      decode_key_materials: HashMap::new(),
      participant_encrypt_options: HashMap::new(),
      endpoint_encrypt_options: HashMap::new(),
      participant_to_endpoint_info: HashMap::new(),
      endpoint_to_participant: HashMap::new(),
      matched_remote_endpoint: HashMap::new(),
      matched_local_endpoint: HashMap::new(),
      crypto_handle_counter: 0,
    }
  }

  fn insert_encode_key_materials(
    &mut self,
    crypto_handle: CryptoHandle,
    key_materials: KeyMaterial_AES_GCM_GMAC_seq,
  ) -> SecurityResult<()> {
    match self
      .encode_key_materials
      .insert(crypto_handle, key_materials)
    {
      None => SecurityResult::Ok(()),
      Some(old_key_materials) => {
        self
          .encode_key_materials
          .insert(crypto_handle, old_key_materials);
        SecurityResult::Err(security_error!(
          "The CryptoHandle {} was already associated with encode key material",
          crypto_handle
        ))
      }
    }
  }
  fn get_encode_key_materials(
    &self,
    crypto_handle: &CryptoHandle,
  ) -> SecurityResult<&KeyMaterial_AES_GCM_GMAC_seq> {
    self.encode_key_materials.get(crypto_handle).ok_or_else(|| {
      security_error!(
        "Could not find encode key materials for the CryptoHandle {}",
        crypto_handle
      )
    })
  }

  fn insert_decode_key_materials(
    &mut self,
    crypto_handle: CryptoHandle,
    key_materials: KeyMaterial_AES_GCM_GMAC_seq,
  ) -> SecurityResult<()> {
    match self
      .decode_key_materials
      .insert(crypto_handle, key_materials)
    {
      None => SecurityResult::Ok(()),
      Some(old_key_materials) => {
        self
          .decode_key_materials
          .insert(crypto_handle, old_key_materials);
        SecurityResult::Err(security_error!(
          "The CryptoHandle {} was already associated with decode key material",
          crypto_handle
        ))
      }
    }
  }

  fn get_decode_key_materials(
    &self,
    crypto_handle: &CryptoHandle,
  ) -> SecurityResult<&KeyMaterial_AES_GCM_GMAC_seq> {
    self.decode_key_materials.get(crypto_handle).ok_or_else(|| {
      security_error!(
        "Could not find decode key materials for the CryptoHandle {}",
        crypto_handle
      )
    })
  }

  fn insert_endpoint_info(
    &mut self,
    participant_crypto_handle: ParticipantCryptoHandle,
    endpoint_info: EndpointInfo,
  ) {
    match self
      .participant_to_endpoint_info
      .get_mut(&participant_crypto_handle)
    {
      Some(endpoint_set) => {
        endpoint_set.insert(endpoint_info);
      }
      None => {
        self
          .participant_to_endpoint_info
          .insert(participant_crypto_handle, HashSet::from([endpoint_info]));
      }
    };
  }

  fn insert_participant_attributes(
    &mut self,
    participant_crypto_handle: ParticipantCryptoHandle,
    attributes: ParticipantSecurityAttributes,
  ) -> SecurityResult<()> {
    match self
      .participant_encrypt_options
      .insert(participant_crypto_handle, attributes)
    {
      None => SecurityResult::Ok(()),
      Some(old_attributes) => {
        self
          .participant_encrypt_options
          .insert(participant_crypto_handle, old_attributes);
        SecurityResult::Err(security_error!(
          "The ParticipantCryptoHandle {} was already associated with security attributes",
          participant_crypto_handle
        ))
      }
    }
  }

  fn insert_endpoint_attributes(
    &mut self,
    endpoint_crypto_handle: EndpointCryptoHandle,
    attributes: EndpointSecurityAttributes,
  ) -> SecurityResult<()> {
    match self
      .endpoint_encrypt_options
      .insert(endpoint_crypto_handle, attributes)
    {
      None => SecurityResult::Ok(()),
      Some(old_attributes) => {
        self
          .endpoint_encrypt_options
          .insert(endpoint_crypto_handle, old_attributes);
        SecurityResult::Err(security_error!(
          "The EndpointCryptoHandle {} was already associated with security attributes",
          endpoint_crypto_handle
        ))
      }
    }
  }
}
