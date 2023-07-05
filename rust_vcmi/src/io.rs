use gmeta::{InOut, Metadata};
use gstd::{prelude::*, ActorId};

#[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub struct GameState {
    pub name: String,
    pub data: Vec<u8>,
}

#[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub enum Action {
    Save(GameState),
    Load { name: String },
}

#[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
pub enum Event {
    Loaded(Option<GameState>),
    Saved,
}

pub struct ContractMetadata;

impl Metadata for ContractMetadata {
    type Init = ();
    type Handle = InOut<Action, Event>;
    type Others = ();
    type Reply = ();
    type Signal = ();
    type State = Vec<(ActorId, GameState)>;
}
