use crate::{
    gear_client::{GearCommand, GearReply, RECV_TIMEOUT},
    ipfs_client::{IpfsCommand, IpfsReply},
    lobby::{LobbyCommand, LobbyReply, VCMI_VERSION},
    utils::convert_battle_info2,
    GuiCommand,
};
use crossbeam_channel::{Receiver, RecvTimeoutError, Sender};
use gclient::WSAddress;
use gear_connector_api::{BattleInfo, PlayerState, VcmiCommand, VcmiReply, VcmiSavedGame};
use homm3_archive_io::{Action, ArchiveDescription, Event};
use std::{
    process::Command,
    sync::{
        atomic::{AtomicBool, Ordering::Relaxed},
        Arc,
    },
};
use tauri::{LogicalSize, PhysicalSize, Size, Window};
use tauri_plugin_positioner::{Position, WindowExt};

pub enum Recipient {
    GearClient,
    Vcmi,
    Gui,
}

pub enum Message {
    VcmiCommand,
    VcmiReply,
    Action,
    Event,
}

pub struct Logic {
    need_stop: Arc<AtomicBool>,
    gear_command_sender: Sender<GearCommand>,
    gear_reply_receiver: Receiver<GearReply>,
    vcmi_command_receiver: Receiver<VcmiCommand>,
    vcmi_reply_sender: Sender<VcmiReply>,
    ipfs_reply_receiver: Receiver<IpfsReply>,
    ipfs_command_sender: Sender<IpfsCommand>,
    gui_command_receiver: Receiver<GuiCommand>,
    lobby_command_sender: Sender<LobbyCommand>,
    lobby_reply_receiver: Receiver<LobbyReply>,
    main_window: Window,
    log_window: Window,
}

impl Logic {
    pub fn new(
        need_stop: Arc<AtomicBool>,
        gear_command_sender: Sender<GearCommand>,
        gear_reply_receiver: Receiver<GearReply>,
        vcmi_command_receiver: Receiver<VcmiCommand>,
        vcmi_reply_sender: Sender<VcmiReply>,
        ipfs_reply_receiver: Receiver<IpfsReply>,
        ipfs_command_sender: Sender<IpfsCommand>,
        gui_command_receiver: Receiver<GuiCommand>,
        lobby_command_sender: Sender<LobbyCommand>,
        lobby_reply_receiver: Receiver<LobbyReply>,

        main_window: Window,
        log_window: Window,
    ) -> Self {
        Self {
            need_stop,
            gear_command_sender,
            gear_reply_receiver,
            vcmi_command_receiver,
            vcmi_reply_sender,
            ipfs_reply_receiver,
            ipfs_command_sender,
            gui_command_receiver,
            lobby_command_sender,
            lobby_reply_receiver,
            main_window,
            log_window,
        }
    }

    pub async fn run(&mut self) {
        while !self.need_stop.load(Relaxed) {
            self.process_gui_command();
            self.process_vcmi_command().await;
            self.process_lobby_reply();
        }
    }

    fn connect_to_gear(&self) {
        self.main_window.center().unwrap();
        self.main_window.show().unwrap();
        self.main_window.set_focus().unwrap();
        self.vcmi_reply_sender
            .send(VcmiReply::ConnectDialogShowed)
            .expect("Error in another thread");
    }

    // fn show_load_game_dialog(&self) {
    //     let command = GearCommand::GetSavedGames;
    //     self.gear_command_sender.send(command).expect("Can't send");
    //     let reply = self.gear_reply_receiver.recv().expect("Can't recv");

    //     match reply {
    //         GearReply::SavedGames(_games) => {}
    //         _ => unreachable!("Unexpected reply to GetSavedGames command"),
    //     }
    //     self.vcmi_reply_sender
    //         .send(VcmiReply::LoadGameDialogShowed)
    //         .expect("Error in another thread");
    // }

    fn simulate_battle(&self, battle_info: BattleInfo) {
        let battle_info = crate::utils::convert_battle_info(battle_info);
        let gear_command = GearCommand::SimulateBattle(battle_info);

        self.gear_command_sender
            .send(gear_command)
            .expect("Send error");
        let gear_reply = self.gear_reply_receiver.recv().expect("Recv error");
        tracing::debug!("simulate battle reply: {:?}", gear_reply);
        match gear_reply {
            GearReply::Simulated(e) => match e {
                homm3_battle_io::Event::BattleResult(res) => {
                    self.vcmi_reply_sender
                        .send(VcmiReply::BattleInfo(convert_battle_info2(res)))
                        .expect("Send error");
                }
            },
            _ => {}
        }
    }

    fn save_archive(&self, filename: String, compressed_archive: Vec<u8>) {
        let archive_name = format!("{filename}");

        tracing::info!("Archive len: {}", compressed_archive.len());

        let command = IpfsCommand::UploadData {
            filename,
            data: compressed_archive,
        };
        self.ipfs_command_sender.send(command).expect("Send error");

        let reply = self.ipfs_reply_receiver.recv().expect("Recv error");

        if let IpfsReply::Uploaded { name: _, hash } = reply {
            let archive = ArchiveDescription {
                filename: archive_name,
                hash,
            };

            let gear_command = GearCommand::SaveArchive(archive);
            self.gear_command_sender
                .send(gear_command)
                .expect("Send error");
            let gear_reply = self.gear_reply_receiver.recv().expect("Recv error");

            if let GearReply::Saved(e) = gear_reply {
                if matches!(e, Event::SavedArchive) {
                    self.vcmi_reply_sender
                        .send(VcmiReply::Saved)
                        .expect("Send error");
                    return;
                }
            }
        }

        unreachable!();
    }

    fn save_game_state(&self, day: u32, current_player: String, player_states: Vec<PlayerState>) {
        let gear_command = GearCommand::SaveGameState {
            day,
            current_player,
            player_states,
        };
        self.gear_command_sender
            .send(gear_command)
            .expect("Send error");
    }

    fn load_all(&self) {
        self.gear_command_sender
            .send(GearCommand::GetSavedGames)
            .expect("Send error");

        let gear_reply = self.gear_reply_receiver.recv().expect("Recv Error");
        match gear_reply {
            GearReply::SavedGames(games) => {
                let mut archives = Vec::with_capacity(games.len());
                for state in games.into_iter() {
                    let hash = state.archive.hash;
                    let ipfs_command = IpfsCommand::DownloadData { hash };
                    self.ipfs_command_sender
                        .send(ipfs_command)
                        .expect("Send err");
                    let ipfs_reply = self.ipfs_reply_receiver.recv().expect("Recv err");
                    match ipfs_reply {
                        IpfsReply::Downloaded { data } => {
                            archives.push(VcmiSavedGame {
                                filename: state.archive.filename,
                                data,
                            });
                        }
                        _ => unreachable!("Wrong reply to Ipfs Download command"),
                    }
                }
                let vcmi_reply = VcmiReply::AllLoaded { archives };
                self.vcmi_reply_sender.send(vcmi_reply).expect("Send err");
            }
            _ => unreachable!("Wrong reply to GetSavedGames"),
        }
    }

    async fn update_balance(&self) {
        self.gear_command_sender
            .send(GearCommand::GetFreeBalance)
            .expect("Send Error");

        let reply = self.gear_reply_receiver.recv().expect("Recv error");
        match reply {
            GearReply::FreeBalance(balance) => {
                self.log_window.emit("update_balance", balance).unwrap();
                tracing::info!("Free balance: {}", balance);
            }
            _ => unreachable!("Reply {reply:?} is wrong to command FreeBalance"),
        }
    }

    async fn process_vcmi_command(&self) {
        match self.vcmi_command_receiver.recv_timeout(RECV_TIMEOUT) {
            Ok(vcmi_command) => match vcmi_command {
                VcmiCommand::Connect => self.connect_to_gear(),
                VcmiCommand::SaveGameState {
                    day,
                    current_player,
                    player_states,
                } => {
                    self.save_game_state(day, current_player, player_states);
                    self.update_balance().await;
                }
                VcmiCommand::SaveArchive {
                    filename,
                    compressed_archive,
                } => {
                    self.save_archive(filename, compressed_archive);
                    self.update_balance().await;
                }
                VcmiCommand::Load(name) => self
                    .gear_command_sender
                    .send(GearCommand::SendAction(Action::Load { hash: name }))
                    .expect("Error in another thread"),
                VcmiCommand::ShowLoadGameDialog => {
                    unreachable!("Shouldn't request ShowLoadGameDialog")
                }
                VcmiCommand::LoadAll => self.load_all(),
                VcmiCommand::SimulateBattle(battle_info) => self.simulate_battle(battle_info),
            },
            Err(e) if e == RecvTimeoutError::Timeout => {}
            Err(e) => {
                tracing::error!("Error in another thread: {}", e);
                self.need_stop.store(true, Relaxed);
            }
        }
    }

    fn connect_to_node(
        &self,
        address: String,
        program_id: String,
        meta_program_id: String,
        battle_program_id: String,
        account_id: String,
        password: String,
    ) {
        let port = match address.starts_with("ws://localhost") {
            true => 9944,
            false => 443,
        };
        let address = WSAddress::new(address, port);
        self.gear_command_sender
            .send(GearCommand::ConnectToNode {
                address,
                program_id,
                meta_program_id,
                battle_program_id,
                password,
                account_id,
            })
            .expect("Error in another thread");

        let reply = self.gear_reply_receiver.recv().expect("Recv error");

        match reply {
            GearReply::Connected { username } => {
                tracing::info!("Connected to node. Account ID: {username}");
                self.log_window.emit("update_account_id", username).unwrap();
            }
            GearReply::NotConnected(reason) => self.main_window.emit("alert", reason).unwrap(),
            GearReply::ProgramNotFound { program_id } => {
                self.main_window.emit("alert", program_id).unwrap()
            }
            _ => unreachable!("Reply {reply:?} is wrong to command Connect"),
        }
    }

    fn connect_to_lobby(&mut self, address: String, username: String) {
        self.lobby_command_sender
            .send(LobbyCommand::Connect(address, username.clone()))
            .expect("Send error");
        self.lobby_command_sender
            .send(LobbyCommand::Greeting(username, VCMI_VERSION.to_string()))
            .expect("Send Error")
    }

    fn process_gui_command(&mut self) {
        match self.gui_command_receiver.recv_timeout(RECV_TIMEOUT) {
            Ok(gui_command) => {
                tracing::debug!("Process Gui Command: {:?}", gui_command);
                match gui_command {
                    GuiCommand::Connect {
                        lobby_address,
                        username,
                        node_address,
                        program_id,
                        meta_program_id,
                        battle_program_id,
                        password,
                        account_id,
                    } => {
                        self.connect_to_lobby(lobby_address, username);
                        self.connect_to_node(
                            node_address,
                            program_id,
                            meta_program_id,
                            battle_program_id,
                            account_id,
                            password,
                        );
                    }
                    GuiCommand::Cancel => {
                        self.main_window.hide().unwrap();
                        self.vcmi_reply_sender
                            .send(VcmiReply::CanceledDialog)
                            .expect("Panic in another thread");
                        self.need_stop.store(true, Relaxed);
                    }
                    GuiCommand::NewRoom {
                        room_name,
                        password,
                        max_players,
                        mods,
                    } => {
                        let lobby_command =
                            LobbyCommand::Create(room_name, password, max_players, mods);
                        self.lobby_command_sender
                            .send(lobby_command)
                            .expect("Send Error");
                    }
                    GuiCommand::JoinRoom {
                        room_name,
                        password,
                        mods,
                    } => {
                        let lobby_command = LobbyCommand::Join(room_name, password, mods);
                        self.lobby_command_sender
                            .send(lobby_command)
                            .expect("Send Error");
                    }
                    GuiCommand::Ready { room_name } => {
                        let lobby_command = LobbyCommand::Ready(room_name);
                        self.lobby_command_sender
                            .send(lobby_command)
                            .expect("Send Error");
                    }
                    GuiCommand::HostMode { mode } => {
                        let lobby_command = LobbyCommand::HostMode(mode);
                        self.lobby_command_sender
                            .send(lobby_command)
                            .expect("Send Error");
                    }
                    GuiCommand::Leave { room_name } => {
                        let lobby_command = LobbyCommand::Leave(room_name);
                        self.lobby_command_sender
                            .send(lobby_command)
                            .expect("Send Error");
                    }
                }
            }
            Err(e) if e == RecvTimeoutError::Timeout => {}
            Err(e) => {
                tracing::error!("Error in another thread: {}", e);
                self.need_stop.store(true, Relaxed);
            }
        }
    }

    fn process_lobby_reply(&mut self) {
        match self.lobby_reply_receiver.recv_timeout(RECV_TIMEOUT) {
            Ok(lobby_reply) => {
                tracing::debug!("Process Lobby Reply: {:?}", lobby_reply);
                match lobby_reply {
                    LobbyReply::Connected { error } => {
                        if error.is_empty() {
                            tracing::debug!("Connected to lobby");
                            self.main_window.emit("showRooms", "").unwrap();

                            let mon = self.log_window.current_monitor();
                            let monitor_size = mon.unwrap().unwrap().size().clone();
                            self.log_window.move_window(Position::TopRight).unwrap();
                            std::thread::sleep(std::time::Duration::from_millis(10));
                            self.log_window
                                .set_size(Size::Physical(PhysicalSize {
                                    width: 480,
                                    height: monitor_size.height,
                                }))
                                .unwrap();
                            // std::thread::sleep(std::time::Duration::from_millis(1));
                            self.log_window.show().unwrap();
                            self.log_window.move_window(Position::TopRight).unwrap();
                        } else {
                            self.main_window.emit("alert", error).unwrap();
                        }
                    }
                    LobbyReply::Created(room_name) => {
                        self.main_window.emit("created", room_name).unwrap()
                    }
                    LobbyReply::Sessions(rooms) => {
                        self.main_window.emit("addSessions", &rooms).unwrap()
                    }
                    LobbyReply::Joined(room_name, username) => self
                        .main_window
                        .emit("joined", (room_name, username))
                        .unwrap(),
                    LobbyReply::Kicked(room_name, username) => self
                        .main_window
                        .emit("kicked", (room_name, username))
                        .unwrap(),
                    LobbyReply::Start {
                        lobby_address,
                        lobby_port,
                        game_mode,
                        username,
                        connection_uuid,
                        vcmiserver_uuid,
                        players_count,
                    } => {
                        tracing::debug!("connection_uuid: {}", connection_uuid);
                        let mut args = vec![];
                        args.push("--lobby".to_string());
                        if let Some(vcmiserver_uuid) = vcmiserver_uuid {
                            args.push("--lobby-host".to_string());
                            args.push("--lobby-uuid".to_string());
                            args.push(vcmiserver_uuid);
                        }
                        if let Some(players_count) = players_count {
                            args.push("--lobby-connections".to_string());
                            args.push(players_count.to_string());
                        }
                        args.push("--lobby-address".to_string());
                        args.push(lobby_address);
                        args.push("--lobby-port".to_string());
                        args.push(lobby_port.to_string());
                        args.push("--lobby-username".to_string());
                        args.push(username);
                        args.push("--lobby-gamemode".to_string());
                        args.push(game_mode.to_string());
                        args.push("--uuid".to_string());
                        args.push(connection_uuid);

                        self.log_window
                            .set_size(Size::Logical(LogicalSize::new(0.2, 2.0)))
                            .unwrap();
                        self.log_window.move_window(Position::TopRight).unwrap();
                        self.log_window.show().unwrap();
                        std::thread::sleep(std::time::Duration::from_millis(1));

                        start_game(args);
                    }
                    LobbyReply::Host(_, _) => unreachable!(),
                    LobbyReply::Status(users_count, statuses) => self
                        .main_window
                        .emit("status", (users_count, statuses))
                        .unwrap(),
                    LobbyReply::ServerError(error) => {
                        self.main_window.emit("alert", error).unwrap()
                    }
                    LobbyReply::Mods => {}
                    LobbyReply::ClientMods => {}
                    LobbyReply::Chat(username, message) => {
                        self.main_window
                            .emit("chatMessage", (username, message))
                            .unwrap();
                    }
                    LobbyReply::Users(users) => {
                        self.main_window
                            .emit("addUsers", users)
                            .expect("Can't emit addUsers");
                        tracing::debug!("add user");
                    }
                    LobbyReply::Health => todo!(),
                    LobbyReply::GameMode(game_mode) => {
                        self.main_window.emit("updateGameMode", game_mode).unwrap();
                    }
                }
            }
            Err(e) if e == RecvTimeoutError::Timeout => {}
            Err(e) => {
                tracing::error!("process_lobby_reply(): Error in another thread: {}", e);
                self.need_stop.store(true, Relaxed);
            }
        }
    }
}

fn start_game(args: Vec<String>) {
    let arg = args.join(" ");
    let vcmiclient_path = match std::env::var("VCMICLIENT_PATH") {
        Ok(dir_path) => dir_path,
        Err(_) => "./vcmiclient".to_string(),
    };
    tracing::info!("Start game: {vcmiclient_path} {arg}");
    Command::new(&vcmiclient_path)
        .args(&args)
        .spawn()
        .expect("Failed to spawn process");
}
