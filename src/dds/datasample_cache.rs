use crate::dds::traits::key::{Key,Keyed};
use crate::dds::datasample::DataSample;
use crate::dds::values::result::Result;
use crate::dds::qos::QosPolicies;
use crate::dds::qos::policy::History;
use crate::structure::instance_handle::InstanceHandle;

use std::collections::HashMap;
use std::collections::hash_map::DefaultHasher;
use std::hash::{Hash, Hasher};

pub struct DataSampleCache<D:Keyed> {
  qos: QosPolicies,
  datasamples: HashMap<u64, Vec<DataSample<D>>>,
}

impl<D> DataSampleCache<D>
where
  D: Keyed, 
  <D as Keyed>::K : Key,
{
  pub fn new(qos: QosPolicies) -> DataSampleCache<D> {
    DataSampleCache {
      qos,
      datasamples: HashMap::new(),
    }
  }


  pub fn add_datasample(&mut self, data_sample: DataSample<D>) -> Result<D::K> 
  {
    let key : D::K = data_sample.get_key();
    // TODO: The following three lines should be packaged into a subroutine, and all repetitions thereof
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let h:u64 = hasher.finish();

    let block = self.datasamples.get_mut(&h);

    match self.qos.history() {
      Some(history) => match history {
        History::KeepAll => match block {
          Some(prev_samples) => prev_samples.push(data_sample),
          None => {
            self.datasamples.insert(h, vec![data_sample]);
            // TODO: Check if DDS resource limits are exceeded by this insert, and
            // discard older samples as needed.
          }
        },
        History::KeepLast { depth } => match block {
          Some(prev_samples) => {
            prev_samples.push(data_sample);
            let val = prev_samples.len() - *depth as usize;
            prev_samples.drain(0..val);
          }
          None => {
            self.datasamples.insert(h, vec![data_sample]);
          }
        },
      },
      None => match block {
        // using keep last 1 as default history policy
        Some(prev_samples) => {
          prev_samples.push(data_sample);
          let val = prev_samples.len() - 1;
          prev_samples.drain(0..val);
        }
        None => {
          self.datasamples.insert(h, vec![data_sample]);
        }
      },
    }
    Ok(key)
  }

  pub fn get_datasample(&self, key: &D::K) -> Option<&Vec<DataSample<D>>> {
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let h:u64 = hasher.finish();

    let values = self.datasamples.get(&h);
    values
  }

  pub fn remove_datasamples(&mut self, key: &D::K) {
    let mut hasher = DefaultHasher::new();
    key.hash(&mut hasher);
    let h:u64 = hasher.finish();

    self.datasamples.remove(&h).unwrap();
  }

  pub fn set_qos_policy(&mut self, qos: QosPolicies) {
    self.qos = qos
  }

  pub fn generate_free_instance_handle(&self) -> InstanceHandle {
    let mut instance_handle = InstanceHandle::generate_random_key();

    loop {
      let mut has_key = false;
      for (_, cc) in &self.datasamples {
        for ds in cc {
          if ds.instance_handle == instance_handle {
            has_key = true;
          }
        }
      }

      if !has_key {
        return instance_handle;
      }

      instance_handle = InstanceHandle::generate_random_key();
    }
  }
}

#[cfg(test)]
mod tests {
  use super::*;
  use crate::structure::time::Timestamp;
  use crate::dds::ddsdata::DDSData;
  use crate::dds::traits::key::Keyed;
  use crate::test::random_data::*;

  #[test]
  fn dsc_empty_qos() {
    let qos = QosPolicies::qos_none();
    let mut datasample_cache = DataSampleCache::<RandomData>::new(qos);

    let timestamp = Timestamp::from(time::get_time());
    let data = RandomData {
      a: 4,
      b: "Fobar".to_string(),
    };

    let instance_handle = datasample_cache.generate_free_instance_handle();
    let org_ddsdata = DDSData::from(instance_handle.clone(), &data, Some(timestamp));

    let key = data.get_key().clone();
    let datasample = DataSample::new(timestamp, instance_handle.clone(), data.clone());
    datasample_cache.add_datasample(datasample.clone()).unwrap();
    datasample_cache.add_datasample(datasample).unwrap();

    let samples = datasample_cache.get_datasample(&key);
    match samples {
      Some(ss) => {
        assert_eq!(ss.len(), 1);
        match &ss.get(0).unwrap() {
          Some(huh) => {
            let ddssample = DDSData::from(instance_handle.clone(), huh, Some(timestamp));
            assert_eq!(org_ddsdata, ddssample);
          }
          _ => (),
        }
      }
      None => assert!(false),
    }
  }
}
