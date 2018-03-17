/*
 * Entity.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once


class DLL_LINKAGE Entity
{
public:
	virtual ~Entity() = default;

	virtual int32_t getIndex() const = 0;
	virtual const std::string & getJsonKey() const = 0;
	virtual const std::string & getName() const = 0;
};

template <typename IdType>
class DLL_LINKAGE EntityT : public Entity
{
public:
	virtual IdType getId() const = 0;
};
