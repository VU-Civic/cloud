use civicalert_cloud_common::{EvidenceClip, aws::db::DynamoDbClient, aws::s3::S3Client, params};
use dashmap::DashMap;
use std::{
  collections::HashMap,
  sync::{Arc, Mutex},
};
use tokio::sync::broadcast::{Receiver, Sender, channel};

async fn store_evidence_clip(
  segments: HashMap<u32, EvidenceClip>,
  s3: Arc<Mutex<S3Client>>,
  db: Arc<Mutex<DynamoDbClient>>,
) {
}

async fn collect_device_clips(mut receiver: Receiver<EvidenceClip>, clips: &mut HashMap<u32, EvidenceClip>) {
  let mut clip_complete = false;
  while !clip_complete {
    if let Ok(clip) = receiver.recv().await {
      // clip_complete = true; // TODO: Determine if clip is complete, add to "clips"
    }
  }
}

async fn parse_device_clips(
  device_parsers: Arc<DashMap<String, Sender<EvidenceClip>>>,
  receiver: Receiver<EvidenceClip>,
  clip: EvidenceClip,
  s3: Arc<Mutex<S3Client>>,
  db: Arc<Mutex<DynamoDbClient>>,
) {
  // Insert the initial clip into the local hash map
  let mut clips = HashMap::new();
  let client_id = clip.client_id.clone();
  clips.insert(clip.clip_idx, clip);

  // Process incoming evidence clips for the device until complete or timed out
  if let Ok(()) = tokio::time::timeout(
    tokio::time::Duration::from_secs(params::EVIDENCE_PROCESSING_TIMEOUT_SECONDS),
    collect_device_clips(receiver, &mut clips),
  )
  .await
  {
    std::mem::drop(tokio::spawn(async move {
      store_evidence_clip(clips, s3, db).await;
    }));
  }

  // Remove the device from the parser map
  let _ = device_parsers.remove(&client_id);
}

pub fn create_clip_parser_loop(s3: Arc<Mutex<S3Client>>, db: Arc<Mutex<DynamoDbClient>>) -> Sender<EvidenceClip> {
  // Create structures to transmit and store per-device evidence clip segments
  let device_parsers = Arc::new(DashMap::<String, Sender<EvidenceClip>>::new());
  let (parser_sender, mut parser_receiver) = channel::<EvidenceClip>(10000);

  // Spawn a new task to handle the parsing of evidence clips
  std::mem::drop(tokio::spawn(async move {
    loop {
      if let Ok(clip) = parser_receiver.recv().await {
        if let Some(device_sender) = device_parsers.get(&clip.client_id) {
          let _ = device_sender.value().send(clip);
        } else {
          let s3_clone = s3.clone();
          let db_clone = db.clone();
          let device_parsers_clone = device_parsers.clone();
          let (device_sender, device_receiver) = channel::<EvidenceClip>(100);
          device_parsers.insert(clip.client_id.clone(), device_sender);
          std::mem::drop(tokio::spawn(async move {
            parse_device_clips(device_parsers_clone, device_receiver, clip, s3_clone, db_clone)
          }));
        }
      }
    }
  }));

  // Return a sender to allow sending evidence clip segments to the parser
  parser_sender
}
