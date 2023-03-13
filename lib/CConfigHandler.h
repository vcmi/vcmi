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

namespace config
{
	struct DLL_LINKAGE ButtonInfo
	{
		std::string defName;
		std::vector<std::string> additionalDefs;
		int x, y; //position on the screen
		bool playerColoured; //if true button will be colored to main player's color (works properly only for appropriate 8bpp graphics)
	};
	/// Struct which holds data about position of several GUI elements at the adventure map screen
	struct DLL_LINKAGE AdventureMapConfig
	{
		//minimap properties
		int minimapX, minimapY, minimapW, minimapH;
		//statusbar
		int statusbarX, statusbarY; //pos
		std::string statusbarG; //graphic name
		//resdatabar
		int resdatabarX, resdatabarY, resDist, resDateDist, resOffsetX, resOffsetY; //pos
		std::string resdatabarG; //graphic name
		//infobox
		int infoboxX, infoboxY;
		//advmap
		int advmapX, advmapY, advmapW, advmapH;
		bool smoothMove;
		bool puzzleSepia;
		bool screenFading;
		bool objectFading;
		//general properties
		std::string mainGraphic;
		std::string worldViewGraphic;
		//buttons
		ButtonInfo kingOverview, underground, questlog,	sleepWake, moveHero, spellbook,	advOptions,
			sysOptions,	nextHero, endTurn;
		//hero list
		int hlistX, hlistY, hlistSize;
		std::string hlistMB, hlistMN, hlistAU, hlistAD;
		//town list
		int tlistX, tlistY, tlistSize;
		std::string tlistAU, tlistAD;
		//gems
		int gemX[4], gemY[4];
		std::vector<std::string> gemG;
		//in-game console
		int inputLineLength, outputLineLength;
		//kingdom overview
		int overviewPics, overviewSize; //pic count in def and count of visible slots
		std::string overviewBg; //background name
	};
	struct DLL_LINKAGE GUIOptions
	{
		AdventureMapConfig ac;
	};
	/// Handles adventure map screen settings
	class DLL_LINKAGE CConfigHandler
	{
		GUIOptions *current; // pointer to current gui options

	public:
		typedef std::map<std::pair<int,int>, GUIOptions > GuiOptionsMap;
		GuiOptionsMap guiOptions;
		void init();
		CConfigHandler();

		GUIOptions *go() { return current; };
		void SetResolution(int x, int y)
		{
			std::pair<int,int> index(x, y);
			if (guiOptions.count(index) == 0)
				current = nullptr;
			else
				current = &guiOptions.at(index);
		}
	};
}

extern DLL_LINKAGE SettingsStorage settings;
extern DLL_LINKAGE config::CConfigHandler conf;

VCMI_LIB_NAMESPACE_END
