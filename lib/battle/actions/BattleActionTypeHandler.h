/*
 * BattleActionTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class BattleActionType;
class LibraryLoadContext;

/// Registry of battle action types, with the identifiers that name them
class DLL_LINKAGE BattleActionTypeHandler : boost::noncopyable
{
	std::vector<const BattleActionType *> typesById;
	/// in registration order, which is the order of collected options
	std::vector<const BattleActionType *> types;
	std::vector<std::string> jsonKeys;

public:
	explicit BattleActionTypeHandler(LibraryLoadContext & context);

	/// Type with the id, or nullptr if no type has it
	const BattleActionType * getById(int32_t id) const;
	const std::vector<const BattleActionType *> & getAll() const;
	/// Full identifier of the type with the id. Throws if no type has it
	const std::string & getJsonKey(int32_t id) const;
};
