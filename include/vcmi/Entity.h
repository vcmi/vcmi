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

VCMI_LIB_NAMESPACE_BEGIN

class IBonusBearer;

class DLL_LINKAGE Entity
{
public:
	using IconRegistar = std::function<void(int32_t index, int32_t group, const std::string & listName, const std::string & imageName)>;

	virtual ~Entity() = default;

	virtual int32_t getIndex() const = 0;
	virtual int32_t getIconIndex() const = 0;
	virtual const std::string & getJsonKey() const = 0;
	virtual const std::string & getName() const = 0;

	virtual void registerIcons(const IconRegistar & cb) const = 0;
};

template <typename IdType>
class DLL_LINKAGE EntityT : public Entity
{
public:
	virtual IdType getId() const = 0;
};

template <typename IdType>
class DLL_LINKAGE EntityWithBonuses : public EntityT<IdType>
{
public:
	virtual const IBonusBearer * accessBonuses() const = 0;
};

VCMI_LIB_NAMESPACE_END
