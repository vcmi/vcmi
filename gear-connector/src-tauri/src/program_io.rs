// use gear_connector_api::PlayerState;
// use gmeta::{InOut, Metadata};
// use gstd::{prelude::*, ActorId};

// #[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
// pub struct ArchiveDescription {
//     pub filename: String,
//     pub hash: String,
// }

// #[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
// pub struct GameArchive {
//     pub saver_id: ActorId,
//     pub archive: ArchiveDescription,
// }

// #[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
// pub enum Action {
//     SaveArchive(GameArchive),
//     SaveGameState {
//         day: u32,
//         current_player: String,
//         player_states: Vec<PlayerState>,
//     },
//     Load {
//         hash: String,
//     },
// }

// #[derive(Encode, Decode, TypeInfo, Hash, PartialEq, PartialOrd, Eq, Ord, Clone, Debug)]
// pub enum Event {
//     Loaded(Option<GameArchive>),
//     SavedArchive,
//     SavedGameState,
// }

// pub struct ContractMetadata;

// impl Metadata for ContractMetadata {
//     type Init = ();
//     type Handle = InOut<Action, Event>;
//     type Others = ();
//     type Reply = ();
//     type Signal = ();
//     type State = Vec<GameArchive>;
// }
