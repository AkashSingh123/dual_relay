use serde::{Deserialize, Serialize};

type ConfigError = serde_xml_rs::Error;

// A list of Grants
#[derive(Debug, Clone)]
pub struct DomainParticiapntPermissions {
  pub grants: Vec<Grant>,
}

impl DomainParticiapntPermissions {
  pub fn find_grant(
    &self,
    subject_name: String,
    current_datetime: &chrono::DateTime<chrono::Utc>,
  ) -> Option<&Grant> {
    // TODO: How to match subject names?
    self
      .grants
      .iter()
      .find(|g| g.subject_name == subject_name && g.validity.contains(current_datetime))
  }

  pub fn from_xml(domain_participant_permissions_xml: &str) -> Result<Self, ConfigError> {
    let dpp: xml::DomainParticiapntPermissionsDocument =
      serde_xml_rs::from_str(domain_participant_permissions_xml)?;
    let grants = dpp
      .permissions
      .grants
      .iter()
      .map(Grant::from_xml)
      .collect::<Result<Vec<Grant>, serde_xml_rs::Error>>()?;
    Ok(Self { grants })
  }
}

// A Grant is a set of permissions for a particular DomainParticipant, which
// is identified as a X.509 subject.
// The permissions allow or deny the DP to publish, subscribe, or relay messages
// on particular DDS topics or DDS partitions.
//
// To check if a DP is allowd to e.g. publish to a particular topic, the list of
// Grants is scanned in order. A matching grant (subject name and validity
// range) must be found. If not, then there is no permission.
// If a matching Grant is found, the Rules are scanned in order. The Rules is
// matched against the proposed action. The domain id and all the
// action-specific (e.g. publish) Criteria must match before the rule can be
// applied. If a matching rule is found, then its verdict is applied.
// If no applicable rule exists, then the verdict is default_action.
#[derive(Debug, Clone)]
pub struct Grant {
  pub subject_name: String, // X.509 subject name
  pub validity: std::ops::Range<chrono::DateTime<chrono::Utc>>,
  pub rules: Vec<Rule>,
  pub default_action: AllowOrDeny,
}

impl Grant {
  pub fn check_action<'a>(
    &self,
    action: Action,
    domain_id: u16,
    topic_name: &'a str,
    partitions: &'a [&str],
    data_tags: &'a [(&str, &str)],
  ) -> AllowOrDeny {
    self
      .rules
      .iter()
      .find(|rule| rule.is_applicable(action, domain_id, topic_name, partitions, data_tags))
      .map(|rule| rule.verdict)
      .unwrap_or(self.default_action)
  }

  fn from_xml(xgrant: &xml::Grant) -> Result<Self, ConfigError> {
    todo!()
    //   let subj_names = xgrant.elems.filter_map(|ge|
    // Self::subject_name_from_xml(ge) );   let validity =
    // xgrant.elems.filter_map(|ge| Self::validity_from_xml(ge) );
    //   let allows = xgrant.elems.filter_map(|ge|
    // Self::allow_rule_from_xml(ge));   let denys =
    // xgrant.elems.filter_map(|ge| Self::deny_rule_from_xml(ge));
    //   let defaults = xgrant.elems.filter_map(|ge|
    // Self::default_from_xml(ge));

    //   Ok(Self {
    //     subject_name,
    //     validity,
    //     rules,
    //     default_action,
    //   })
  }

  // fn subject_name_from_xml()
}

#[derive(Debug, Clone)]
pub struct Rule {
  pub verdict: AllowOrDeny,
  pub domains: Vec<DomainIds>, // must not be empty
  pub publish: Vec<Criterion>,
  pub subscribe: Vec<Criterion>,
  pub relay: Vec<Criterion>,
}

#[derive(Debug, Clone, Copy)]
pub enum Action {
  Publish,
  Subscribe,
  Relay,
}

impl Rule {
  pub fn is_applicable<'a>(
    &self,
    action: Action,
    domain_id: u16,
    topic_name: &'a str,
    partitions: &'a [&str],
    data_tags: &'a [(&str, &str)],
  ) -> bool {
    debug_assert!(!self.domains.is_empty());

    if self.domains.iter().any(|d| d.check(domain_id)) {
      // Rule applies to this domain
      let verdict = self.verdict;
      let criteria = match action {
        Action::Publish => &self.publish,
        Action::Subscribe => &self.subscribe,
        Action::Relay => &self.relay,
      };
      if criteria
        .iter()
        .any(|c| c.is_applicable(topic_name, partitions.iter(), data_tags.iter()))
      {
        true
      } else {
        false // No criterion matches, no result
      }
    } else {
      false // Did not apply to this domain, no result
    }
  }
}

#[derive(Debug, Clone, Copy)]
pub enum AllowOrDeny {
  Allow,
  Deny,
}

impl AllowOrDeny {
  fn from_xml(x: xml::DefaultAction) -> Self {
    match x {
      xml::DefaultAction::Allow => AllowOrDeny::Allow,
      xml::DefaultAction::Deny => AllowOrDeny::Deny,
    }
  }
}

impl From<AllowOrDeny> for bool {
  fn from(a: AllowOrDeny) -> bool {
    match a {
      AllowOrDeny::Allow => true,
      AllowOrDeny::Deny => false,
    }
  }
}

#[derive(Debug, Clone)]
pub struct Criterion {
  topics: Vec<Glob>, /* This Vec must not be empty. Match occurs when any Glob matches the topic
                      * name. */

  partitions: Vec<Glob>, // Match occurs when any Glob matches the partition name.
  // If the Vec is empty, then the default "empty string" partition is assumed. This means that
  // only the "empty string" partition will match.
  // DDS Security spec defines two matching behaviours in case of publishing (subscribing)
  // to multiple partitions. "Default" behaviour requires all partitions to match, and
  // the "legacy" behaviour requires only some partitions to match.
  data_tags: Vec<DataTag>, /* Match condition: All the data tags associated with a a DDS Entity
                            * must match an element of this vector. (But other, unmatched,
                            * DataTags in the Vec are ok.)
                            * consequently, if this Vec is empty, only Entities with no data
                            * tags will match. There is no
                            * `fnmatch()` here. */
}

impl Criterion {
  pub fn is_applicable<'a>(
    &self,
    topic_name: &'a str,
    mut partitions: impl Iterator<Item = &'a &'a str>,
    mut data_tags: impl Iterator<Item = &'a (&'a str, &'a str)>,
  ) -> bool {
    debug_assert!(!self.topics.is_empty());

    self.topics.iter().any(|glob| glob.check(topic_name))
      && partitions.all(|p| self.partitions.iter().any(|glob| glob.check(p)))
      && data_tags.all(|(name, value)| self.data_tags.iter().any(|dt| dt.check(name, value)))
  }

  fn from_xml(xc: &xml::Criteria) -> Result<Self, ConfigError> {
    let contents: (Vec<Glob>, Vec<Glob>, Vec<DataTag>) = xc.members.iter().fold(
      (Vec::new(), Vec::new(), Vec::new()),
      |mut acc, cr| match cr {
        xml::Criterion::Topics(te_list) => {
          let topics = te_list.members.iter().map(|te| Glob::new(&te.value));
          acc.0.extend(topics);
          acc
        }
        xml::Criterion::Partitions(pe_list) => {
          let partitions = pe_list.members.iter().map(|pe| Glob::new(&pe.value));
          acc.1.extend(partitions);
          acc
        }
        xml::Criterion::DataTags(dt_list) => {
          let dts = dt_list
            .members
            .iter()
            .map(|dt| DataTag::new(&dt.name, &dt.value));
          acc.2.extend(dts);
          acc
        }
      },
    );
    let (topics, partitions, data_tags) = contents;

    if topics.len() < 1 {
      return Err(ConfigError::Custom {
        field: "Grant Criterion must define at least a Topic name.".to_string(),
      });
    }

    Ok(Criterion {
      topics,
      partitions,
      data_tags,
    })
  }
}

#[derive(Debug, Clone)]
pub struct Glob {
  glob: String, // Placeholder. This is a POSIX `fnmatch()` "glob" pattern.
}

impl Glob {
  fn check(&self, s: &str) -> bool {
    // TODO: Implement fnmatch matching
    // The glob sanity (syntax) checking should be performed at Glob construction
    s == &self.glob
  }

  fn new(s: &str) -> Self {
    Glob {
      glob: s.to_string(),
    }
  }
}

#[derive(Debug, Clone)]
pub struct DataTag {
  name: String,
  value: String,
}

impl DataTag {
  fn check(&self, name: &str, value: &str) -> bool {
    name == &self.name && value == &self.value
  }

  fn new(name: &str, value: &str) -> Self {
    DataTag {
      name: name.to_string(),
      value: value.to_string(),
    }
  }
}

#[derive(Debug, Clone, Copy)]
pub enum DomainIds {
  Value(u16),
  Range(u16, u16),
  Min(u16),
  Max(u16),
}

impl DomainIds {
  pub fn from_id(i: u16) -> DomainIds {
    DomainIds::Value(i)
  }
  pub fn from_range(min: u16, max: u16) -> DomainIds {
    DomainIds::Range(min, max)
  }
  pub fn from_min(min: u16) -> DomainIds {
    DomainIds::Min(min)
  }
  pub fn from_max(max: u16) -> DomainIds {
    DomainIds::Max(max)
  }
  pub fn check(&self, i: u16) -> bool {
    match self {
      DomainIds::Value(v) => *v == i,
      DomainIds::Range(mi, ma) => *mi <= i && i <= *ma,
      DomainIds::Min(mi) => *mi <= i,
      DomainIds::Max(ma) => i <= *ma,
    }
  }

  fn from_xml(xd: &xml::DomainIdSetMember) -> Result<Self, ConfigError> {
    match xd {
      xml::DomainIdSetMember::DomainId(xml::DomainId { id }) => Ok(DomainIds::Value(*id)),
      xml::DomainIdSetMember::DomainIdRange(xml::DomainIdRange { min, max }) => match (min, max) {
        (Some(min), Some(max)) => Ok(DomainIds::Range(min.id, max.id)),
        (None, Some(max)) => Ok(DomainIds::Max(max.id)),
        (Some(min), None) => Ok(DomainIds::Min(min.id)),
        (None, None) => Err(ConfigError::Custom {
          field: "Domain id range must have at leat one bound".to_string(),
        }),
      },
    }
  } // fn
}

mod xml {
  use serde::{Deserialize, Serialize};

  // Define structs to mirror the XML Schema given in
  // DDS Security Spec v1.1 Section
  // "9.4.1.3 DomainParticipant permissions document"

  // TODO: Allow Boolean literals also in all uppercase, e.g. "TRUE" in addition
  // to "true".

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename = "dds")]
  pub struct DomainParticiapntPermissionsDocument {
    pub permissions: Permissions,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct Permissions {
    #[serde(rename = "$value")]
    pub grants: Vec<Grant>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename = "grant")]
  pub struct Grant {
    #[serde(rename = "$value")]
    pub elems: Vec<GrantElement>,
    //TODO: This is a hacky way to get serde-xml to read the XML as specified.
    // We need to manually check that there is (exactly) one of each SubjectName, Validity, and
    // Default in a Grant.
    // There may be an arbitray number of AllowRules and DenyRules, and their order is important.
    // The AllowRules and DenyRules are to scanned in order until one matches, and that is to be
    // applied. if there is no match, then use Default.
    //
    // See Section "9.4.1.3.2.3 Rules Section" in DDS Security Spec v1.1
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename_all = "snake_case")]
  pub enum GrantElement {
    SubjectName(String), // This is a X.509 subject name, so may need further parsing
    Validity(Validity),
    AllowRule(Rule),
    DenyRule(Rule),
    Default(DefaultAction),
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct Validity {
    not_before: String, // XsdDateTime,
    not_after: String,  // XsdDateTime,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct Rule {
    #[serde(rename = "$value")]
    pub elems: Vec<RuleElement>, /*TODO: This is a hacky way to get serde-xml to read the XML
                                  * as specified. We need to
                                  * manually check that there is (exactly) one of `domain`
                                  * in a Rule, breferably at the begining. */
  }

  // The RuleElements should be in order Publish, Subscribe, Relay, witch 0..N
  // occurencences of each. This definition accepts them in any order.

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename_all = "snake_case")]
  pub enum RuleElement {
    Domains(DomainIdSet),
    Publish(Criteria),
    Subscribe(Criteria),
    Relay(Criteria),
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct DomainIdSet {
    #[serde(rename = "$value")]
    pub members: Vec<DomainIdSetMember>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub enum DomainIdSetMember {
    #[serde(rename = "id")]
    DomainId(DomainId),
    #[serde(rename = "id_range")]
    DomainIdRange(DomainIdRange),
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct DomainId {
    #[serde(rename = "$value")]
    pub id: u16,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct DomainIdRange {
    pub min: Option<DomainId>, // At least one of these must be defined.
    pub max: Option<DomainId>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct Criteria {
    #[serde(rename = "$value")]
    pub members: Vec<Criterion>, /* must not be empty: must have at least 1 topic criterion
                                  * specified */
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename_all = "snake_case")]
  pub enum Criterion {
    Topics(TopicExpressionList),
    Partitions(PartitionExpressionList),
    DataTags(DataTags),
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct TopicExpressionList {
    #[serde(rename = "$value")]
    pub members: Vec<TopicExpression>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct TopicExpression {
    #[serde(rename = "$value")]
    pub value: String,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct PartitionExpressionList {
    #[serde(rename = "$value")]
    pub members: Vec<PartitionExpression>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct PartitionExpression {
    #[serde(rename = "$value")]
    pub value: String,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  pub struct DataTags {
    #[serde(rename = "$value")]
    pub members: Vec<DataTag>,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename = "tag")]
  pub struct DataTag {
    pub name: String,
    pub value: String,
  }

  #[derive(Debug, Serialize, Deserialize, PartialEq)]
  #[serde(rename_all = "SCREAMING_SNAKE_CASE")]
  pub enum DefaultAction {
    Allow,
    Deny,
  }

  #[cfg(test)]
  mod tests {
    use serde_xml_rs::from_str;

    use super::*;

    #[test]
    pub fn parse_spec_example() {
      // Modifications to example in spec:
      // * insert missing "/" in closing id_range
      // * Boolean literals true/false in all lowercase
      // * field `enable_liveliness_protection` is systematically missing from
      //   `topic_rule`s

      let domain_governance_document = r#"<?xml version="1.0" encoding="UTF-8"?>
  <dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS-Security/20170801/omg_shared_ca_permissions.xsd">
    <permissions>
      <grant name="ShapesPermission">
        <subject_name>emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US</subject_name>
        <validity>
          <!-- Format is CCYY-MM-DDThh:mm:ss[Z|(+|-)hh:mm] The time zone may
          be specified as Z (UTC) or (+|-)hh:mm. Time zones that aren't
          specified are considered UTC.
          -->
          <not_before>2013-10-26T00:00:00</not_before>
          <not_after>2018-10-26T22:45:30</not_after>
        </validity>

        <allow_rule>
          <domains>
          <id>0</id>
          </domains>
          <!-- DDSSEC11-56 - deleted invalid elements -->
        </allow_rule>

        <deny_rule>
          <domains>
            <id>0</id>
          </domains>
          <publish>
            <topics>
            <topic>Circle1</topic>
            </topics>
          </publish>
          <publish>
            <topics>
            <topic>Square</topic>
            </topics>
            <partitions>
            <partition>A_partition</partition>
            </partitions>
          </publish>
          <subscribe>
            <topics>
            <topic>Square1</topic>
            </topics>
          </subscribe>
          <subscribe>
            <topics>
            <topic>Tr*</topic>
            </topics>
            <partitions>
            <partition>P1*</partition>
            </partitions>
          </subscribe>
        </deny_rule>

        <allow_rule>
          <domains>
          <id>0</id>
          </domains>
          <publish>
          <topics>
          <topic>Cir*</topic>
          </topics>
          <data_tags>
          <tag>
          <name>aTagName1</name>
          <value>aTagValue1</value>
          </tag>
          </data_tags>
          </publish>
          <subscribe>
          <topics>
          <topic>Sq*</topic>
          </topics>
          <data_tags>
          <tag>
          <name>aTagName1</name>
          <value>aTagValue1</value>
          </tag>
          <tag>
          <name>aTagName2</name>
          <value>aTagValue2</value>
          </tag>
          </data_tags>
          </subscribe>
          <subscribe>
          <topics>
          <topic>Triangle</topic>
          </topics>
          <partitions>
          <partition>P*</partition>
          </partitions>
          <data_tags>
          <tag>
          <name>aTagName1</name>
          <value>aTagValue1</value>
          </tag>
          </data_tags>
          </subscribe>
          <relay>
          <topics>
          <topic>*</topic>
          </topics>
          <partitions>
          <partition>aPartitionName</partition>
          </partitions>
          </relay>
        </allow_rule>

        <default>DENY</default>

      </grant>
    </permissions>
  </dds>
  "#;

      let dgd: DomainParticiapntPermissionsDocument = from_str(domain_governance_document).unwrap();
    }

    #[test]
    pub fn parse_minimal() {
      let domain_governance_document = r#"<?xml version="1.0" encoding="UTF-8"?>
  <dds xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xsi:noNamespaceSchemaLocation="http://www.omg.org/spec/DDS-Security/20170801/omg_shared_ca_permissions.xsd">
    <permissions>
      <grant name="ShapesPermission">
        <subject_name>emailAddress=cto@acme.com, CN=DDS Shapes Demo, OU=CTO Office, O=ACME Inc., L=Sunnyvale, ST=CA, C=US</subject_name>
        <validity>
          <not_before>2013-10-26T00:00:00</not_before>
          <not_after>2018-10-26T22:45:30</not_after>
        </validity>

        <allow_rule>
          <domains><id> 0 </id></domains>
        </allow_rule>

        <default>DENY</default>

      </grant>
    </permissions>
  </dds>
  "#;

      let dgd: DomainParticiapntPermissionsDocument = from_str(domain_governance_document).unwrap();
    }
  }
}
