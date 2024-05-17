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

#include "BinaryDeserializer.h"
#include "BinarySerializer.h"

#include "../gameState/CGameState.h"
#include "../networkPacks/NetPacksBase.h"
#include "../network/NetworkInterface.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE ConnectionPackWriter final : public IBinaryWriter
{
public:
	std::vector<std::byte> buffer;

	int write(const std::byte * data, unsigned size) final;
};

class DLL_LINKAGE ConnectionPackReader final : public IBinaryReader
{
public:
	const std::vector<std::byte> * buffer;
	size_t position;

	int read(std::byte * data, unsigned size) final;
};

int ConnectionPackWriter::write(const std::byte * data, unsigned size)
{
	buffer.insert(buffer.end(), data, data + size);
	return size;
}

int ConnectionPackReader::read(std::byte * data, unsigned size)
{
	if (position + size > buffer->size())
		throw std::runtime_error("End of file reached when reading received network pack!");

	std::copy_n(buffer->begin() + position, size, data);
	position += size;
	return size;
}

CConnection::CConnection(std::weak_ptr<INetworkConnection> networkConnection)
	: networkConnection(networkConnection)
	, packReader(std::make_unique<ConnectionPackReader>())
	, packWriter(std::make_unique<ConnectionPackWriter>())
	, deserializer(std::make_unique<BinaryDeserializer>(packReader.get()))
	, serializer(std::make_unique<BinarySerializer>(packWriter.get()))
	, connectionID(-1)
{
	assert(networkConnection.lock() != nullptr);

	enterLobbyConnectionMode();
	deserializer->version = ESerializationVersion::CURRENT;
}

CConnection::~CConnection() = default;

void CConnection::sendPack(const CPack * pack)
{
	boost::mutex::scoped_lock lock(writeMutex);

	auto connectionPtr = networkConnection.lock();

	if (!connectionPtr)
		throw std::runtime_error("Attempt to send packet on a closed connection!");

	*serializer & pack;

	logNetwork->trace("Sending a pack of type %s", typeid(*pack).name());

	connectionPtr->sendPacket(packWriter->buffer);
	packWriter->buffer.clear();
}

CPack * CConnection::retrievePack(const std::vector<std::byte> & data)
{
	CPack * result;

	packReader->buffer = &data;
	packReader->position = 0;

	*deserializer & result;

	if (result == nullptr)
		throw std::runtime_error("Failed to retrieve pack!");

	if (packReader->position != data.size())
		throw std::runtime_error("Failed to retrieve pack! Not all data has been read!");

	logNetwork->trace("Received CPack of type %s", typeid(*result).name());
	return result;
}

bool CConnection::isMyConnection(const std::shared_ptr<INetworkConnection> & otherConnection) const
{
	return otherConnection != nullptr && networkConnection.lock() == otherConnection;
}

std::shared_ptr<INetworkConnection> CConnection::getConnection()
{
	return networkConnection.lock();
}

void CConnection::disableStackSendingByID()
{
	packReader->sendStackInstanceByIds = false;
	packWriter->sendStackInstanceByIds = false;
}

void CConnection::enableStackSendingByID()
{
	packReader->sendStackInstanceByIds = true;
	packWriter->sendStackInstanceByIds = true;
}

void CConnection::enterLobbyConnectionMode()
{
	deserializer->loadedPointers.clear();
	serializer->savedPointers.clear();
	disableSmartVectorMemberSerialization();
	disableSmartPointerSerialization();
	disableStackSendingByID();
}

void CConnection::setCallback(IGameCallback * cb)
{
	deserializer->cb = cb;
}

void CConnection::enterGameplayConnectionMode(CGameState * gs)
{
	enableStackSendingByID();
	disableSmartPointerSerialization();

	setCallback(gs->callback);
	enableSmartVectorMemberSerializatoin(gs);
}

void CConnection::disableSmartPointerSerialization()
{
	deserializer->smartPointerSerialization = false;
	serializer->smartPointerSerialization = false;
}

void CConnection::enableSmartPointerSerialization()
{
	deserializer->smartPointerSerialization = true;
	serializer->smartPointerSerialization = true;
}

void CConnection::disableSmartVectorMemberSerialization()
{
	packReader->smartVectorMembersSerialization = false;
	packWriter->smartVectorMembersSerialization = false;
}

void CConnection::enableSmartVectorMemberSerializatoin(CGameState * gs)
{
	packWriter->addStdVecItems(gs);
	packReader->addStdVecItems(gs);
}

VCMI_LIB_NAMESPACE_END
