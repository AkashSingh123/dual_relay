use core::fmt;
use std::{
  collections::HashMap,
  sync::{Arc, Mutex},
};

use crate::{
  messages::submessages::{
    secure_postfix::SecurePostfix,
    secure_prefix::SecurePrefix,
    submessage::{ReaderSubmessage, WriterSubmessage},
  },
  rtps::{Message, Submessage},
  security_error,
  structure::guid::GuidPrefix,
  QosPolicies, GUID,
};
use super::{
  access_control::*,
  authentication::*,
  cryptographic::{
    DatareaderCryptoHandle, DatawriterCryptoHandle, EncodedSubmessage, EntityCryptoHandle,
    ParticipantCryptoHandle, SecureSubmessageCategory,
  },
  types::*,
  AccessControl, Cryptographic,
};

pub(crate) struct SecurityPlugins {
  pub auth: Box<dyn Authentication>,
  pub access: Box<dyn AccessControl>,
  crypto: Box<dyn Cryptographic>,

  identity_handle_cache_: HashMap<GUID, IdentityHandle>,

  permissions_handle_cache_: HashMap<GUID, PermissionsHandle>,

  participant_crypto_handle_cache_: HashMap<GuidPrefix, ParticipantCryptoHandle>,
  local_entity_crypto_handle_cache_: HashMap<GUID, EntityCryptoHandle>,
  remote_entity_crypto_handle_cache_: HashMap<(GUID, GUID), EntityCryptoHandle>,
}

impl SecurityPlugins {
  pub fn new(
    auth: Box<impl Authentication + 'static>,
    access: Box<impl AccessControl + 'static>,
    crypto: Box<impl Cryptographic + 'static>,
  ) -> Self {
    Self {
      auth,
      access,
      crypto,
      identity_handle_cache_: HashMap::new(),
      permissions_handle_cache_: HashMap::new(),
      participant_crypto_handle_cache_: HashMap::new(),
      local_entity_crypto_handle_cache_: HashMap::new(),
      remote_entity_crypto_handle_cache_: HashMap::new(),
    }
  }

  fn get_identity_handle(&self, guid: &GUID) -> SecurityResult<IdentityHandle> {
    self
      .identity_handle_cache_
      .get(guid)
      .ok_or(security_error!(
        "Could not find an IdentityHandle for the Guid {:?}",
        guid
      ))
      .copied()
  }

  fn get_permissions_handle(&self, guid: &GUID) -> SecurityResult<PermissionsHandle> {
    self
      .permissions_handle_cache_
      .get(guid)
      .ok_or(security_error!(
        "Could not find a PermissionsHandle for the Guid {:?}",
        guid
      ))
      .copied()
  }

  fn get_participant_crypto_handle(
    &self,
    guid_prefix: &GuidPrefix,
  ) -> SecurityResult<ParticipantCryptoHandle> {
    self
      .participant_crypto_handle_cache_
      .get(guid_prefix)
      .ok_or(security_error!(
        "Could not find a ParticipantCryptoHandle for the GuidPrefix {:?}",
        guid_prefix
      ))
      .copied()
  }

  fn get_local_entity_crypto_handle(&self, guid: &GUID) -> SecurityResult<ParticipantCryptoHandle> {
    self
      .local_entity_crypto_handle_cache_
      .get(guid)
      .ok_or(security_error!(
        "Could not find a local EntityHandle for the GUID {:?}",
        guid
      ))
      .copied()
  }

  /// The `local_proxy_guid_pair` should be `&(local_entity_guid, proxy_guid)`.
  fn get_remote_entity_crypto_handle(
    &self,
    (local_entity_guid, proxy_guid): (&GUID, &GUID),
  ) -> SecurityResult<ParticipantCryptoHandle> {
    let local_and_proxy_guid_pair = (*local_entity_guid, *proxy_guid);
    self
      .remote_entity_crypto_handle_cache_
      .get(&local_and_proxy_guid_pair)
      .ok_or(security_error!(
        "Could not find a remote EntityHandle for the (local_entity_guid, proxy_guid) pair {:?}",
        local_and_proxy_guid_pair
      ))
      .copied()
  }
}

/// Interface for using the Authentication plugin
impl SecurityPlugins {
  pub fn validate_local_identity(
    &mut self,
    domain_id: u16,
    participant_qos: &QosPolicies,
    candidate_participant_guid: GUID,
  ) -> SecurityResult<GUID> {
    let (outcome, identity_handle, sec_guid) =
      self
        .auth
        .validate_local_identity(domain_id, participant_qos, candidate_participant_guid)?;

    if let ValidationOutcome::Ok = outcome {
      // Everything OK, store handle and return GUID
      self
        .identity_handle_cache_
        .insert(sec_guid, identity_handle);
      Ok(sec_guid)
    } else {
      // If the builtin authentication does not fail, it should produce only OK
      // outcome. If some other outcome was produced, return an error
      Err(security_error!(
        "Validating local identity produced an unexpected outcome"
      ))
    }
  }

  pub fn get_identity_token(&self, participant_guid: GUID) -> SecurityResult<IdentityToken> {
    let identity_handle = self.get_identity_handle(&participant_guid)?;
    self.auth.get_identity_token(identity_handle)
  }

  pub fn get_identity_status_token(
    &self,
    participant_guid: GUID,
  ) -> SecurityResult<IdentityStatusToken> {
    let identity_handle = self.get_identity_handle(&participant_guid)?;
    self.auth.get_identity_status_token(identity_handle)
  }

  pub fn set_permissions_credential_and_token(
    &self,
    participant_guid: GUID,
    permissions_credential_token: PermissionsCredentialToken,
    permissions_token: PermissionsToken,
  ) -> SecurityResult<()> {
    let handle = self.get_identity_handle(&participant_guid)?;
    self.auth.set_permissions_credential_and_token(
      handle,
      permissions_credential_token,
      permissions_token,
    )
  }
}

/// Interface for using the Access control plugin
impl SecurityPlugins {
  pub fn validate_local_permissions(
    &mut self,
    domain_id: u16,
    participant_guid: GUID,
    participant_qos: &QosPolicies,
  ) -> SecurityResult<()> {
    let identity_handle = self.get_identity_handle(&participant_guid)?;
    let permissions_handle = self.access.validate_local_permissions(
      &*self.auth,
      identity_handle,
      domain_id,
      participant_qos,
    )?;
    self
      .permissions_handle_cache_
      .insert(participant_guid, permissions_handle);
    Ok(())
  }

  pub fn check_create_participant(
    &self,
    domain_id: u16,
    participant_guid: GUID,
    qos: &QosPolicies,
  ) -> SecurityResult<()> {
    let handle = self.get_permissions_handle(&participant_guid)?;
    self.access.check_create_participant(handle, domain_id, qos)
  }

  pub fn get_permissions_token(&self, participant_guid: GUID) -> SecurityResult<PermissionsToken> {
    let handle: PermissionsHandle = self.get_permissions_handle(&participant_guid)?;
    self.access.get_permissions_token(handle)
  }

  pub fn get_permissions_credential_token(
    &self,
    participant_guid: GUID,
  ) -> SecurityResult<PermissionsCredentialToken> {
    let handle: PermissionsHandle = self.get_permissions_handle(&participant_guid)?;
    self.access.get_permissions_credential_token(handle)
  }

  pub fn get_participant_sec_attributes(
    &self,
    participant_guid: GUID,
  ) -> SecurityResult<ParticipantSecurityAttributes> {
    let handle: PermissionsHandle = self.get_permissions_handle(&participant_guid)?;
    self.access.get_participant_sec_attributes(handle)
  }
}

/// Interface for using the CryptoKeyTransform of the Cryptographic plugin
impl SecurityPlugins {
  pub fn encode_datawriter_submessage(
    &self,
    plain_submessage: Submessage,
    source_guid: &GUID,
    destination_guid_list: &[GUID],
  ) -> SecurityResult<EncodedSubmessage> {
    // Convert the destination GUIDs to handles
    let mut receiving_datareader_crypto_list: Vec<DatareaderCryptoHandle> =
      SecurityResult::from_iter(destination_guid_list.iter().map(|destination_guid| {
        self.get_remote_entity_crypto_handle((source_guid, destination_guid))
      }))?;
    // Remove duplicates
    receiving_datareader_crypto_list.sort();
    receiving_datareader_crypto_list.dedup();

    self.crypto.encode_datawriter_submessage(
      plain_submessage,
      self.get_local_entity_crypto_handle(source_guid)?,
      receiving_datareader_crypto_list,
    )
  }

  pub fn encode_datareader_submessage(
    &self,
    plain_submessage: Submessage,
    source_guid: &GUID,
    destination_guid_list: &[GUID],
  ) -> SecurityResult<EncodedSubmessage> {
    // Convert the destination GUIDs to handles
    let mut receiving_datawriter_crypto_list: Vec<DatawriterCryptoHandle> =
      SecurityResult::from_iter(destination_guid_list.iter().map(|destination_guid| {
        self.get_remote_entity_crypto_handle((source_guid, destination_guid))
      }))?;
    // Remove duplicates
    receiving_datawriter_crypto_list.sort();
    receiving_datawriter_crypto_list.dedup();

    self.crypto.encode_datareader_submessage(
      plain_submessage,
      self.get_local_entity_crypto_handle(source_guid)?,
      receiving_datawriter_crypto_list,
    )
  }

  pub fn encode_message(
    &self,
    plain_message: Message,
    source_guid_prefix: &GuidPrefix,
    destination_guid_prefix_list: &[GuidPrefix],
  ) -> SecurityResult<Message> {
    // Convert the destination GUID prefixes to handles
    let mut receiving_datawriter_crypto_list: Vec<DatawriterCryptoHandle> =
      SecurityResult::from_iter(destination_guid_prefix_list.iter().map(
        |destination_guid_prefix| self.get_participant_crypto_handle(destination_guid_prefix),
      ))?;
    // Remove duplicates
    receiving_datawriter_crypto_list.sort();
    receiving_datawriter_crypto_list.dedup();

    self.crypto.encode_rtps_message(
      plain_message,
      self.get_participant_crypto_handle(source_guid_prefix)?,
      receiving_datawriter_crypto_list,
    )
  }

  pub fn decode_rtps_message(
    &self,
    encoded_message: Message,
    source_guid_prefix: &GuidPrefix,
    destination_guid_prefix: &GuidPrefix,
  ) -> SecurityResult<Message> {
    self.crypto.decode_rtps_message(
      encoded_message,
      self.get_participant_crypto_handle(destination_guid_prefix)?,
      self.get_participant_crypto_handle(source_guid_prefix)?,
    )
  }

  pub fn preprocess_secure_submessage(
    &self,
    secure_prefix: &SecurePrefix,
    source_guid_prefix: &GuidPrefix,
    destination_guid_prefix: &GuidPrefix,
  ) -> SecurityResult<SecureSubmessageCategory> {
    self.crypto.preprocess_secure_submsg(
      secure_prefix,
      self.get_participant_crypto_handle(destination_guid_prefix)?,
      self.get_participant_crypto_handle(source_guid_prefix)?,
    )
  }

  pub fn decode_datawriter_submessage(
    &self,
    encoded_rtps_submessage: (SecurePrefix, Submessage, SecurePostfix),
    receiving_datareader_crypto: DatareaderCryptoHandle,
    sending_datawriter_crypto: DatawriterCryptoHandle,
  ) -> SecurityResult<WriterSubmessage> {
    self.crypto.decode_datawriter_submessage(
      encoded_rtps_submessage,
      receiving_datareader_crypto,
      sending_datawriter_crypto,
    )
  }

  pub fn decode_datareader_submessage(
    &self,
    encoded_rtps_submessage: (SecurePrefix, Submessage, SecurePostfix),
    receiving_datawriter_crypto: DatawriterCryptoHandle,
    sending_datareader_crypto: DatareaderCryptoHandle,
  ) -> SecurityResult<ReaderSubmessage> {
    self.crypto.decode_datareader_submessage(
      encoded_rtps_submessage,
      receiving_datawriter_crypto,
      sending_datareader_crypto,
    )
  }
}

#[derive(Clone)]
pub(crate) struct SecurityPluginsHandle {
  inner: Arc<Mutex<SecurityPlugins>>,
}

impl SecurityPluginsHandle {
  pub(crate) fn new(s: SecurityPlugins) -> Self {
    Self {
      inner: Arc::new(Mutex::new(s)),
    }
  }
}

impl fmt::Debug for SecurityPluginsHandle {
  fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
    f.write_str("SecurityPluginsHandle")
  }
}

impl std::ops::Deref for SecurityPluginsHandle {
  type Target = Mutex<SecurityPlugins>;
  fn deref(&self) -> &Self::Target {
    &self.inner
  }
}
