use std::collections::HashMap;

use bytes::Bytes;
use x509_certificate::signing::InMemorySigningKeyPair;

use crate::{
  security::{certificate, SecurityError, SecurityResult},
  security_error,
  structure::guid::GuidPrefix,
  GUID,
};
use super::{
  authentication_builtin::types::BuiltinIdentityToken, HandshakeHandle, IdentityHandle,
  IdentityToken, SharedSecret,
};

mod authentication;
pub(in crate::security) mod types;

// States for an ongoing handshake with a remote participant. Used by the plugin
// internally. Note that there is no 'failed' state, since once a handshake has
// started, it doesn't terminate if some step fails. Instead, it just doesn't
// advance to the next step.
#[derive(Debug)]
pub(crate) enum BuiltinHandshakeState {
  PendingRequestSend,    // We need to create & send the handshake request
  PendingRequestMessage, // We are waiting for a handshake request from remote participant
  PendingReplyMessage {
    // We have sent a handshake request and are waiting for a reply
    dh1: InMemorySigningKeyPair, // both public and private keys for dh1
    challenge1: [u8; 32],        // 256-bit nonce
  },

  // We have sent a handshake reply message and are waiting for the
  // final message
  PendingFinalMessage {
    dh1: Bytes,                  // only public part of dh1
    challenge1: [u8; 32],        // 256-bit nonce
    dh2: InMemorySigningKeyPair, // both public and private keys for dh2
    challenge2: [u8; 32],        // 256-bit nonce
  },

  // Handshake was completed & we sent the final message. If
  // requested again, we need to resend the message
  CompletedWithFinalMessageSent {
    dh1: InMemorySigningKeyPair, // both public and private keys for dh1
    challenge1: [u8; 32],        // 256-bit nonce
    dh2: Bytes,
    challenge2: [u8; 32], //256-bit nonce
    shared_secret: SharedSecret,
  },

  // Handshake was completed & we received the final
  // message. Nothing to do for us anymore.
  CompletedWithFinalMessageReceived {
    dh1: Bytes,                  // only public part of dh1
    challenge1: [u8; 32],        // 256-bit nonce
    dh2: InMemorySigningKeyPair, // both public and private keys for dh2
    challenge2: [u8; 32],        // 256-bit nonce
    shared_secret: SharedSecret,
  },
}

// This is a mirror of the above states, but with no data carried from
// one state to another. This is for use in secure Discovery.
// TODO: Refactor (how?) to not need to separate types for this.
#[derive(Clone, Copy, PartialEq, Debug)]
pub(crate) enum DiscHandshakeState {
  PendingRequestSend,
  PendingRequestMessage,
  PendingReplyMessage,
  PendingFinalMessage,
  CompletedWithFinalMessageSent,
  CompletedWithFinalMessageReceived,
}

struct LocalParticipantInfo {
  identity_handle: IdentityHandle,
  identity_token: BuiltinIdentityToken,
  guid: GUID,
  id_cert_private_key: certificate::PrivateKey, // PrivateKey is actually (private,public) key pair
  identity_certificate: certificate::Certificate, // Certificate contains the public key also
  identity_ca: certificate::Certificate,        /* Certification Authority who has signed
                                                 * identity_certificate */
  permissions_document_xml: Bytes, // We do not care about UTF-8:ness anymore
}

// All things about remote participant that we're interested in
struct RemoteParticipantInfo {
  identity_token: IdentityToken,
  guid_prefix: GuidPrefix,
  handshake: HandshakeInfo,
}

struct HandshakeInfo {
  state: BuiltinHandshakeState,
  // latest_sent_message: Option<HandshakeMessageToken>,
  // my_dh_keys: Option<InMemorySigningKeyPair>, // This is dh1 or dh2, whichever we generated
  // ourself challenge1: Option<Bytes>,
  // challenge2: Option<Bytes>,
  // shared_secret: Option<SharedSecret>,
}

// A struct implementing the builtin Authentication plugin
// See sections 8.3 and 9.3 of the Security specification (v. 1.1)
pub struct AuthenticationBuiltin {
  local_participant_info: Option<LocalParticipantInfo>,
  remote_participant_infos: HashMap<IdentityHandle, RemoteParticipantInfo>,
  // handshake_to_identity_handle maps handshake handles to identity handles.
  handshake_to_identity_handle_map: HashMap<HandshakeHandle, IdentityHandle>,

  next_identity_handle: IdentityHandle,
  next_handshake_handle: HandshakeHandle,
}

impl AuthenticationBuiltin {
  pub fn new() -> Self {
    Self {
      local_participant_info: None, // No info yet
      remote_participant_infos: HashMap::new(),
      handshake_to_identity_handle_map: HashMap::new(),
      next_identity_handle: 0,
      next_handshake_handle: 0,
    }
  }

  fn get_new_identity_handle(&mut self) -> IdentityHandle {
    let new_handle = self.next_identity_handle;
    self.next_identity_handle += 1;
    new_handle
  }

  fn get_new_handshake_handle(&mut self) -> HandshakeHandle {
    let new_handle = self.next_handshake_handle;
    self.next_handshake_handle += 1;
    new_handle
  }

  fn get_local_participant_info(&self) -> SecurityResult<&LocalParticipantInfo> {
    self.local_participant_info.as_ref().ok_or_else(|| {
      security_error!("Local participant info not found. Has the local identity been validated?")
    })
  }

  fn get_local_participant_info_mutable(&mut self) -> SecurityResult<&mut LocalParticipantInfo> {
    self.local_participant_info.as_mut().ok_or_else(|| {
      security_error!("Local participant info not found. Has the local identity been validated?")
    })
  }

  // Returns immutable info
  fn get_remote_participant_info(
    &self,
    identity_handle: &IdentityHandle,
  ) -> SecurityResult<&RemoteParticipantInfo> {
    self
      .remote_participant_infos
      .get(identity_handle)
      .ok_or_else(|| security_error!("Remote participant info not found"))
  }

  // Returns mutable info
  fn get_remote_participant_info_mutable(
    &mut self,
    identity_handle: &IdentityHandle,
  ) -> SecurityResult<&mut RemoteParticipantInfo> {
    self
      .remote_participant_infos
      .get_mut(identity_handle)
      .ok_or_else(|| security_error!("Remote participant info not found"))
  }

  fn handshake_handle_to_identity_handle(
    &self,
    hs_handle: &HandshakeHandle,
  ) -> SecurityResult<&IdentityHandle> {
    self
      .handshake_to_identity_handle_map
      .get(hs_handle)
      .ok_or_else(|| security_error!("Identity handle not found with handshake handle"))
  }
}
