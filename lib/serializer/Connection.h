/*
 * Connection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BinaryDeserializer.h"
#include "BinarySerializer.h"

struct CPack;

namespace boost
{
	namespace asio
	{
		namespace ip
		{
			class tcp;
		}
		class io_service;

		template <typename Protocol> class stream_socket_service;
		template <typename Protocol,typename StreamSocketService>
		class basic_stream_socket;

		template <typename Protocol> class socket_acceptor_service;
		template <typename Protocol,typename SocketAcceptorService>
		class basic_socket_acceptor;
	}
	class mutex;
}

typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp , boost::asio::stream_socket_service<boost::asio::ip::tcp>  > TSocket;
typedef boost::asio::basic_socket_acceptor<boost::asio::ip::tcp, boost::asio::socket_acceptor_service<boost::asio::ip::tcp> > TAcceptor;

/// Main class for network communication
/// Allows establishing connection and bidirectional read-write
class DLL_LINKAGE CConnection
	: public IBinaryReader, public IBinaryWriter, public std::enable_shared_from_this<CConnection>
{
	CConnection();

	void init();
	void reportState(vstd::CLoggerBase * out) override;

	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
public:
	BinaryDeserializer iser;
	BinarySerializer oser;

	boost::mutex *rmx, *wmx; // read/write mutexes
	TSocket * socket;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if endianness is different we'll have to revert received multi-byte vars
	std::string contactUuid;
	boost::asio::io_service *io_service;
	std::string name; //who uses this connection
	std::string uuid;

	int connectionID;
	boost::thread *handler;

	CConnection(std::string host, ui16 port, std::string Name, std::string UUID);
	CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name, std::string UUID);
	CConnection(TSocket * Socket, std::string Name, std::string UUID); //use immediately after accepting connection into socket

	void close();
	bool isOpen() const;
	template<class T>
	CConnection &operator&(const T&);
	virtual ~CConnection();

	CPack * retreivePack();
	void sendPack(const CPack * pack);

	void disableStackSendingByID();
	void enableStackSendingByID();
	void disableSmartPointerSerialization();
	void enableSmartPointerSerialization();
	void disableSmartVectorMemberSerialization();
	void enableSmartVectorMemberSerializatoin();

	void enterLobbyConnectionMode();
	void enterGameplayConnectionMode(CGameState * gs);

	std::string toString() const;

	template<class T>
	CConnection & operator>>(T &t)
	{
		iser & t;
		return * this;
	}

	template<class T>
	CConnection & operator<<(const T &t)
	{
		oser & t;
		return * this;
	}
};
