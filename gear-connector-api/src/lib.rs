use bytes::{Buf, BytesMut};
use parity_scale_codec::{Decode, Encode};
use scale_info::TypeInfo;
use serde::{Deserialize, Serialize};
use std::error::Error;
use tokio_util::codec::{Decoder, Encoder};

pub mod utils;

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
#[repr(u8)]
pub enum PrimarySkill {
    None = u8::MAX,
    Attack = 0,
    Defense,
    SpellPower,
    Knowledge,
    Experience = 4,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
#[repr(u8)]
pub enum SecondarySkill {
    Wrong = u8::MAX - 1,
    Default = u8::MAX,
    Pathfinding = 0,
    Archery,
    Logistics,
    Scouting,
    Diplomacy,
    Navigation,
    Leadership,
    Wisdom,
    Mysticism,
    Luck,
    Ballistics,
    EagleEye,
    Necromancy,
    Estates,
    FireMagic,
    AirMagic,
    WaterMagic,
    EarthMagic,
    Scholar,
    Tactics,
    Artillery,
    Learning,
    Offence,
    Armorer,
    Intelligence,
    Sorcery,
    Resistance,
    FirstAid,
    SkillSize,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
#[repr(i32)]
pub enum FortLevel {
    None = 0,
    Fort = 1,
    Citadel = 2,
    Castle = 3,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
#[repr(u8)]
pub enum HallLevel {
    None = u8::MAX,
    Village = 0,
    Town = 1,
    City = 2,
    Capitol = 3,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub struct SecondarySkillInfo {
    pub skill: SecondarySkill,
    pub value: u8,
}

#[derive(Debug, Default, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub struct Stack {
    pub name: String,
    pub level: i32,
    pub count: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub struct Hero {
    pub name: String,
    pub level: u32,
    pub mana: i32,
    pub sex: u8,
    pub experience_points: i64,
    pub secondary_skills: Vec<SecondarySkillInfo>,
    pub stacks: [Option<Stack>; 7],
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub struct Town {
    pub name: String,
    pub fort_level: FortLevel,
    pub hall_level: HallLevel,
    pub mage_guild_level: i32,
    pub level: i32,
}

#[derive(
    Debug,
    Clone,
    Serialize,
    Deserialize,
    Hash,
    PartialEq,
    PartialOrd,
    Eq,
    Ord,
    Encode,
    Decode,
    TypeInfo,
)]
pub enum Resource {
    Wood(i64),
    Mercury(i64),
    Ore(i64),
    Sulfur(i64),
    Crystal(i64),
    Gems(i64),
    Gold(i64),
    Mithril(i64),
    WoodAndOre,
    Invalid,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub enum Terrain {
    NativeTerrain,
    AnyTerrain,
    None,
    FirstRegularTerrain,
    Dirt,
    Sand,
    Grass,
    Snow,
    Swamp,
    Rough,
    Subterranean,
    Lava,
    Water,
    Rock,
    OriginalRegularTerrainCount,
}

#[derive(Debug, Clone, Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord)]
pub struct PlayerState {
    pub color: String,
    pub team_id: u32,
    pub is_human: bool,
    pub resources: Vec<Resource>,
    pub heroes: Vec<Hero>,
    pub towns: Vec<Town>,
    pub days_without_castle: Option<u8>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum VcmiCommand {
    Connect,
    ShowLoadGameDialog,
    SaveGameState {
        day: u32,
        current_player: String,
        player_states: Vec<PlayerState>,
    },
    SaveArchive {
        filename: String,
        compressed_archive: Vec<u8>,
    },
    SimulateBattle(BattleInfo),
    Load(String),
    LoadAll,
}

#[derive(Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub struct BattleInfo {
    pub stacks: Vec<Stack>,
    pub sides: [BattleSide; 2],
    pub round: i32,
    pub active_stack: i32,
    pub terrain_type: Terrain,
}
#[derive(Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub struct BattleSide {
    pub color: String,
    pub hero: Hero,
}

#[derive(Serialize, Deserialize, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub struct VcmiSavedGame {
    pub filename: String,
    pub data: Vec<u8>,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub enum VcmiReply {
    ConnectDialogShowed,
    CanceledDialog,
    Connected,

    Saved,
    Loaded { archive_data: Vec<u8> },
    AllLoaded { archives: Vec<VcmiSavedGame> },
    BattleInfo(BattleInfo),
    LoadGameDialogShowed,
}

pub struct VcmiCommandCodec;

impl Encoder<VcmiCommand> for VcmiCommandCodec {
    type Error = Box<dyn Error>;

    fn encode(&mut self, item: VcmiCommand, dst: &mut BytesMut) -> Result<(), Self::Error> {
        let item = serde_json::to_vec(&item)?;
        let len_slice = u32::to_ne_bytes(item.len() as u32);

        // Reserve space in the buffer.
        dst.reserve(4 + item.len());

        // Write the length and string to the buffer.
        dst.extend_from_slice(&len_slice);
        dst.extend_from_slice(item.as_slice());
        Ok(())
    }
}

impl Decoder for VcmiCommandCodec {
    type Item = VcmiCommand;
    type Error = Box<dyn Error>;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        if src.len() < 4 {
            // Not enough data to read length marker.
            return Ok(None);
        }

        let mut length_bytes = [0u8; 4];
        length_bytes.copy_from_slice(&src[..4]);
        let length = u32::from_le_bytes(length_bytes) as usize;

        if src.len() < 4 + length {
            // The full string has not yet arrived.
            //
            // We reserve more space in the buffer. This is not strictly
            // necessary, but is a good idea performance-wise.
            src.reserve(4 + length - src.len());

            // We inform the Framed that we need more bytes to form the next
            // frame.
            return Ok(None);
        }

        let data = src[4..4 + length].to_vec();
        src.advance(4 + length);

        let mut deserializer = serde_json::Deserializer::from_slice(&data);
        let command = VcmiCommand::deserialize(&mut deserializer)?;
        Ok(Some(command))
    }
}
