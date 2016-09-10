
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
	: public IBinaryReader, public IBinaryWriter
{
	CConnection(void);

	void init();
	void reportState(CLogger * out) override;
public:
	BinaryDeserializer iser;
	BinarySerializer oser;

	boost::mutex *rmx, *wmx; // read/write mutexes
	TSocket * socket;
	bool logging;
	bool connected;
	bool myEndianess, contactEndianess; //true if little endian, if endianness is different we'll have to revert received multi-byte vars
	boost::asio::io_service *io_service;
	std::string name; //who uses this connection

	int connectionID;
	boost::thread *handler;

	bool receivedStop, sendStop;

	CConnection(std::string host, std::string port, std::string Name);
	CConnection(TAcceptor * acceptor, boost::asio::io_service *Io_service, std::string Name);
	CConnection(TSocket * Socket, std::string Name); //use immediately after accepting connection into socket

	int write(const void * data, unsigned size) override;
	int read(void * data, unsigned size) override;
	void close();
	bool isOpen() const;
	template<class T>
	CConnection &operator&(const T&);
	virtual ~CConnection(void);

	CPack *retreivePack(); //gets from server next pack (allocates it with new)
	void sendPackToServer(const CPack &pack, PlayerColor player, ui32 requestID);

	void disableStackSendingByID();
	void enableStackSendingByID();
	void disableSmartPointerSerialization();
	void enableSmartPointerSerializatoin();
	void disableSmartVectorMemberSerialization();
	void enableSmartVectorMemberSerializatoin();

	void prepareForSendingHeroes(); //disables sending vectorized, enables smart pointer serialization, clears saved/loaded ptr cache
	void enterPregameConnectionMode();

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

DLL_LINKAGE std::ostream &operator<<(std::ostream &str, const CConnection &cpc);
