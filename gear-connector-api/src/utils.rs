use tokio::net::tcp::{OwnedReadHalf, OwnedWriteHalf};
use tokio::net::TcpStream;
use tokio_serde::{formats::Json, Framed};

use tokio_util::codec::{FramedRead, FramedWrite, LengthDelimitedCodec};

use crate::{VcmiCommand, VcmiReply};

type WrappedStream = FramedRead<OwnedReadHalf, LengthDelimitedCodec>;
type WrappedSink = FramedWrite<OwnedWriteHalf, LengthDelimitedCodec>;

// We use the unit type in place of the message types since we're
// only dealing with one half of the IO
type CommandReadStream = Framed<WrappedStream, VcmiCommand, (), Json<VcmiCommand, ()>>;
type CommandWriteStream = Framed<WrappedSink, (), VcmiCommand, Json<(), VcmiCommand>>;

type ReplyReadStream = Framed<WrappedStream, VcmiReply, (), Json<VcmiReply, ()>>;
type ReplyWriteStream = Framed<WrappedSink, (), VcmiReply, Json<(), VcmiReply>>;

pub fn wrap_to_command_read_reply_write(
    stream: TcpStream,
) -> (CommandReadStream, ReplyWriteStream) {
    let (read, write) = stream.into_split();
    let codec = LengthDelimitedCodec::builder()
        .length_field_length(8)
        .length_field_type::<u64>()
        .new_codec();
    let stream = WrappedStream::new(read, codec.clone());
    let sink = WrappedSink::new(write, codec);
    (
        CommandReadStream::new(stream, Json::default()),
        ReplyWriteStream::new(sink, Json::default()),
    )
}

pub fn split_to_reply_read_command_write(
    stream: TcpStream,
) -> (ReplyReadStream, CommandWriteStream) {
    let (read, write) = stream.into_split();
    let codec = LengthDelimitedCodec::builder()
        .length_field_length(8)
        .length_field_type::<u64>()
        .new_codec();
    let stream = WrappedStream::new(read, codec.clone());
    let sink = WrappedSink::new(write, codec);
    (
        ReplyReadStream::new(stream, Json::default()),
        CommandWriteStream::new(sink, Json::default()),
    )
}
