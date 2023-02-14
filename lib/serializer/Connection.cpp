/*
 * Connection.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Connection.h"

#include "../registerTypes/RegisterTypes.h"
#include "../mapping/CMap.h"
#include "../CGameState.h"

#include <boost/asio.hpp>

VCMI_LIB_NAMESPACE_BEGIN

using namespace boost;
using namespace boost::asio::ip;

struct ConnectionBuffers
{
	boost::asio::streambuf readBuffer;
	boost::asio::streambuf writeBuffer;
};

void CConnection::init()
{
	enableBufferedWrite = false;
	enableBufferedRead = false;
	connectionBuffers = std::make_unique<ConnectionBuffers>();

	socket->set_option(boost::asio::ip::tcp::no_delay(true));
    try
    {
        socket->set_option(boost::asio::socket_base::send_buffer_size(4194304));
        socket->set_option(boost::asio::socket_base::receive_buffer_size(4194304));
    }
    catch (const boost::system::system_error & e)
    {
        logNetwork->error("error setting socket option: %s", e.what());
    }

	enableSmartPointerSerialization();
	disableStackSendingByID();
	registerTypes(iser);
	registerTypes(oser);
#ifndef VCMI_ENDIAN_BIG
	myEndianess = true;
#else
	myEndianess = false;
#endif
	connected = true;
	std::string pom;
	//we got connection
	oser & std::string("Aiya!\n") & name & uuid & myEndianess; //identify ourselves
	iser & pom & pom & contactUuid & contactEndianess;
	logNetwork->info("Established connection with %s. UUID: %s", pom, contactUuid);
	mutexRead = std::make_shared<boost::mutex>();
	mutexWrite = std::make_shared<boost::mutex>();

	iser.fileVersion = SERIALIZATION_VERSION;
}

CConnection::CConnection(const std::string & host, ui16 port, std::string Name, std::string UUID):
	io_service(std::make_shared<asio::io_service>()),
	iser(this),
	oser(this),
	name(std::move(Name)),
	uuid(std::move(UUID))
{
	int i = 0;
	boost::system::error_code error = asio::error::host_not_found;
	socket = std::make_shared<tcp::socket>(*io_service);

	tcp::resolver resolver(*io_service);
	tcp::resolver::iterator end;
	tcp::resolver::iterator pom;
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(tcp::resolver::query(host, std::to_string(port)), error);
	if(error)
	{
		logNetwork->error("Problem with resolving: \n%s", error.message());
		goto connerror1;
	}
	pom = endpoint_iterator;
	if(pom != end)
		logNetwork->info("Found endpoints:");
	else
	{
		logNetwork->error("Critical problem: No endpoints found!");
		goto connerror1;
	}
	while(pom != end)
	{
		logNetwork->info("\t%d:%s", i, (boost::asio::ip::tcp::endpoint&)*pom);
		pom++;
	}
	i=0;
	while(endpoint_iterator != end)
	{
		logNetwork->info("Trying connection to %s(%d)", (boost::asio::ip::tcp::endpoint&)*endpoint_iterator, i++);
		socket->connect(*endpoint_iterator, error);
		if(!error)
		{
			init();
			return;
		}
		else
		{
			logNetwork->error("Problem with connecting: %s", error.message());
		}
		endpoint_iterator++;
	}

	//we shouldn't be here - error handling
connerror1:
	logNetwork->error("Something went wrong... checking for error info");
	if(error)
		logNetwork->error(error.message());
	else
		logNetwork->error("No error info. ");
	throw std::runtime_error("Can't establish connection :(");
}
CConnection::CConnection(std::shared_ptr<TSocket> Socket, std::string Name, std::string UUID):
	iser(this),
	oser(this),
	socket(std::move(Socket)),
	name(std::move(Name)),
	uuid(std::move(UUID))
{
	init();
}
CConnection::CConnection(const std::shared_ptr<TAcceptor> & acceptor,
						 const std::shared_ptr<boost::asio::io_service> & io_service,
						 std::string Name,
						 std::string UUID):
	io_service(io_service),
	iser(this),
	oser(this),
	name(std::move(Name)),
	uuid(std::move(UUID))
{
	boost::system::error_code error = asio::error::host_not_found;
	socket = std::make_shared<tcp::socket>(*io_service);
	acceptor->accept(*socket,error);
	if (error)
	{
		logNetwork->error("Error on accepting: %s", error.message());
		socket.reset();
		throw std::runtime_error("Can't establish connection :(");
	}
	init();
}

void CConnection::flushBuffers()
{
	if(!enableBufferedWrite)
		return;

	try
	{
		asio::write(*socket, connectionBuffers->writeBuffer);
	}
	catch(...)
	{
		//connection has been lost
		connected = false;
		throw;
	}

	enableBufferedWrite = false;
}

int CConnection::write(const void * data, unsigned size)
{
	try
	{
		if(enableBufferedWrite)
		{
			std::ostream ostream(&connectionBuffers->writeBuffer);
		
			ostream.write(static_cast<const char *>(data), size);

			return size;
		}

		int ret = static_cast<int>(asio::write(*socket, asio::const_buffers_1(asio::const_buffer(data, size))));
		return ret;
	}
	catch(...)
	{
		//connection has been lost
		connected = false;
		throw;
	}
}

int CConnection::read(void * data, unsigned size)
{
	try
	{
		if(enableBufferedRead)
		{
			auto available = connectionBuffers->readBuffer.size();

			while(available < size)
			{
				auto bytesRead = socket->read_some(connectionBuffers->readBuffer.prepare(1024));
				connectionBuffers->readBuffer.commit(bytesRead);
				available = connectionBuffers->readBuffer.size();
			}

			std::istream istream(&connectionBuffers->readBuffer);

			istream.read(static_cast<char *>(data), size);

			return size;
		}

		int ret = static_cast<int>(asio::read(*socket,asio::mutable_buffers_1(asio::mutable_buffer(data,size))));
		return ret;
	}
	catch(...)
	{
		//connection has been lost
		connected = false;
		throw;
	}
}

CConnection::~CConnection()
{
	if(handler)
		handler->join();

	close();
}

template<class T>
CConnection & CConnection::operator&(const T &t) {
//	throw std::exception();
//XXX this is temporaly ? solution to fix gcc (4.3.3, other?) compilation
//    problem for more details contact t0@czlug.icis.pcz.pl or impono@gmail.com
//    do not remove this exception it shoudnt be called
	return *this;
}

void CConnection::close()
{
	if(socket)
	{
		socket->close();
		socket.reset();
	}
}

bool CConnection::isOpen() const
{
	return socket && connected;
}

void CConnection::reportState(vstd::CLoggerBase * out)
{
	out->debug("CConnection");
	if(socket && socket->is_open())
	{
		out->debug("\tWe have an open and valid socket");
		out->debug("\t %d bytes awaiting", socket->available());
	}
}

CPack * CConnection::retrievePack()
{
	enableBufferedRead = true;

	CPack * pack = nullptr;
	boost::unique_lock<boost::mutex> lock(*mutexRead);
	iser & pack;
	logNetwork->trace("Received CPack of type %s", (pack ? typeid(*pack).name() : "nullptr"));
	if(pack == nullptr)
	{
		logNetwork->error("Received a nullptr CPack! You should check whether client and server ABI matches.");
	}
	else
	{
		pack->c = this->shared_from_this();
	}

	enableBufferedRead = false;

	return pack;
}

void CConnection::sendPack(const CPack * pack)
{
	boost::unique_lock<boost::mutex> lock(*mutexWrite);
	logNetwork->trace("Sending a pack of type %s", typeid(*pack).name());

	enableBufferedWrite = true;

	oser & pack;

	flushBuffers();
}

void CConnection::disableStackSendingByID()
{
	CSerializer::sendStackInstanceByIds = false;
}

void CConnection::enableStackSendingByID()
{
	CSerializer::sendStackInstanceByIds = true;
}

void CConnection::disableSmartPointerSerialization()
{
	iser.smartPointerSerialization = oser.smartPointerSerialization = false;
}

void CConnection::enableSmartPointerSerialization()
{
	iser.smartPointerSerialization = oser.smartPointerSerialization = true;
}

void CConnection::enterLobbyConnectionMode()
{
	iser.loadedPointers.clear();
	oser.savedPointers.clear();
	disableSmartVectorMemberSerialization();
	disableSmartPointerSerialization();
}

void CConnection::enterGameplayConnectionMode(CGameState * gs)
{
	enableStackSendingByID();
	disableSmartPointerSerialization();
	addStdVecItems(gs);
}

void CConnection::disableSmartVectorMemberSerialization()
{
	CSerializer::smartVectorMembersSerialization = false;
}

void CConnection::enableSmartVectorMemberSerializatoin()
{
	CSerializer::smartVectorMembersSerialization = true;
}

std::string CConnection::toString() const
{
	boost::format fmt("Connection with %s (ID: %d UUID: %s)");
	fmt % name % connectionID % uuid;
	return fmt.str();
}

VCMI_LIB_NAMESPACE_END
