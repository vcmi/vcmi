// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

pub mod gear_client;
pub mod ipfs_client;
pub mod lobby;
pub mod logic;
pub mod program_io;
pub mod utils;
pub mod vcmi_server;

use std::net::SocketAddr;
use std::sync::atomic::AtomicBool;
use std::sync::Arc;

use crate::vcmi_server::VcmiServer;
use crossbeam_channel::{bounded, Sender};

use gear_client::GearClient;
use gear_client::GearCommand;

use gear_client::GearReply;
use gear_connector_api::VcmiCommand;
use gear_connector_api::VcmiReply;
use gstd::FromStr;
use ipfs_client::IpfsClient;
use ipfs_client::IpfsCommand;
use ipfs_client::IpfsReply;
use lobby::LobbyClient;
use logic::Logic;
use tauri::Manager;
use tracing::info;
use tracing_core::LevelFilter;
use tracing_subscriber::{prelude::*, Registry};
use utils::MainWindowSubscriber;

/// We start vcmiclient together with gear-connector.
/// When user chooses multiplayer game, we show dialog with offer to connect to GEAR.
/// If user agrees - we connect, minimize window, show connection status.
/// If user declines - close dialog.
// gui  <-> connector
// vcmi <-> connector -> gear

#[derive(Debug)]
pub enum GuiCommand {
    Connect {
        lobby_address: String,
        username: String,
        node_address: String,
        program_id: String,
        meta_program_id: String,
        battle_program_id: String,
        account_id: String,
        password: String,
    },
    NewRoom {
        room_name: String,
        password: String,
        max_players: u8,
        mods: String,
    },
    JoinRoom {
        room_name: String,
        password: String,
        mods: String,
    },
    Ready {
        room_name: String,
    },
    Leave {
        room_name: String,
    },
    HostMode {
        mode: u8,
    },
    Cancel,
}

fn main() {
    let (vcmi_command_sender, vcmi_command_receiver) = bounded::<VcmiCommand>(1);
    let (vcmi_reply_sender, vcmi_reply_receiver) = bounded::<VcmiReply>(1);

    let (gui_sender, gui_command_receiver) = bounded::<GuiCommand>(1);

    let (gear_command_sender, gear_command_receiver) = bounded::<GearCommand>(1);
    let (gear_reply_sender, gear_reply_receiver) = bounded::<GearReply>(1);

    let (ipfs_command_sender, ipfs_command_receiver) = bounded::<IpfsCommand>(1);
    let (ipfs_reply_sender, ipfs_reply_receiver) = bounded::<IpfsReply>(1);

    let (lobby_command_sender, lobby_command_receiver) = bounded::<lobby::LobbyCommand>(1);
    let (lobby_reply_sender, lobby_reply_receiver) = bounded::<lobby::LobbyReply>(1);

    let need_stop = Arc::new(AtomicBool::new(false));
    let need_stop_clone = need_stop.clone();

    tauri::Builder::default()
        .manage(gui_sender)
        .plugin(tauri_plugin_positioner::init())
        .invoke_handler(tauri::generate_handler![
            connect, skip, new_room, join_room, ready, hostmode, leave
        ])
        .setup(|app| {
            let app_handle = app.handle();

            let main_window = app_handle.get_window("lobby").unwrap();
            let log_window = app_handle.get_window("log").unwrap();

            let filter = LevelFilter::DEBUG;
            let stdout_log = tracing_subscriber::fmt::layer().with_filter(filter);
            let my_subscriber = MainWindowSubscriber {
                window: log_window.clone(),
            };
            let subscriber = Registry::default().with(stdout_log).with(my_subscriber);
            tracing::subscriber::set_global_default(subscriber).unwrap();

            main_window.center().unwrap();
            let address = SocketAddr::from_str("127.0.0.1:0").unwrap();
            tauri::async_runtime::spawn(async move {
                VcmiServer::new(
                    need_stop.clone(),
                    address,
                    vcmi_command_sender,
                    vcmi_reply_receiver,
                )
                .await
                .run()
                .await
                .expect("Server error")
            });

            let need_stop = need_stop_clone.clone();
            std::thread::spawn(move || {
                IpfsClient::new(need_stop.clone(), ipfs_reply_sender, ipfs_command_receiver)
                    .run()
                    .expect("IpfsClient error");
            });

            let need_stop = need_stop_clone.clone();
            let mut logic = Logic::new(
                need_stop.clone(),
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
            );

            tauri::async_runtime::spawn(async move {
                logic.run().await;
            });

            let need_stop = need_stop_clone.clone();
            std::thread::spawn(move || {
                let gear_client =
                    GearClient::new(need_stop_clone, gear_command_receiver, gear_reply_sender);
                gear_client.run()
            });

            std::thread::spawn(move || {
                let mut lobby: LobbyClient =
                    LobbyClient::new(need_stop, lobby_command_receiver, lobby_reply_sender);
                lobby.run().unwrap();
            });

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("Cant create app");
}

#[tauri::command]
async fn connect(
    lobby_address: String,
    username: String,
    node_address: String,
    account_id: String,
    program_id: String,
    meta_program_id: String,
    battle_program_id: String,
    password: String,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    info!(
        "Received Connect from js: LobbyAddress: {lobby_address}, Username: {username}, NodeAddress: {node_address}, ProgramID: {program_id}, AccountID: {account_id}");

    let cmd = GuiCommand::Connect {
        lobby_address,
        username,
        node_address,
        account_id,
        program_id,
        meta_program_id,
        battle_program_id,
        password,
    };
    gui_sender.send(cmd).unwrap();

    Ok(())
}

#[tauri::command]
async fn skip(gui_sender: tauri::State<'_, Sender<GuiCommand>>) -> Result<(), String> {
    let cmd = GuiCommand::Cancel;
    gui_sender.send(cmd).unwrap();

    Ok(())
}

#[tauri::command]
async fn new_room(
    room_name: String,
    password: String,
    max_players: u8,
    mods: String,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    let cmd = GuiCommand::NewRoom {
        room_name,
        password,
        max_players,
        mods,
    };
    gui_sender.send(cmd).expect("Send Error");

    Ok(())
}

#[tauri::command]
async fn join_room(
    room_name: String,
    password: String,
    _mods: String,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    let mods = "h3-for-vcmi-englisation&1.2;vcmi&1.2;vcmi-extras&3.3.6;vcmi-extras.arrowtowericons&1.1;vcmi-extras.battlefieldactions&0.2;vcmi-extras.bonusicons&0.8.1;vcmi-extras.bonusicons.bonus icons&0.8;vcmi-extras.bonusicons.immunity icons&0.6;vcmi-extras.extendedrmg&1.2;vcmi-extras.extraresolutions&1.0;vcmi-extras.quick-exchange&1.0".to_string();
    let cmd = GuiCommand::JoinRoom {
        room_name,
        password,
        mods,
    };
    gui_sender.send(cmd).expect("Send Error");

    Ok(())
}

#[tauri::command]
async fn ready(
    room_name: String,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    let cmd = GuiCommand::Ready { room_name };
    gui_sender.send(cmd).expect("Send Error");

    Ok(())
}

#[tauri::command]
async fn leave(
    room_name: String,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    let cmd = GuiCommand::Leave { room_name };
    gui_sender.send(cmd).expect("Send Error");

    Ok(())
}

#[tauri::command]
async fn hostmode(
    mode: u8,
    gui_sender: tauri::State<'_, Sender<GuiCommand>>,
) -> Result<(), String> {
    info!("Host mode {mode}");
    let cmd = GuiCommand::HostMode { mode };
    gui_sender.send(cmd).expect("Send Error");

    Ok(())
}
