/*
 * CConfigHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class Settings;
class SettingsListener;

/// Main storage of game settings
class DLL_LINKAGE SettingsStorage
{
	//Helper struct to access specific node either via chain of operator[] or with one operator() (vector)
	template<typename Accessor>
	struct DLL_LINKAGE NodeAccessor
	{
		SettingsStorage & parent;
		std::vector<std::string> path;

		NodeAccessor(SettingsStorage & _parent, std::vector<std::string> _path);
		NodeAccessor<Accessor> operator[](const std::string & nextNode) const;
		NodeAccessor<Accessor> operator () (std::vector<std::string> _path) const;
		operator Accessor() const;
	};

	std::set<SettingsListener*> listeners;
	JsonNode config;

	JsonNode & getNode(const std::vector<std::string> & path);

	// Calls all required listeners
	void invalidateNode(const std::vector<std::string> &changedPath);

	Settings get(const std::vector<std::string> & path);

public:
	// Initialize config structure
	SettingsStorage();
	void init();
	
	// Get write access to config node at path
	const NodeAccessor<Settings> write;

	// Get access to listener at path
	const NodeAccessor<SettingsListener> listen;

	//Read access, see JsonNode::operator[]
	const JsonNode & operator[](const std::string & value) const;
	const JsonNode & toJsonNode() const;

	friend class SettingsListener;
	friend class Settings;
};

/// Class for listening changes in specific part of configuration (e.g. change of music volume)
class DLL_LINKAGE SettingsListener
{
	SettingsStorage &parent;
	// Path to this node
	std::vector<std::string> path;
	// Callback
	std::function<void(const JsonNode&)> callback;

	SettingsListener(SettingsStorage & _parent, std::vector<std::string> _path);

	// Executes callback if changedpath begins with path
	void nodeInvalidated(const std::vector<std::string> & changedPath);

public:
	SettingsListener(const SettingsListener &sl);
	~SettingsListener();

	// assign callback function
	void operator()(std::function<void(const JsonNode&)> _callback);

	friend class SettingsStorage;
};

/// System options, provides write access to config tree with auto-saving on change
class DLL_LINKAGE Settings
{
	SettingsStorage &parent;
	//path to this node
	std::vector<std::string> path;
	JsonNode &node;
	JsonNode copy;
	
	//Get access to node pointed by path
	Settings(SettingsStorage &_parent, const std::vector<std::string> &_path);

public:
	//Saves config if it was modified
	~Settings();

	//Returns node selected during construction
	JsonNode* operator ->();
	const JsonNode* operator ->() const;

	//Helper, replaces JsonNode::operator[]
	JsonNode & operator[](const std::string & value);
	const JsonNode & operator[](const std::string & value) const;

	friend class SettingsStorage;
};

extern DLL_LINKAGE SettingsStorage settings;

VCMI_LIB_NAMESPACE_END
