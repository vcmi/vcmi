#![allow(non_camel_case_types, unreachable_patterns)]

mod io;
mod utils;

use futures::{SinkExt, StreamExt};
use gear_connector_api::utils::split_to_reply_read_command_write;
use gear_connector_api::*;
use gstd::prelude::*;
use once_cell::sync::OnceCell;
use std::{
    fs::{File, OpenOptions},
    io::Cursor,
    path::Path,
};

use crossbeam_channel::{bounded, Receiver, Sender};
use std::io::{Read, Write};
use tokio::net::TcpStream as TokioTcpStream;
use tokio::task::JoinHandle;

use std::sync::atomic::AtomicBool;
use std::sync::atomic::Ordering::Relaxed;
use std::sync::Arc;
use zip::{
    write::{FileOptions, ZipWriter},
    ZipArchive,
};

#[allow(dead_code)]
struct Connection {
    runtime: tokio::runtime::Runtime,
    need_stop: Arc<AtomicBool>,
    command_sender: Sender<VcmiCommand>,
    reply_receiver: Receiver<VcmiReply>,
    read_t: JoinHandle<()>,
    write_t: JoinHandle<()>,
}

static mut CONNECTION: OnceCell<Connection> = OnceCell::new();

pub fn save_files_onchain(vcgm_path: String, vsgm_path: String) -> i32 {
    let connection = try_init_connection!(connection_init);
    println!("Rust save_state_onchain");

    let path = Path::new(&vcgm_path);
    let filename = path.file_stem().unwrap().to_str().unwrap();
    assert_eq!(
        filename,
        Path::new(&vsgm_path).file_stem().unwrap().to_str().unwrap()
    );
    let filename = format!("{filename}");
    println!(
        "Save current state {} {} {} on gear chain",
        filename, vcgm_path, vsgm_path,
    );

    let archive = File::create(&filename).unwrap();
    let options = FileOptions::default().compression_method(zip::CompressionMethod::Deflated);
    let mut zip = ZipWriter::new(archive);

    let mut original_vsgm_len = 0;
    match OpenOptions::new().read(true).write(false).open(&vsgm_path) {
        Ok(mut vsgm_file) => {
            let vsgm_inner_path = format!("{filename}.vsgm1");

            zip.start_file(vsgm_inner_path, options).unwrap();
            let mut buffer = Vec::new();
            vsgm_file.read_to_end(&mut buffer).unwrap();
            zip.write_all(&*buffer).unwrap();

            let vsgm_metadata = vsgm_file.metadata().unwrap();
            original_vsgm_len = vsgm_metadata.len();
        }
        Err(e) => println!("Can't open {vsgm_path}: {}", e),
    }

    let mut original_vcgm_len = 0;
    let vcgm_inner_path = format!("{filename}.vcgm1");
    match OpenOptions::new().read(true).write(false).open(&vcgm_path) {
        Ok(mut vcgm_file) => {
            zip.start_file(vcgm_inner_path, options).unwrap();
            let mut buffer = Vec::new();
            vcgm_file.read_to_end(&mut buffer).unwrap();
            zip.write_all(&*buffer).unwrap();

            let vcgm_metadata = vcgm_file.metadata().unwrap();
            original_vcgm_len = vcgm_metadata.len();
        }
        Err(e) => println!("Can't open {vcgm_path}: {}", e),
    }
    let len = {
        let archive = zip.finish().unwrap();
        archive.metadata().unwrap().len()
    };
    let mut buf: Vec<u8> = Vec::with_capacity(len as usize);

    let mut archive = File::open(&filename).unwrap();
    archive.read_to_end(&mut buf).unwrap();

    // archive.read_to_end(&mut buf).unwrap();
    let compressed_len = buf.len();

    let original_len = original_vcgm_len + original_vsgm_len;

    let vcmi_command = VcmiCommand::SaveArchive {
        filename: filename.clone(),
        compressed_archive: buf,
    };
    connection
        .command_sender
        .send(vcmi_command)
        .expect("Error in another thread");
    println!(
        "Save Command.  Sended {filename}.tar {} (vec: {}) (original {}: {} + {} to gear-connector",
        len, compressed_len, original_len, original_vcgm_len, original_vsgm_len,
    );

    0
}

pub fn load_all_from_chain() -> i32 {
    let connection = try_init_connection!(connection_init);

    println!("Load all saved games from chain");

    connection
        .command_sender
        .send(VcmiCommand::LoadAll)
        .expect("Error in another thread");
    println!("Try to receive all saved games");
    let reply = connection.reply_receiver.recv().expect("Recv error");
    match reply {
        VcmiReply::AllLoaded { archives } => {
            for saved_game in archives {
                println!(
                    "Game name: {} {} bytes",
                    saved_game.filename,
                    saved_game.data.len()
                );

                let cursor = Cursor::new(saved_game.data);
                let mut archive = ZipArchive::new(cursor).unwrap();
                archive.extract("~/.local/share/vcmi/Saves/").unwrap();
            }
        }
        _ => unreachable!(),
    }
    0
}

fn save_game_state(day: u32, current_player: String, players: Vec<ffi::RPlayerState>) -> i32 {
    let player_states: Vec<PlayerState> = players.into_iter().map(|state| state.into()).collect();
    println!(
        "Day: {day}, current_player: {current_player}, players: {:?}",
        player_states
    );
    let connection = try_init_connection!(connection_init);

    connection
        .command_sender
        .send(VcmiCommand::SaveGameState {
            day,
            current_player,
            player_states,
        })
        .expect("Error in another thread");
    0
}

fn simulate_battle_onchain(rbattle_info: &mut ffi::RBattleInfo) -> i32 {
    let connection = try_init_connection!(connection_init);
    let battle_info = rbattle_info.clone().into();
    connection
        .command_sender
        .send(VcmiCommand::SimulateBattle(battle_info))
        .expect("Error in another thread");
    let reply = connection.reply_receiver.recv().expect("Recv error");
    match reply {
        VcmiReply::BattleInfo(ref received) => {
            dbg!(received);
            let round = received.round;
            let active_stack = received.active_stack;
            let terrain_type = received.terrain_type.clone().into();
            let stacks = received.stacks.iter().map(|stack| stack.into()).collect();
            let s1 = (&received.sides[0]).into();
            let s2 = (&received.sides[1]).into();
            let sides: [ffi::RBattleSide; 2] = [s1, s2];

            rbattle_info.round = round;
            rbattle_info.active_stack = active_stack;
            rbattle_info.terrain_type = terrain_type;
            rbattle_info.stacks = stacks;
            rbattle_info.sides = sides;

            dbg!(&rbattle_info);
        }
        _ => {
            dbg!(reply);
        }
    }
    0
}

fn connection_init() -> Result<Connection, std::io::Error> {
    let (command_sender, command_receiver) = bounded(1);
    let (reply_sender, reply_receiver) = bounded(1);
    let need_stop = Arc::new(AtomicBool::new(false));
    let need_stop_clone = need_stop.clone();

    let runtime = tokio::runtime::Runtime::new()?;
    let tokio_stream =
        runtime.block_on(async move { TokioTcpStream::connect("127.0.0.1:6666").await })?;

    let need_stop = need_stop_clone.clone();
    let (mut reply_read_stream, mut command_write_stream) =
        split_to_reply_read_command_write(tokio_stream);
    let read_t = runtime.spawn(async move {
        while !need_stop.load(Relaxed) {
            let command = command_receiver.recv().unwrap();
            command_write_stream
                .send(command)
                .await
                .expect("Send VCMI command Error");
        }
        println!("[Read thread] Stop listen gear-proxy")
    });

    let need_stop = need_stop_clone.clone();
    let write_t = runtime.spawn(async move {
        while !need_stop.load(Relaxed) {
            match reply_read_stream.next().await {
                Some(reply) => {
                    let reply = reply.expect("Failed to parse");
                    reply_sender.send(reply).expect("Error in another thread");
                }
                None => {}
            }
        }
        println!("[Write thread] Stop listen gear-proxy")
    });

    let connection = Connection {
        runtime,
        need_stop: need_stop_clone,
        command_sender,
        reply_receiver,
        read_t,
        write_t,
    };
    Ok(connection)
}

#[cxx::bridge]
mod ffi {
    #[repr(u8)]
    enum SelectionScreen {
        Unknown = 0,
        NewGame,
        LoadGame,
        SaveGame,
        ScenarioInfo,
        CampaignList,
    }

    #[derive(Debug)]
    #[repr(i8)]
    enum RPrimarySkill {
        NONE = -1,
        ATTACK,
        DEFENSE,
        SPELL_POWER,
        KNOWLEDGE,
        EXPERIENCE = 4,
    }

    #[derive(Debug, Clone)]
    #[repr(i32)]
    enum RSecondarySkill {
        WRONG = -2,
        DEFAULT = -1,
        PATHFINDING = 0,
        ARCHERY,
        LOGISTICS,
        SCOUTING,
        DIPLOMACY,
        NAVIGATION,
        LEADERSHIP,
        WISDOM,
        MYSTICISM,
        LUCK,
        BALLISTICS,
        EAGLE_EYE,
        NECROMANCY,
        ESTATES,
        FIRE_MAGIC,
        AIR_MAGIC,
        WATER_MAGIC,
        EARTH_MAGIC,
        SCHOLAR,
        TACTICS,
        ARTILLERY,
        LEARNING,
        OFFENCE,
        ARMORER,
        INTELLIGENCE,
        SORCERY,
        RESISTANCE,
        FIRST_AID,
        SKILL_SIZE,
    }

    #[derive(Debug, Clone)]
    #[repr(i32)]

    enum RTerrain {
        NATIVE_TERRAIN = -4,
        ANY_TERRAIN = -3,
        NONE = -1,
        FIRST_REGULAR_TERRAIN = 0,
        DIRT = 0,
        SAND,
        GRASS,
        SNOW,
        SWAMP,
        ROUGH,
        SUBTERRANEAN,
        LAVA,
        WATER,
        ROCK,
        ORIGINAL_REGULAR_TERRAIN_COUNT = 9,
    }

    #[derive(Debug)]
    #[repr(i32)]
    enum RFortLevel {
        None = 0,
        Fort = 1,
        Citadel = 2,
        Castle = 3,
    }

    #[derive(Debug)]
    #[repr(i32)]
    enum RHallLevel {
        None = -1,
        Village = 0,
        Town = 1,
        City = 2,
        Capitol = 3,
    }

    #[derive(Debug, Clone)]
    struct SecondarySkillInfo {
        skill: RSecondarySkill,
        value: u8,
    }

    #[derive(Debug, Clone, Default)]
    pub struct RStack {
        name: String,
        level: i32,
        count: u32,
    }

    #[derive(Debug, Clone)]
    struct RHero {
        name: String,
        level: u32,
        mana: i32,
        sex: u8,
        experience_points: i64,
        secondary_skills: Vec<SecondarySkillInfo>,
        stacks: [RStack; 7],
    }

    #[derive(Debug)]
    struct TownInstance {
        name: String,
        fort_level: RFortLevel,
        hall_level: RHallLevel,
        mage_guild_level: i32,
        level: i32,
    }

    #[derive(Debug)]
    struct RPlayerState {
        color: String,
        team_id: u32,
        is_human: bool,
        resources: String,
        heroes: Vec<RHero>,
        towns: Vec<TownInstance>,
        days_without_castle: i8,
    }

    #[derive(Debug, Clone)]
    struct RBattleSide {
        color: String,
        hero: RHero,
    }

    #[derive(Debug, Clone)]
    struct RBattleInfo {
        stacks: Vec<RStack>,
        sides: [RBattleSide; 2],
        round: i32,
        active_stack: i32,
        terrain_type: RTerrain,
    }

    extern "Rust" {
        fn save_files_onchain(vcgm_path: String, vsgm_path: String) -> i32;
    }

    extern "Rust" {
        fn load_all_from_chain() -> i32;
    }

    extern "Rust" {
        fn save_game_state(day: u32, current_player: String, players: Vec<RPlayerState>) -> i32;
    }

    extern "Rust" {
        fn simulate_battle_onchain(battle_info: &mut RBattleInfo) -> i32;
    }

    // TODO! Try to understand how to include C++ header file
    // enum ESelectionScreen {
    //     unknown, newGame, loadGame, saveGame, scenarioInfo, campaignList,
    // }
    // extern "C++" {
    //     include!("src/headers.h");
    //     type ESelectionScreen;
    // }
}

impl From<ffi::RPlayerState> for PlayerState {
    fn from(value: ffi::RPlayerState) -> Self {
        let heroes = value.heroes.into_iter().map(|hero| hero.into()).collect();
        let towns = value.towns.into_iter().map(|hero| hero.into()).collect();

        let v: Vec<i64> = serde_json::from_str(&value.resources).expect("Can't parse resources");
        let mut resources: Vec<Resource> = Vec::with_capacity(v.len());
        resources.push(Resource::Wood(v[0]));
        resources.push(Resource::Mercury(v[1]));
        resources.push(Resource::Ore(v[2]));
        resources.push(Resource::Sulfur(v[3]));
        resources.push(Resource::Crystal(v[4]));
        resources.push(Resource::Gems(v[5]));
        resources.push(Resource::Gold(v[6]));
        resources.push(Resource::Mithril(v[7]));

        let days_without_castle = if value.days_without_castle >= 0 {
            Some(value.days_without_castle as u8)
        } else {
            None
        };

        Self {
            color: value.color,
            team_id: value.team_id,
            is_human: value.is_human,
            resources,
            heroes,
            towns,
            days_without_castle,
        }
    }
}

impl From<ffi::RHero> for Hero {
    fn from(value: ffi::RHero) -> Self {
        let secondary_skills = value
            .secondary_skills
            .into_iter()
            .map(|skill| skill.into())
            .collect();
        let mut stacks: [Option<Stack>; 7] = Default::default();
        for (i, stack) in value.stacks.iter().enumerate() {
            stacks[i] = if stack.count == 0 {
                None
            } else {
                Some(stack.into())
            };
        }
        Self {
            name: value.name,
            level: value.level,
            mana: value.mana,
            sex: value.sex,
            experience_points: value.experience_points,
            secondary_skills,
            stacks,
        }
    }
}

impl From<Hero> for ffi::RHero {
    fn from(value: Hero) -> Self {
        let secondary_skills = value
            .secondary_skills
            .into_iter()
            .map(|skill| skill.into())
            .collect();
        let mut stacks: [ffi::RStack; 7] = Default::default();
        for (i, stack) in value.stacks.iter().enumerate() {
            match stack {
                Some(stack) => {
                    let s: ffi::RStack = stack.into();
                    stacks[i] = s;
                }
                None => {
                    stacks[i] = Default::default();
                }
            }
        }
        Self {
            name: value.name,
            level: value.level,
            mana: value.mana,
            sex: value.sex,
            experience_points: value.experience_points,
            secondary_skills,
            stacks,
        }
    }
}

impl From<ffi::SecondarySkillInfo> for SecondarySkillInfo {
    fn from(value: ffi::SecondarySkillInfo) -> Self {
        Self {
            value: value.value,
            skill: value.skill.into(),
        }
    }
}

impl From<SecondarySkillInfo> for ffi::SecondarySkillInfo {
    fn from(value: SecondarySkillInfo) -> Self {
        Self {
            value: value.value,
            skill: value.skill.into(),
        }
    }
}

impl From<&ffi::RStack> for Stack {
    fn from(value: &ffi::RStack) -> Self {
        Self {
            name: value.name.clone(),
            level: value.level,
            count: value.count,
        }
    }
}

impl From<&Stack> for ffi::RStack {
    fn from(value: &Stack) -> Self {
        Self {
            name: value.name.clone(),
            level: value.level,
            count: value.count,
        }
    }
}

impl From<ffi::RSecondarySkill> for SecondarySkill {
    fn from(value: ffi::RSecondarySkill) -> Self {
        match value {
            ffi::RSecondarySkill::WRONG => Self::Wrong,
            ffi::RSecondarySkill::DEFAULT => Self::Default,
            ffi::RSecondarySkill::PATHFINDING => Self::Pathfinding,
            ffi::RSecondarySkill::ARCHERY => Self::Archery,
            ffi::RSecondarySkill::LOGISTICS => Self::Logistics,
            ffi::RSecondarySkill::SCOUTING => Self::Scouting,
            ffi::RSecondarySkill::DIPLOMACY => Self::Diplomacy,
            ffi::RSecondarySkill::NAVIGATION => Self::Navigation,
            ffi::RSecondarySkill::LEADERSHIP => Self::Leadership,
            ffi::RSecondarySkill::WISDOM => Self::Wisdom,
            ffi::RSecondarySkill::MYSTICISM => Self::Mysticism,
            ffi::RSecondarySkill::LUCK => Self::Luck,
            ffi::RSecondarySkill::BALLISTICS => Self::Ballistics,
            ffi::RSecondarySkill::EAGLE_EYE => Self::EagleEye,
            ffi::RSecondarySkill::NECROMANCY => Self::Necromancy,
            ffi::RSecondarySkill::ESTATES => Self::Estates,
            ffi::RSecondarySkill::FIRE_MAGIC => Self::FireMagic,
            ffi::RSecondarySkill::AIR_MAGIC => Self::AirMagic,
            ffi::RSecondarySkill::WATER_MAGIC => Self::WaterMagic,
            ffi::RSecondarySkill::EARTH_MAGIC => Self::EarthMagic,
            ffi::RSecondarySkill::SCHOLAR => Self::Scholar,
            ffi::RSecondarySkill::TACTICS => Self::Tactics,
            ffi::RSecondarySkill::ARTILLERY => Self::Artillery,
            ffi::RSecondarySkill::LEARNING => Self::Learning,
            ffi::RSecondarySkill::OFFENCE => Self::Offence,
            ffi::RSecondarySkill::ARMORER => Self::Armorer,
            ffi::RSecondarySkill::INTELLIGENCE => Self::Intelligence,
            ffi::RSecondarySkill::SORCERY => Self::Sorcery,
            ffi::RSecondarySkill::RESISTANCE => Self::Resistance,
            ffi::RSecondarySkill::FIRST_AID => Self::FirstAid,
            ffi::RSecondarySkill::SKILL_SIZE => Self::SkillSize,
            ffi::RSecondarySkill {
                repr: i32::MIN..=-3_i32,
            }
            | ffi::RSecondarySkill {
                repr: 29_i32..=i32::MAX,
            } => Self::SkillSize,
        }
    }
}

impl From<SecondarySkill> for ffi::RSecondarySkill {
    fn from(value: SecondarySkill) -> Self {
        match value {
            SecondarySkill::Wrong => Self::WRONG,
            SecondarySkill::Default => Self::DEFAULT,
            SecondarySkill::Pathfinding => Self::PATHFINDING,
            SecondarySkill::Archery => Self::ARCHERY,
            SecondarySkill::Logistics => Self::LOGISTICS,
            SecondarySkill::Scouting => Self::SCOUTING,
            SecondarySkill::Diplomacy => Self::DIPLOMACY,
            SecondarySkill::Navigation => Self::NAVIGATION,
            SecondarySkill::Leadership => Self::LEADERSHIP,
            SecondarySkill::Wisdom => Self::WISDOM,
            SecondarySkill::Mysticism => Self::MYSTICISM,
            SecondarySkill::Luck => Self::LUCK,
            SecondarySkill::Ballistics => Self::BALLISTICS,
            SecondarySkill::EagleEye => Self::EAGLE_EYE,
            SecondarySkill::Necromancy => Self::NECROMANCY,
            SecondarySkill::Estates => Self::ESTATES,
            SecondarySkill::FireMagic => Self::FIRE_MAGIC,
            SecondarySkill::AirMagic => Self::AIR_MAGIC,
            SecondarySkill::WaterMagic => Self::WATER_MAGIC,
            SecondarySkill::EarthMagic => Self::EARTH_MAGIC,
            SecondarySkill::Scholar => Self::SCHOLAR,
            SecondarySkill::Tactics => Self::TACTICS,
            SecondarySkill::Artillery => Self::ARTILLERY,
            SecondarySkill::Learning => Self::LEARNING,
            SecondarySkill::Offence => Self::OFFENCE,
            SecondarySkill::Armorer => Self::ARMORER,
            SecondarySkill::Intelligence => Self::INTELLIGENCE,
            SecondarySkill::Sorcery => Self::SORCERY,
            SecondarySkill::Resistance => Self::RESISTANCE,
            SecondarySkill::FirstAid => Self::FIRST_AID,
            SecondarySkill::SkillSize => Self::SKILL_SIZE,
        }
    }
}

impl From<ffi::TownInstance> for Town {
    fn from(value: ffi::TownInstance) -> Self {
        Self {
            name: value.name,
            fort_level: value.fort_level.into(),
            hall_level: value.hall_level.into(),
            mage_guild_level: value.mage_guild_level,
            level: value.level,
        }
    }
}

impl From<ffi::RFortLevel> for FortLevel {
    fn from(value: ffi::RFortLevel) -> Self {
        match value {
            ffi::RFortLevel::Fort => Self::Fort,
            ffi::RFortLevel::Citadel => Self::Citadel,
            ffi::RFortLevel::Castle => Self::Castle,
            _ => Self::None,
        }
    }
}

impl From<ffi::RHallLevel> for HallLevel {
    fn from(value: ffi::RHallLevel) -> Self {
        match value {
            ffi::RHallLevel::Village => Self::Village,
            ffi::RHallLevel::Town => Self::Town,
            ffi::RHallLevel::City => Self::City,
            ffi::RHallLevel::Capitol => Self::Capitol,
            _ => Self::None,
        }
    }
}

impl From<ffi::RBattleInfo> for BattleInfo {
    fn from(value: ffi::RBattleInfo) -> Self {
        let stacks = value
            .stacks
            .into_iter()
            .map(|stack| (&stack).into())
            .collect();
        let side1 = (&value.sides[0]).into();
        let side2 = (&value.sides[1]).into();
        let sides = [side1, side2];
        Self {
            stacks,
            sides,
            round: value.round,
            active_stack: value.active_stack,
            terrain_type: value.terrain_type.into(),
        }
    }
}

impl From<ffi::RTerrain> for Terrain {
    fn from(value: ffi::RTerrain) -> Self {
        match value {
            ffi::RTerrain::NATIVE_TERRAIN => Terrain::NativeTerrain,
            ffi::RTerrain::ANY_TERRAIN => Terrain::AnyTerrain,
            ffi::RTerrain::NONE => Terrain::None,
            ffi::RTerrain::FIRST_REGULAR_TERRAIN => Terrain::FirstRegularTerrain,
            ffi::RTerrain::DIRT => Terrain::Dirt,
            ffi::RTerrain::SAND => Terrain::Sand,
            ffi::RTerrain::GRASS => Terrain::Grass,
            ffi::RTerrain::SNOW => Terrain::Snow,
            ffi::RTerrain::SWAMP => Terrain::Swamp,
            ffi::RTerrain::ROUGH => Terrain::Rough,
            ffi::RTerrain::SUBTERRANEAN => Terrain::Subterranean,
            ffi::RTerrain::LAVA => Terrain::Lava,
            ffi::RTerrain::WATER => Terrain::Water,
            ffi::RTerrain::ROCK => Terrain::Rock,
            ffi::RTerrain::ORIGINAL_REGULAR_TERRAIN_COUNT => Terrain::OriginalRegularTerrainCount,
            _ => todo!("Unknown terrain type"),
        }
    }
}

impl From<Terrain> for ffi::RTerrain {
    fn from(value: Terrain) -> Self {
        match value {
            Terrain::NativeTerrain => ffi::RTerrain::NATIVE_TERRAIN,
            Terrain::AnyTerrain => ffi::RTerrain::ANY_TERRAIN,
            Terrain::None => ffi::RTerrain::NONE,
            Terrain::FirstRegularTerrain => ffi::RTerrain::FIRST_REGULAR_TERRAIN,
            Terrain::Dirt => ffi::RTerrain::DIRT,
            Terrain::Sand => ffi::RTerrain::SAND,
            Terrain::Grass => ffi::RTerrain::GRASS,
            Terrain::Snow => ffi::RTerrain::SNOW,
            Terrain::Swamp => ffi::RTerrain::SWAMP,
            Terrain::Rough => ffi::RTerrain::ROUGH,
            Terrain::Subterranean => ffi::RTerrain::SUBTERRANEAN,
            Terrain::Lava => ffi::RTerrain::LAVA,
            Terrain::Water => ffi::RTerrain::WATER,
            Terrain::Rock => ffi::RTerrain::ROCK,
            Terrain::OriginalRegularTerrainCount => ffi::RTerrain::ORIGINAL_REGULAR_TERRAIN_COUNT,
        }
    }
}

impl From<&ffi::RBattleSide> for BattleSide {
    fn from(value: &ffi::RBattleSide) -> Self {
        Self {
            color: value.color.clone(),
            hero: value.hero.clone().into(),
        }
    }
}

impl From<&BattleSide> for ffi::RBattleSide {
    fn from(value: &BattleSide) -> Self {
        Self {
            color: value.color.clone(),
            hero: value.hero.clone().into(),
        }
    }
}
