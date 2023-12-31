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
class FactionID;
class TerrainId;

class DLL_LINKAGE IConstBonusProvider
{
public:
	virtual const IBonusBearer * getBonusBearer() const = 0;
};

class DLL_LINKAGE INativeTerrainProvider
{
public:
	virtual TerrainId getNativeTerrain() const = 0;
	virtual FactionID getFaction() const = 0;
	virtual bool isNativeTerrain(TerrainId terrain) const;
};

class DLL_LINKAGE Entity
{
public:
	using IconRegistar = std::function<void(int32_t index, int32_t group, const std::string & listName, const std::string & imageName)>;

	virtual ~Entity() = default;

	virtual int32_t getIndex() const = 0;
	virtual int32_t getIconIndex() const = 0;
	virtual std::string getJsonKey() const = 0;
	virtual std::string getNameTranslated() const = 0;
	virtual std::string getNameTextID() const = 0;

	virtual void registerIcons(const IconRegistar & cb) const = 0;
};

template <typename IdType>
class DLL_LINKAGE EntityT : public Entity
{
public:
	using IdentifierType = IdType;

	virtual IdType getId() const = 0;
};

template <typename IdType>
class DLL_LINKAGE EntityWithBonuses : public EntityT<IdType>, public IConstBonusProvider
{
};

VCMI_LIB_NAMESPACE_END
