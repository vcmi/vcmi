/*
 * IConnection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct CPack;

class DLL_LINKAGE IGameConnection : boost::noncopyable
{
public:
	virtual void sendPack(const CPack & pack) = 0;
	virtual int getConnectionID() const = 0;
};

VCMI_LIB_NAMESPACE_END
