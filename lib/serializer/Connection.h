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

#if BOOST_VERSION >= 107000  // Boost version >= 1.70
#include <boost/asio.hpp>
typedef boost::asio::basic_stream_socket < boost::asio::ip::tcp > TSocket;
typedef boost::asio::basic_socket_acceptor < boost::asio::ip::tcp > TAcceptor;
#else
namespace boost
{
	namespace asio
	{
		namespace ip
		{
			class tcp;
		}

#if BOOST_VERSION >= 106600  // Boost version >= 1.66
		class io_context;
		typedef io_context io_service;
#else
		class io_service;
#endif

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
#endif


VCMI_LIB_NAMESPACE_BEGIN

struct CPack;
struct ConnectionBuffers;

/// Main class for network communication
/// Allows establishing connection and bidirectional read-write
class DLL_LINKAGE CConnection
	: public IBinaryReader, public IBinaryWriter, public std::enable_shared_from_this<CConnection>
{
	void init();
	void reportState(vstd::CLoggerBase * out) override;

	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
	void flushBuffers();

	std::shared_ptr<boost::asio::io_service> io_service; //can be empty if connection made from socket

	bool enableBufferedWrite;
	bool enableBufferedRead;
	std::unique_ptr<ConnectionBuffers> connectionBuffers;

public:
	BinaryDeserializer iser;
	BinarySerializer oser;

	std::shared_ptr<boost::mutex> mutexRead;
	std::shared_ptr<boost::mutex> mutexWrite;
	std::shared_ptr<TSocket> socket;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if endianness is different we'll have to revert received multi-byte vars
	std::string contactUuid;
	std::string name; //who uses this connection
	std::string uuid;

	int connectionID;
	std::shared_ptr<boost::thread> handler;

	CConnection(const std::string & host, ui16 port, std::string Name, std::string UUID);
	CConnection(const std::shared_ptr<TAcceptor> & acceptor, const std::shared_ptr<boost::asio::io_service> & Io_service, std::string Name, std::string UUID);
	CConnection(std::shared_ptr<TSocket> Socket, std::string Name, std::string UUID); //use immediately after accepting connection into socket

	void close();
	bool isOpen() const;
	template<class T>
	CConnection &operator&(const T&);
	virtual ~CConnection();

	CPack * retrievePack();
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

VCMI_LIB_NAMESPACE_END
