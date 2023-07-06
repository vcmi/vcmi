use crate::utils::convert_state;
use crossbeam_channel::{Receiver, RecvTimeoutError, Sender};
use gclient::{EventListener, GearApi, WSAddress};
use gear_connector_api::PlayerState;
use gmeta::Encode;
use gstd::ActorId;
use homm3_archive_io::{Action as ArchiveAction, ArchiveDescription, GameArchive};
use homm3_battle_io::BattleInfo;
use homm3_gamestate_io::PlayerState as IoPlayerState;
use std::{
    sync::{
        atomic::{AtomicBool, Ordering::Relaxed},
        Arc, RwLock,
    },
    time::Duration,
};

pub const RECV_TIMEOUT: Duration = std::time::Duration::from_millis(1);

#[derive(Debug)]
pub enum GearCommand {
    ConnectToNode {
        address: WSAddress,
        program_id: String,
        meta_program_id: String,
        battle_program_id: String,
        account_id: String,
        password: String,
    },
    GetFreeBalance,
    SaveArchive(ArchiveDescription),
    SaveGameState {
        day: u32,
        current_player: String,
        player_states: Vec<PlayerState>,
    },
    SendAction(ArchiveAction),
    SimulateBattle(BattleInfo),
    GetSavedGames,
}

#[derive(Debug)]
pub enum GearReply {
    Connected { username: String },
    NotConnected(String),
    ProgramNotFound { program_id: String },
    Simulated(homm3_battle_io::Event),
    Saved(homm3_archive_io::Event),
    FreeBalance(u128),
    SavedGames(Vec<GameArchive>),
}

pub struct GearConnection {
    client: GearApi,
    listener: EventListener,
    program_id: [u8; 32],
    meta_program_id: [u8; 32],
    battle_program_id: [u8; 32],
}

pub struct GearClient {
    need_stop: Arc<AtomicBool>,
    gear_reply_sender: Sender<GearReply>,
    gear_command_receiver: Receiver<GearCommand>,
    gear_connection: Arc<RwLock<Option<GearConnection>>>,
}

unsafe impl Send for GearConnection {}
unsafe impl Send for GearClient {}

impl GearClient {
    pub fn new(
        need_stop: Arc<AtomicBool>,
        gear_command_receiver: Receiver<GearCommand>,
        gear_reply_sender: Sender<GearReply>,
    ) -> Self {
        Self {
            need_stop,
            gear_reply_sender,
            gear_command_receiver,
            gear_connection: Arc::new(RwLock::new(None)),
        }
    }

    pub fn run(&self) {
        let rt = tokio::runtime::Runtime::new().unwrap();
        rt.block_on(async move {
            while !self.need_stop.load(Relaxed) {
                let command = self.gear_command_receiver.recv_timeout(RECV_TIMEOUT);

                match command {
                    Ok(command) => self.process_command(command).await,
                    Err(error) if error == RecvTimeoutError::Timeout => {}
                    Err(error) => {
                        tracing::error!("Error in another thread: {}", error);
                        self.need_stop.store(true, Relaxed);
                    }
                }

                if let Some(connection) = self.gear_connection.write().unwrap().as_mut() {
                    if let Err(e) = connection.listener.blocks_running().await {
                        self.gear_reply_sender
                            .send(GearReply::NotConnected(format!("{e}")))
                            .expect("Cant' send");
                    }
                }

                std::thread::sleep(std::time::Duration::from_millis(1));
            }
        });
    }

    async fn simulate_battle(&self, battle_info: BattleInfo) {
        let mut guard = self
            .gear_connection
            .write()
            .expect("Error in another thread");
        if let Some(GearConnection {
            client,
            program_id: _,
            listener,
            meta_program_id: _,
            battle_program_id,
        }) = guard.as_mut()
        {
            let program_id = (*battle_program_id).into();

            let action = homm3_battle_io::Action::Simulate(battle_info);
            let gas_limit = client
                .calculate_handle_gas(None, program_id, action.encode(), 0, true)
                .await
                .expect("Can't calculate gas for Action::Save")
                .min_limit;
            tracing::info!("Gas limit {} for Action {:?}", gas_limit, action);

            send_message(client, listener,*battle_program_id, action).await;

            let mut battle_infos: Vec<BattleInfo> = client
                .read_state(program_id)
                .await
                .expect("Can't read state");
            tracing::debug!("battle infos: {:?}", battle_infos);
            if let Some(last) = battle_infos.pop() {
                let reply = GearReply::Simulated(homm3_battle_io::Event::BattleResult(last));
                self.gear_reply_sender
                    .send(reply)
                    .expect("Panic in another thread");
            }
        }
    }

    async fn get_saved_games(&self) {
        let guard = self
            .gear_connection
            .read()
            .expect("Error in another thread");
        if let Some(GearConnection {
            client,
            program_id,
            listener: _,
            meta_program_id: _,
            battle_program_id: _,
        }) = guard.as_ref()
        {
            let program_id = (*program_id).into();
            let actor_id = client.account_id().encode();
            let actor_id = ActorId::from_slice(&actor_id).unwrap();
            let saved_games: Vec<GameArchive> = client
                .read_state(program_id)
                .await
                .expect("Can't read state");
            let saved_games: Vec<GameArchive> = saved_games
                .into_iter()
                .filter(|state| state.saver_id.eq(&actor_id))
                .collect();
            tracing::debug!(
                "For ActorId: {:?} len: {}, saved_games: {:?}",
                actor_id,
                saved_games.len(),
                saved_games
            );
            self.gear_reply_sender
                .send(GearReply::SavedGames(saved_games))
                .expect("Panic in another thread");
        } else {
            unreachable!("Not connected to blockchain");
        }
    }

    async fn get_free_balance(&self) {
        let guard = self
            .gear_connection
            .read()
            .expect("Error in another thread");
        if let Some(GearConnection {
            client,
            program_id: _,
            listener: _,
            meta_program_id: _,
            battle_program_id: _,
        }) = guard.as_ref()
        {
            let free_balance = client.free_balance(client.account_id()).await.unwrap();
            self.gear_reply_sender
                .send(GearReply::FreeBalance(free_balance))
                .expect("Panic in another thread");
        } else {
            unreachable!("Not connected to blockchain");
        }
    }

    async fn save_game_archive(&self, archive: ArchiveDescription) {
        let mut guard = self
            .gear_connection
            .write()
            .expect("Error in another thread");
        tracing::debug!("Save to Chain: {:?}", archive);
        if let Some(GearConnection {
            client,
            program_id,
            meta_program_id: _,
            listener,
            battle_program_id: _,
        }) = guard.as_mut()
        {
            let pid = *program_id;
            let account_id = ActorId::from_slice(&client.account_id().encode()).unwrap();

            let action = ArchiveAction::SaveArchive(GameArchive {
                saver_id: account_id,
                archive,
            });
            send_message(client, listener, pid, action).await;
            self.gear_reply_sender
                .send(GearReply::Saved(homm3_archive_io::Event::SavedArchive))
                .unwrap();
        } else {
            tracing::warn!("Can't connect to Gear Blockchain Node")
        }
    }

    async fn save_game_state(
        &self,
        day: u32,
        current_player: String,
        player_states: Vec<PlayerState>,
    ) {
        let mut guard = self
            .gear_connection
            .write()
            .expect("Error in another thread");
        tracing::debug!(
            "Save to Chain: day: {:?}, curreny_player: {:?}",
            day,
            current_player
        );
        if let Some(GearConnection {
            client,
            program_id: _,
            meta_program_id,
            battle_program_id: _,
            listener,
        }) = guard.as_mut()
        {
            let pid = *meta_program_id;
            let player_states: Vec<IoPlayerState> = player_states
                .into_iter()
                .map(|state| convert_state(state))
                .collect();
            let actor_id = client.account_id().encode();
            let actor_id = ActorId::from_slice(&actor_id).unwrap();
            let action = homm3_gamestate_io::Action::SaveGameState {
                saver_id: actor_id,
                day,
                current_player,
                player_states,
            };
            send_message(client, listener, pid, action).await;
        }
    }

    async fn process_command(&self, command: GearCommand) {
        match command {
            GearCommand::ConnectToNode {
                address,
                program_id,
                meta_program_id,
                battle_program_id,
                account_id,
                password,
            } => {
                tracing::info!(
                    "Process GUI command ConnectToNode address: {:?}, Program ID: {}, mnemonic_phrase: {}, password: {}",
                    address,
                    program_id,
                    account_id,
                    password
                );
                let mut guard = self
                    .gear_connection
                    .write()
                    .expect("Error in another thread");
                if guard.is_none() {
                    let client = if account_id.is_empty() {
                        tracing::debug!(
                            "Init GEAR API as default Alice user, address: {:?}",
                            address
                        );
                        GearApi::init(address).await
                    } else {
                        let suri = account_id;
                        tracing::debug!("Init GEAR API as {}, address: {:?}", &suri, address);
                        GearApi::init_with(address, suri).await
                    };
                    match client {
                        Ok(client) => {
                            let (ok, program_id) = self.read_metahash(program_id, &client).await;
                            if ok {
                                let (ok, meta_program_id) =
                                    self.read_metahash(meta_program_id, &client).await;
                                if ok {
                                    let (ok, battle_program_id) =
                                        self.read_metahash(battle_program_id, &client).await;
                                    if ok {
                                        let gear_connection = GearConnection {
                                            client: client.clone(),
                                            listener: client.subscribe().await.unwrap(),
                                            program_id,
                                            meta_program_id,
                                            battle_program_id,
                                        };
                                        let account_id =
                                            gear_connection.client.account_id().clone();
                                        let free_balance =
                                            client.free_balance(client.account_id()).await.unwrap();
                                        tracing::info!(
                                            "Available Balance for {:?}: {}",
                                            account_id,
                                            free_balance
                                        );
                                        guard.replace(gear_connection);

                                        self.gear_reply_sender
                                            .send(GearReply::Connected {
                                                username: account_id.to_string(),
                                            })
                                            .expect("Panic in another thread");
                                    }
                                }
                            }
                        }
                        Err(e) => {
                            tracing::error!("Gear connect Error: {}", e);
                            self.gear_reply_sender
                                .send(GearReply::NotConnected(format!("{e}")))
                                .expect("Panic in another thread");
                        }
                    }
                }
            }
            GearCommand::SendAction(_action) => unreachable!("Shouldn't process action"),
            GearCommand::SimulateBattle(battle_info) => self.simulate_battle(battle_info).await,
            GearCommand::GetFreeBalance => self.get_free_balance().await,
            GearCommand::GetSavedGames => self.get_saved_games().await,
            GearCommand::SaveArchive(archive) => self.save_game_archive(archive).await,
            GearCommand::SaveGameState {
                day,
                current_player,
                player_states,
            } => {
                self.save_game_state(day, current_player, player_states)
                    .await
            }
        }
    }

    async fn read_metahash(&self, program_id: String, client: &GearApi) -> (bool, [u8; 32]) {
        let pid = hex::decode(&program_id[2..]).expect("Can't decode Program ID");
        let mut program_id = [0u8; 32];
        program_id.copy_from_slice(&pid);

        match client.read_metahash(program_id.into()).await {
            Ok(hash) => {
                tracing::info!("Program hash: {:?}", hash);
                (true, program_id)
            }
            Err(err) => {
                tracing::error!("Read Metahash Error: {}", err);
                self.gear_reply_sender
                    .send(GearReply::ProgramNotFound {
                        program_id: hex::encode(&program_id),
                    })
                    .expect("Error in another thread");
                (false, [0u8; 32])
            }
        }
    }
}

async fn send_message(
    client: &GearApi,
    listener: &mut EventListener,
    program_id: [u8; 32],
    payload: impl Encode + gstd::fmt::Debug,
) {
    let program_id = program_id.into();

    let gas_limit = client
        .calculate_handle_gas(None, program_id, payload.encode(), 0, true)
        .await
        .expect("Can't calculate gas for Action::Save")
        .min_limit;
    tracing::info!("Gas limit {} for Action {:?}", gas_limit, payload);

    for _ in 0..10 {
        if let Err(e) = client
            .send_message(program_id, &payload, gas_limit, 0)
            .await
        {
            if let gclient::Error::GearSDK(err) = e {
                if let gsdk::Error::Tx(error) = err {
                    if let gsdk::result::TxError::Retracted(_) = error {
                        listener.blocks_running().await.expect("Block running ");
                        continue;
                    }
                }
            } else {
                panic!("Can't send {:?} error: {}", &payload, e);
            }
        } else {
            tracing::info!("Sent Action to Gear: {:?}", payload);
            break;
        }
    }
}
