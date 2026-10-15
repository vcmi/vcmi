/*
 * EditorObjectCatalog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/constants/EntityIdentifiers.h"

class CGObjectInstance;
class IGameInfoCallback;
class ObjectTemplate;

/// Tree of all the objects that can be put on the map, grouped like in the object browser
class EditorObjectCatalog
{
public:
	struct Node
	{
		std::string text;
		/// Identifier of the object type, searched by the filter together with the text
		std::string typeName;
		MapObjectID id;
		MapObjectSubID subId;
		size_t templateIndex = 0;
		bool leaf = false;
		std::vector<Node> children;
	};

	/// Visible row of the tree
	struct Row
	{
		const Node * node;
		int depth;
	};

private:
	std::vector<Node> groups;
	std::set<MapObjectID> catalogued;
	std::set<const Node *> expanded;

	void addGroup(const std::string & groupName, bool staticOnly, IGameInfoCallback * cb);
	void addGroup(const std::string & groupName, bool useCustomName, bool staticOnly, MapObjectID id, IGameInfoCallback * cb);
	Node & findOrCreateGroup(const std::string & groupName);

	bool matches(const Node & node, const std::string & filter, const TerrainId & terrain, bool parentMatchesText) const;
	void collect(const Node & node, int depth, const std::string & filter, const TerrainId & terrain, bool parentMatchesText, std::vector<Row> & rows) const;

public:
	explicit EditorObjectCatalog(IGameInfoCallback * cb);

	/// Rows of the tree that are visible for the filters. While a text filter is used all the groups are expanded
	std::vector<Row> getRows(const std::string & filter, const TerrainId & terrain) const;

	bool isExpanded(const Node & node) const;
	void toggleExpanded(const Node & node);

	static std::shared_ptr<const ObjectTemplate> getTemplate(const Node & leaf);
	static std::shared_ptr<CGObjectInstance> createObject(const Node & leaf, IGameInfoCallback * cb);
};
