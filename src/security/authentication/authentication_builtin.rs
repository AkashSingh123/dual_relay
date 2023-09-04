use std::collections::HashMap;

use bytes::Bytes;
use x509_certificate::{signing::InMemorySigningKeyPair, EcdsaCurve, KeyAlgorithm};

use crate::{
  security::{certificate, SecurityError, SecurityResult},
  security_error,
  structure::guid::GuidPrefix,
  GUID,
};
use super::{
  authentication_builtin::types::BuiltinIdentityToken, Challenge, HandshakeHandle,
  HandshakeMessageToken, IdentityHandle, IdentityToken, Sha256, SharedSecret, ValidationOutcome,
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
    challenge1: Challenge,       // 256-bit nonce
    hash_c1: Sha256,             // To avoid recomputing this on receiving reply
  },

  // We have sent a handshake reply message and are waiting for the
  // final message
  PendingFinalMessage {
    dh1: Bytes,                  // only public part of dh1
    challenge1: Challenge,       // 256-bit nonce
    dh2: InMemorySigningKeyPair, // both public and private keys for dh2
    challenge2: Challenge,       // 256-bit nonce
  },

  // Handshake was completed & we sent the final message. If
  // requested again, we need to resend the message
  CompletedWithFinalMessageSent {
    dh1: InMemorySigningKeyPair, // both public and private keys for dh1
    challenge1: Challenge,       // 256-bit nonce
    dh2: Bytes,
    challenge2: Challenge, //256-bit nonce
    shared_secret: SharedSecret,
  },

  // Handshake was completed & we received the final
  // message. Nothing to do for us anymore.
  CompletedWithFinalMessageReceived {
    dh1: Bytes,                  // only public part of dh1
    challenge1: Challenge,       // 256-bit nonce
    dh2: InMemorySigningKeyPair, // both public and private keys for dh2
    challenge2: Challenge,       // 256-bit nonce
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

// TODO: This struct layer is redundant. Remove and replace with
// BuiltinHandshakeState
struct HandshakeInfo {
  state: BuiltinHandshakeState,
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

  mock_handshakes: bool, // Mock handshakes for testing? Temporary field, for development only
}

impl AuthenticationBuiltin {
  pub fn new() -> Self {
    Self {
      local_participant_info: None, // No info yet
      remote_participant_infos: HashMap::new(),
      handshake_to_identity_handle_map: HashMap::new(),
      next_identity_handle: 0,
      next_handshake_handle: 0,
      mock_handshakes: false,
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

  #[allow(clippy::needless_pass_by_value)]
  fn begin_handshake_request_mocked(
    &mut self,
    initiator_identity_handle: IdentityHandle, // Local
    replier_identity_handle: IdentityHandle,   // Remote
    serialized_local_participant_data: Vec<u8>,
  ) -> SecurityResult<(ValidationOutcome, HandshakeHandle, HandshakeMessageToken)> {
    // Make sure initiator_identity_handle is actually ours
    let local_info = self.get_local_participant_info()?;
    if initiator_identity_handle != local_info.identity_handle {
      return Err(security_error!(
        "The parameter initiator_identity_handle is not the correct local handle"
      ));
    }

    // Make sure we are expecting to send the authentication request message
    let remote_info = self.get_remote_participant_info_mutable(&replier_identity_handle)?;
    if let BuiltinHandshakeState::PendingRequestSend = remote_info.handshake.state {
      // Yes, this is what we expect. No action here.
    } else {
      return Err(security_error!(
        "We are not expecting to send a handshake request. Handshake state: {:?}",
        remote_info.handshake.state
      ));
    }

    // Construct the handshake request message token
    let handshake_request = HandshakeMessageToken::dummy();

    let (dh1_key_pair, _keypair_pkcs8) =
      InMemorySigningKeyPair::generate_random(KeyAlgorithm::Ecdsa(EcdsaCurve::Secp256r1))?;

    // Change handshake state to pending reply message & save the request token
    remote_info.handshake.state = BuiltinHandshakeState::PendingReplyMessage {
      dh1: dh1_key_pair,
      challenge1: Challenge::dummy(),
      hash_c1: Sha256::dummy(),
    };

    // Create a new handshake handle & map it to remotes identity handle
    let new_handshake_handle = self.get_new_handshake_handle();
    self
      .handshake_to_identity_handle_map
      .insert(new_handshake_handle, replier_identity_handle);

    Ok((
      ValidationOutcome::PendingHandshakeMessage,
      new_handshake_handle,
      handshake_request,
    ))
  }

  #[allow(clippy::needless_pass_by_value)]
  fn begin_handshake_reply_mocked(
    &mut self,
    handshake_message_in: HandshakeMessageToken,
    initiator_identity_handle: IdentityHandle, // Remote
    replier_identity_handle: IdentityHandle,   // Local
    serialized_local_participant_data: Vec<u8>,
  ) -> SecurityResult<(ValidationOutcome, HandshakeHandle, HandshakeMessageToken)> {
    // Make sure replier_identity_handle is actually ours
    let local_info = self.get_local_participant_info()?;
    if replier_identity_handle != local_info.identity_handle {
      return Err(security_error!(
        "The parameter replier_identity_handle is not the correct local handle"
      ));
    }

    // Make sure we are expecting a authentication request from remote
    let remote_info = self.get_remote_participant_info_mutable(&initiator_identity_handle)?;
    if let BuiltinHandshakeState::PendingRequestMessage = remote_info.handshake.state {
      // Nothing to see here. Carry on.
    } else {
      return Err(security_error!(
        "We are not expecting to receive a handshake request. Handshake state: {:?}",
        remote_info.handshake.state
      ));
    }

    // Generate a reply token
    let reply_token = HandshakeMessageToken::dummy();

    let (dh2_key_pair, _keypair_pkcs8) =
      InMemorySigningKeyPair::generate_random(KeyAlgorithm::Ecdsa(EcdsaCurve::Secp256r1))?;

    // Change handshake state to pending final message
    remote_info.handshake.state = BuiltinHandshakeState::PendingFinalMessage {
      dh1: Bytes::default(),
      challenge1: Challenge::dummy(),
      dh2: dh2_key_pair,
      challenge2: Challenge::dummy(),
    };

    // Create a new handshake handle & map it to remotes identity handle
    let new_handshake_handle = self.get_new_handshake_handle();
    self
      .handshake_to_identity_handle_map
      .insert(new_handshake_handle, initiator_identity_handle);

    Ok((
      ValidationOutcome::PendingHandshakeMessage,
      new_handshake_handle,
      reply_token,
    ))
  }

  #[allow(clippy::needless_pass_by_value)]
  fn process_handshake_mocked(
    &mut self,
    handshake_message_in: HandshakeMessageToken,
    handshake_handle: HandshakeHandle,
  ) -> SecurityResult<(ValidationOutcome, Option<HandshakeMessageToken>)> {
    // Check what is the handshake state
    let remote_identity_handle = *self.handshake_handle_to_identity_handle(&handshake_handle)?;
    let remote_info = self.get_remote_participant_info_mutable(&remote_identity_handle)?;

    let mut state = BuiltinHandshakeState::PendingRequestSend; // dummy to leave behind
    std::mem::swap(&mut remote_info.handshake.state, &mut state);

    match state {
      BuiltinHandshakeState::PendingReplyMessage {
        dh1,
        challenge1,
        hash_c1,
      } => {
        // We are the initiator, and expect a reply.
        // Result is that we produce a MassageToken (i.e. send the final message)
        // and the handshake results (shared secret)
        let final_message_token = HandshakeMessageToken::dummy();

        // Generate new, random Diffie-Hellman key pair "dh2"
        let dh2 = Bytes::default();
        let shared_secret = SharedSecret::dummy();

        // This is an initiator-generated 256-bit nonce
        let challenge2 = Challenge::from(rand::random::<[u8; 32]>());

        // Change handshake state to Completed & save the final message token
        let remote_info = self.get_remote_participant_info_mutable(&remote_identity_handle)?;
        remote_info.handshake.state = BuiltinHandshakeState::CompletedWithFinalMessageSent {
          dh1,
          dh2,
          challenge1,
          challenge2,
          shared_secret,
        };
        Ok((ValidationOutcome::OkFinalMessage, Some(final_message_token)))
      }
      BuiltinHandshakeState::PendingFinalMessage {
        dh1,
        dh2,
        challenge1,
        challenge2,
      } => {
        // We are the responder, and expect the final message.
        // Result is that we do not produce a MassageToken, since this was the final
        // message, but we compute the handshake results (shared secret)

        // Change handshake state to Completed
        let shared_secret = SharedSecret::dummy();
        let remote_info = self.get_remote_participant_info_mutable(&remote_identity_handle)?;
        remote_info.handshake.state = BuiltinHandshakeState::CompletedWithFinalMessageReceived {
          dh1,
          dh2,
          challenge1,
          challenge2,
          shared_secret,
        };

        Ok((ValidationOutcome::Ok, None))
      }
      other_state => Err(security_error!(
        "Unexpected handshake state: {:?}",
        other_state
      )),
    }
  }
}
