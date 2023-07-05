use crate::gear_client::RECV_TIMEOUT;
use crossbeam_channel::{Receiver, RecvTimeoutError, Sender};
use futures::{SinkExt, StreamExt};
use gear_connector_api::{utils::*, VcmiCommand, VcmiReply};
use std::net::SocketAddr;
use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering::Relaxed;
use std::sync::Arc;
use tokio::net::TcpListener;

#[derive(Debug)]
pub struct VcmiServer {
    need_stop: Arc<AtomicBool>,
    address: SocketAddr,
    vcmi_command_sender: Sender<VcmiCommand>,
    vcmi_reply_receiver: Receiver<VcmiReply>,
}

impl VcmiServer {
    pub async fn new(
        need_stop: Arc<AtomicBool>,
        address: SocketAddr,
        vcmi_command_sender: Sender<VcmiCommand>,
        vcmi_reply_receiver: Receiver<VcmiReply>,
    ) -> Self {
        tracing::debug!("Create Server");
        Self {
            need_stop,
            address,
            vcmi_command_sender,
            vcmi_reply_receiver,
        }
    }
}
impl VcmiServer {
    pub async fn run(&mut self) -> std::io::Result<()> {
        let vcmi_command_sender = self.vcmi_command_sender.clone();
        let vcmi_reply_receiver = self.vcmi_reply_receiver.clone();
        let need_stop_clone = self.need_stop.clone();
        let listener = TcpListener::bind(self.address).await?;

        tokio::spawn(async move {
            let need_stop = need_stop_clone.clone();
            while !need_stop.load(Relaxed) {
                match listener.accept().await {
                    Ok((stream, _addr)) => {
                        tracing::info!("Connected");
                        let (mut read_stream, mut write_stream) =
                            wrap_to_command_read_reply_write(stream);
                        let need_stop_clone = need_stop.clone();
                        let vcmi_command_sender = vcmi_command_sender.clone();

                        tokio::spawn(async move {
                            while !need_stop_clone.load(Relaxed) {
                                let command = match read_stream.next().await {
                                    Some(data) => data.expect("Can't parse"),
                                    None => continue,
                                };

                                vcmi_command_sender.send(command).expect(
                                    "Can't send command to Logic. Maybe thread crashed incorrectly",
                                );
                            }
                        });

                        let need_stop_clone = need_stop.clone();
                        let vcmi_reply_receiver = vcmi_reply_receiver.clone();
                        tokio::spawn(async move {
                            while !need_stop_clone.load(Relaxed) {
                                let reply = vcmi_reply_receiver.recv_timeout(RECV_TIMEOUT);
                                match reply {
                                    Ok(reply) => {
                                        match &reply {
                                            VcmiReply::AllLoaded { archives } => {
                                                tracing::info!(
                                                    "Send Reply to VCMI: AllLoaded len: {}",
                                                    archives.len()
                                                );
                                            }
                                            _ => tracing::info!("Send Reply to VCMI: {:?}", reply),
                                        }

                                        write_stream
                                            .send(reply)
                                            .await
                                            .expect("Cant' send VcmiReply");
                                    }
                                    Err(error) if error == RecvTimeoutError::Timeout => {}
                                    Err(error) => {
                                        tracing::error!("Error in another thread: {}", error);
                                        need_stop_clone.store(true, Relaxed);
                                    }
                                }
                            }
                        });
                    }
                    Err(e) => tracing::error!("{}", e),
                }
            }
        });

        Ok(())
    }
}
