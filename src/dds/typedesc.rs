// TODO: rename this module to e.g. samples


#[derive(Clone, PartialEq, Eq)]
pub struct TypeDesc {
  my_name: String, // this is a rather minimal implementation
} // placeholders

impl TypeDesc {
  pub fn name(&self) -> &str { &self.my_name }
}

#[derive(Clone)]
pub struct SampleInfo {}  // placeholder
