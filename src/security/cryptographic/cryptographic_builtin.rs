mod aes_gcm_gmac;
mod builtin_key;
mod crypto_key_exchange;
mod crypto_key_factory;
mod crypto_transform;
mod decode;
mod encode;
mod key_material;
pub(crate) mod types;

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
use self::{builtin_key::*, key_material::*};

// A struct implementing the builtin Cryptographic plugin
// See sections 8.5 and 9.5 of the Security specification (v. 1.1)
pub struct CryptographicBuiltin {
  // encode: indices are remote (receiver) handles and local (sender) handles.
  // Local sender handle yields encryption key shared with all receivers.
  // Remote receiver handle yields (duplicates of) the same shared key
  // and a receiver-specific encryption (signing) key.
  // All the keys we get here are locally generated.
  encode_key_materials: HashMap<CryptoHandle, KeyMaterial_AES_GCM_GMAC_seq>,

  // decode: indices are remote (sender) handles
  // Remote endpoint handle yields remote endpoint's encoding key (for us, it is decoding key)
  // and a receiver-specific key (remote uses to sign, we use to verify) from remote to us.
  // All the keys we get here are received over Discovery.
  decode_key_materials: HashMap<CryptoHandle, KeyMaterial_AES_GCM_GMAC_seq>,

  participant_encrypt_options: HashMap<ParticipantCryptoHandle, ParticipantSecurityAttributes>,
  endpoint_encrypt_options: HashMap<EndpointCryptoHandle, EndpointSecurityAttributes>,
  participant_to_endpoint_info: HashMap<ParticipantCryptoHandle, HashSet<EndpointInfo>>,
  // For reverse lookups
  endpoint_to_participant: HashMap<EndpointCryptoHandle, ParticipantCryptoHandle>,

  // sessions
  //
  // TODO: The session ids should be stored in a data structure.
  // Associated with each should be a (sent) data block counter, so we should count how many
  // outgoing encrypted blocks are sent. (per remote endpoint)
  // There should be a configuration variable `max_blocks_per_session`. When a counter
  // reaches that, the session_id is changed. Likely a simple increment is sufficient, but random
  // successor is also ok.
  // Initial session id values are also arbitrary.
  // See DDS Security Spec v1.1 Section "9.5.3.3.4 Computation of ciphertext from plaintext"
  //
  // TODO: The current session_id is just a constant. Implementing counting above requires
  // some access to shared mutable state from encryption/send operations.
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
    crypto_handle: CryptoHandle,
    _key_id: CryptoTransformKeyId,
  ) -> SecurityResult<&KeyMaterial_AES_GCM_GMAC_seq> {
    // TODO:
    // Received packet is specifying key_id used to encrypt, but
    // we just ignore that and assume the key_id is uniquely determined by
    // crypto handle.
    // So implement storing multiple keys per handle, distinguished by key_id.
    // See "9.5.3.3.5 Computation of plaintext from ciphertext"

    self
      .decode_key_materials
      .get(&crypto_handle)
      .ok_or_else(|| {
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

  fn session_id(&self) -> SessionId {
    // TODO: This should change at times. See comment at struct definition.
    SessionId::new([1, 3, 3, 7])
  }

  fn random_initialization_vector(&self) -> BuiltinInitializationVector {
    BuiltinInitializationVector::new(self.session_id(), rand::random())
  }

  fn compute_session_key(
    transformation_kind: BuiltinCryptoTransformationKind,
    rec_spec: ReceiverSpecific,
    master_key: &BuiltinKey,
    master_salt: &[u8],
    iv: BuiltinInitializationVector,
  ) -> SecurityResult<BuiltinKey> {
    // "9.5.3.3.3 Computation of SessionKey and SessionReceiverSpecificKey"
    //
    // TODO: Extract this block to a separate function and reuse in
    // other transform functions below.
    use ring::hmac;

    let magic_prefix = match rec_spec {
      ReceiverSpecific::No => b"SessionKey".as_ref(),
      ReceiverSpecific::Yes => b"SessionReceiverKey".as_ref(),
    };

    let ring_master_key = hmac::Key::new(hmac::HMAC_SHA256, master_key.as_bytes());
    let digest = hmac::sign(
      &ring_master_key,
      &[magic_prefix, master_salt, iv.session_id().as_bytes()].concat(),
    );

    BuiltinKey::from_bytes(KeyLength::try_from(transformation_kind)?, digest.as_ref())
  }

  // Get materials needed for encrypting
  fn sender_session_crypto_materials(
    &self,
    sender_handle: CryptoHandle,
    payload_only: bool,
    receiver_specific_crypto_handles: &[CryptoHandle],
  ) -> SecurityResult<EncryptSessionMaterials> {
    let keymaterial_selector = if payload_only {
      KeyMaterial_AES_GCM_GMAC_seq::payload_key_material
    } else {
      KeyMaterial_AES_GCM_GMAC_seq::key_material
    };

    let endpoint_key_material = self
      .get_encode_key_materials(&sender_handle)
      .map(keymaterial_selector)?;

    let KeyMaterial_AES_GCM_GMAC {
      transformation_kind,
      master_salt,
      sender_key_id,
      master_sender_key,
      ..
    } = endpoint_key_material;

    let transformation_kind = *transformation_kind;

    let initialization_vector = self.random_initialization_vector();

    let session_key = Self::compute_session_key(
      transformation_kind,
      ReceiverSpecific::No,
      master_sender_key,
      master_salt,
      initialization_vector,
    )?;

    // Get the key materials for computing receiver-specific MACs
    let receiver_specific_keys = SecurityResult::<Vec<ReceiverSpecificKeyMaterial>>::from_iter(
      // Iterate over receiver handles
      receiver_specific_crypto_handles
        .iter()
        .map(|receiver_crypto_handle| {
          self
            .get_encode_key_materials(receiver_crypto_handle)
            .map(keymaterial_selector)
            // Compare to the common key material and get the receiver specific key material
            .and_then(|receiver_key_material| {
              receiver_key_material.receiver_key_material_for(endpoint_key_material)
            })
            // Map to session keys
            .and_then(|rec_spec_key_material| {
              let session_key = Self::compute_session_key(
                transformation_kind,
                ReceiverSpecific::Yes,
                &rec_spec_key_material.key,
                master_salt,
                initialization_vector,
              )?;
              Ok(ReceiverSpecificKeyMaterial {
                key_id: rec_spec_key_material.key_id,
                key: session_key,
              })
            })
        }),
    )?;

    Ok(EncryptSessionMaterials {
      key_id: *sender_key_id,
      transformation_kind,
      session_key,
      initialization_vector,
      receiver_specific_keys,
    })
  }

  // Get materials needed for encrypting
  fn receiver_session_crypto_materials(
    &self,
    remote_sender_handle: CryptoHandle,
    header_key_id: CryptoTransformKeyId, // what key id was specified on incoming header
    payload_only: bool,
    initialization_vector: BuiltinInitializationVector, // as received in header
  ) -> SecurityResult<DecryptSessionMaterials> {
    let keymaterial_selector = if payload_only {
      KeyMaterial_AES_GCM_GMAC_seq::payload_key_material
    } else {
      KeyMaterial_AES_GCM_GMAC_seq::key_material
    };

    let KeyMaterial_AES_GCM_GMAC {
      transformation_kind,
      master_salt,
      sender_key_id,
      master_sender_key,
      receiver_specific_key_id,
      master_receiver_specific_key,
    } = self
      .get_decode_key_materials(remote_sender_handle, header_key_id)
      .map(keymaterial_selector)?;

    let transformation_kind = *transformation_kind;
    let session_key = Self::compute_session_key(
      transformation_kind,
      ReceiverSpecific::No,
      master_sender_key,
      master_salt,
      initialization_vector,
    )?;

    let receiver_specific_key = if *receiver_specific_key_id == 0 {
      None // does not exist
    } else {
      let session_key = Self::compute_session_key(
        transformation_kind,
        ReceiverSpecific::Yes,
        master_receiver_specific_key,
        master_salt,
        initialization_vector,
      )?;
      Some(ReceiverSpecificKeyMaterial {
        key_id: *receiver_specific_key_id,
        key: session_key,
      })
    };

    Ok(DecryptSessionMaterials {
      key_id: *sender_key_id,
      transformation_kind,
      session_key,
      receiver_specific_key,
    })
  }
}

struct EncryptSessionMaterials {
  key_id: CryptoTransformKeyId, // key identifier over the wire
  transformation_kind: BuiltinCryptoTransformationKind, // encrypt/sign/none
  session_key: BuiltinKey,      // session-specific AES-GCM key
  initialization_vector: BuiltinInitializationVector, /* AES-GCM also needs init vector (shared,
                                 * but not secret) */
  // there may be several receivers
  receiver_specific_keys: Vec<ReceiverSpecificKeyMaterial>,
}

struct DecryptSessionMaterials {
  key_id: CryptoTransformKeyId, // key identifier over the wire
  transformation_kind: BuiltinCryptoTransformationKind, // encrypt/sign/none
  session_key: BuiltinKey,      // session-specific AES-GCM key
  // initialization vector is not returned, because it is received from sender, so caller already
  // knows it
  receiver_specific_key: Option<ReceiverSpecificKeyMaterial>,
  // Either we have receiver specific key material specific to us or not.
}
