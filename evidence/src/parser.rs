use civicalert_cloud_common::{EvidenceClip, aws::db::DynamoDbClient, aws::s3::S3Client, params};
use dashmap::DashMap;
use std::{
  collections::HashMap,
  sync::{Arc, Mutex},
};
use tokio::sync::mpsc::{Receiver, Sender, channel};
use tracing::{error, info};

async fn store_evidence_clip(
  client_id: &str,
  _segments: HashMap<u32, EvidenceClip>,
  _s3: Arc<Mutex<S3Client>>,
  _db: Arc<Mutex<DynamoDbClient>>,
) {
  info!("Full evidence clip received for client: {client_id}");
  // TODO: Combine segments and store in S3 and DynamoDB
}

async fn collect_device_clips(
  mut receiver: Receiver<EvidenceClip>,
  client_id: &str,
  _clips: &mut HashMap<u32, EvidenceClip>,
) {
  let clip_complete = false;
  while !clip_complete {
    if let Some(clip) = receiver.recv().await {
      info!("Received clip segment #{} for client: {client_id}", clip.clip_idx);
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
  // Insert the initial clip into a local device hash map
  let mut clips = HashMap::new();
  let client_id = clip.client_id.clone();
  info!("Received clip segment #{} for client: {client_id}", clip.clip_idx);
  clips.insert(clip.clip_idx, clip);

  // Process incoming device evidence clips until complete or timed out
  if let Ok(()) = tokio::time::timeout(
    tokio::time::Duration::from_secs(params::EVIDENCE_PROCESSING_TIMEOUT_SECONDS),
    collect_device_clips(receiver, client_id.as_str(), &mut clips),
  )
  .await
  {
    // Spawn a new task to merge and store the evidence clip
    let _ = device_parsers.remove(&client_id);
    std::mem::drop(tokio::spawn(async move {
      store_evidence_clip(client_id.as_str(), clips, s3, db).await;
    }));
  } else {
    // Remove the device from the parser map
    let _ = device_parsers.remove(&client_id);
    error!("Evidence clip processing timed out for client: {client_id}");
  }
}

pub fn create_clip_parser_task(s3: Arc<Mutex<S3Client>>, db: Arc<Mutex<DynamoDbClient>>) -> Sender<EvidenceClip> {
  // Create structures to transmit and store per-device evidence clip segments
  let device_parsers = Arc::new(DashMap::<String, Sender<EvidenceClip>>::new());
  let (parser_sender, mut parser_receiver) = channel::<EvidenceClip>(10000);

  // Spawn a new task to handle the parsing of evidence clips
  std::mem::drop(tokio::spawn(async move {
    loop {
      if let Some(clip) = parser_receiver.recv().await {
        if let Some(device_sender) = device_parsers.get(&clip.client_id) {
          let _ = device_sender.send(clip);
        } else {
          info!("Starting new task to process clips from: {}", clip.client_id);
          let s3_clone = s3.clone();
          let db_clone = db.clone();
          let device_parsers_clone = device_parsers.clone();
          let (device_sender, device_receiver) = channel::<EvidenceClip>(100);
          device_parsers.insert(clip.client_id.clone(), device_sender);
          std::mem::drop(tokio::spawn(async move {
            parse_device_clips(device_parsers_clone, device_receiver, clip, s3_clone, db_clone).await
          }));
        }
      }
    }
  }));

  // Return a sender to allow sending evidence clip segments to the parser
  parser_sender
}
