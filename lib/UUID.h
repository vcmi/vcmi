/*
 * UUID.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class DLL_LINKAGE UUID
{
public:
	boost::uuids::uuid getID() const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID;
	}
private:
	boost::uuids::uuid ID = generateID();
	static boost::uuids::uuid generateID();
};
