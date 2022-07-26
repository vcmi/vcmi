/*
 * EntityService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class Entity;

class DLL_LINKAGE EntityService
{
public:
	virtual ~EntityService() = default;

	virtual const Entity * getBaseByIndex(const int32_t index) const = 0;
	virtual void forEachBase(const std::function<void(const Entity * entity, bool & stop)> & cb) const = 0;
};

template <typename IdType, typename EntityType>
class DLL_LINKAGE EntityServiceT : public EntityService
{
public:
	virtual const EntityType * getById(const IdType & id) const = 0;
	virtual const EntityType * getByIndex(const int32_t index) const = 0;

	virtual void forEach(const std::function<void(const EntityType * entity, bool & stop)> & cb) const = 0;
};

VCMI_LIB_NAMESPACE_END
