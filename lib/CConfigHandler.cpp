/*
 * CConfigHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CConfigHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/FileStream.h"
#include "../lib/GameConstants.h"
#include "../lib/VCMIDirs.h"

VCMI_LIB_NAMESPACE_BEGIN

using namespace config;

SettingsStorage settings;
CConfigHandler conf;

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor>::NodeAccessor(SettingsStorage & _parent, std::vector<std::string> _path):
	parent(_parent),
	path(_path)
{
}

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor> SettingsStorage::NodeAccessor<Accessor>::operator [](std::string nextNode) const
{
	std::vector<std::string> newPath = path;
	newPath.push_back(nextNode);
	return NodeAccessor(parent, newPath);
}

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor>::operator Accessor() const
{
	return Accessor(parent, path);
}

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor> SettingsStorage::NodeAccessor<Accessor>::operator () (std::vector<std::string> _path) const
{
	std::vector<std::string> newPath = path;
	newPath.insert( newPath.end(), _path.begin(), _path.end());
	return NodeAccessor(parent, newPath);
}

SettingsStorage::SettingsStorage():
	write(NodeAccessor<Settings>(*this, std::vector<std::string>() )),
	listen(NodeAccessor<SettingsListener>(*this, std::vector<std::string>() ))
{
}

void SettingsStorage::init()
{
	std::string confName = "config/settings.json";

	JsonUtils::assembleFromFiles(confName).swap(config);

	// Probably new install. Create config file to save settings to
	if (!CResourceHandler::get("local")->existsResource(ResourceID(confName)))
		CResourceHandler::get("local")->createResource(confName);

	JsonUtils::maximize(config, "vcmi:settings");
	JsonUtils::validate(config, "vcmi:settings", "settings");
}

void SettingsStorage::invalidateNode(const std::vector<std::string> &changedPath)
{
	for(SettingsListener * listener : listeners)
		listener->nodeInvalidated(changedPath);

	JsonNode savedConf = config;
	savedConf.Struct().erase("session");
	JsonUtils::minimize(savedConf, "vcmi:settings");

	FileStream file(*CResourceHandler::get()->getResourceName(ResourceID("config/settings.json")), std::ofstream::out | std::ofstream::trunc);
	file << savedConf.toJson();
}

JsonNode & SettingsStorage::getNode(std::vector<std::string> path)
{
	JsonNode *node = &config;
	for(std::string& value : path)
		node = &(*node)[value];

	return *node;
}

Settings SettingsStorage::get(std::vector<std::string> path)
{
	return Settings(*this, path);
}

const JsonNode & SettingsStorage::operator [](std::string value) const
{
	return config[value];
}

const JsonNode & SettingsStorage::toJsonNode() const
{
	return config;
}

SettingsListener::SettingsListener(SettingsStorage &_parent, const std::vector<std::string> &_path):
	parent(_parent),
	path(_path)
{
	parent.listeners.insert(this);
}

SettingsListener::SettingsListener(const SettingsListener &sl):
	parent(sl.parent),
	path(sl.path),
	callback(sl.callback)
{
	parent.listeners.insert(this);
}

SettingsListener::~SettingsListener()
{
	parent.listeners.erase(this);
}

void SettingsListener::nodeInvalidated(const std::vector<std::string> &changedPath)
{
	if (!callback)
		return;

	size_t min = std::min(path.size(), changedPath.size());
	size_t mismatch = std::mismatch(path.begin(), path.begin()+min, changedPath.begin()).first - path.begin();

	if (min == mismatch)
		callback(parent.getNode(path));
}

void SettingsListener::operator() (std::function<void(const JsonNode&)> _callback)
{
	callback = _callback;
}

Settings::Settings(SettingsStorage &_parent, const std::vector<std::string> &_path):
	parent(_parent),
	path(_path),
	node(_parent.getNode(_path)),
	copy(_parent.getNode(_path))
{
}

Settings::~Settings()
{
	if (node != copy)
		parent.invalidateNode(path);
}

JsonNode* Settings::operator -> ()
{
	return &node;
}

const JsonNode* Settings::operator ->() const
{
	return &node;
}

const JsonNode& Settings::operator [](std::string value) const
{
	return node[value];
}

JsonNode& Settings::operator [](std::string value)
{
	return node[value];
}
//
// template DLL_LINKAGE struct SettingsStorage::NodeAccessor<SettingsListener>;
// template DLL_LINKAGE struct SettingsStorage::NodeAccessor<Settings>;

static void setButton(ButtonInfo &button, const JsonNode &g)
{
	button.x = static_cast<int>(g["x"].Float());
	button.y = static_cast<int>(g["y"].Float());
	button.playerColoured = g["playerColoured"].Float();
	button.defName = g["graphic"].String();

	if (!g["additionalDefs"].isNull()) {
		const JsonVector &defs_vec = g["additionalDefs"].Vector();

		for(const JsonNode &def : defs_vec) {
			button.additionalDefs.push_back(def.String());
		}
	}
}

static void setGem(AdventureMapConfig &ac, const int gem, const JsonNode &g)
{
	ac.gemX[gem] = static_cast<int>(g["x"].Float());
	ac.gemY[gem] = static_cast<int>(g["y"].Float());
	ac.gemG.push_back(g["graphic"].String());
}

CConfigHandler::CConfigHandler()
	: current(nullptr)
{
}

CConfigHandler::~CConfigHandler() = default;

void config::CConfigHandler::init()
{
	/* Read resolutions. */
	const JsonNode config(ResourceID("config/resolutions.json"));
	const JsonVector &guisettings_vec = config["GUISettings"].Vector();

	for(const JsonNode &g : guisettings_vec)
	{
		std::pair<int,int> curRes((int)g["resolution"]["x"].Float(), (int)g["resolution"]["y"].Float());
		GUIOptions *current = &conf.guiOptions[curRes];

		current->ac.inputLineLength =  static_cast<int>(g["InGameConsole"]["maxInputPerLine"].Float());
		current->ac.outputLineLength = static_cast<int>(g["InGameConsole"]["maxOutputPerLine"].Float());

		current->ac.advmapX = static_cast<int>(g["AdvMap"]["x"].Float());
		current->ac.advmapY = static_cast<int>(g["AdvMap"]["y"].Float());
		current->ac.advmapW = static_cast<int>(g["AdvMap"]["width"].Float());
		current->ac.advmapH = static_cast<int>(g["AdvMap"]["height"].Float());
		current->ac.smoothMove = g["AdvMap"]["smoothMove"].Float();
		current->ac.puzzleSepia = g["AdvMap"]["puzzleSepia"].Float();
		current->ac.screenFading = g["AdvMap"]["screenFading"].isNull() ? true : g["AdvMap"]["screenFading"].Float(); // enabled by default
		current->ac.objectFading = g["AdvMap"]["objectFading"].isNull() ? true : g["AdvMap"]["objectFading"].Float();

		current->ac.infoboxX = static_cast<int>(g["InfoBox"]["x"].Float());
		current->ac.infoboxY = static_cast<int>(g["InfoBox"]["y"].Float());

		setGem(current->ac, 0, g["gem0"]);
		setGem(current->ac, 1, g["gem1"]);
		setGem(current->ac, 2, g["gem2"]);
		setGem(current->ac, 3, g["gem3"]);

		current->ac.mainGraphic = g["background"].String();
		current->ac.worldViewGraphic = g["backgroundWorldView"].String();

		current->ac.hlistX =    static_cast<int>(g["HeroList"]["x"].Float());
		current->ac.hlistY =    static_cast<int>(g["HeroList"]["y"].Float());
		current->ac.hlistSize = static_cast<int>(g["HeroList"]["size"].Float());
		current->ac.hlistMB = g["HeroList"]["movePoints"].String();
		current->ac.hlistMN = g["HeroList"]["manaPoints"].String();
		current->ac.hlistAU = g["HeroList"]["arrowUp"].String();
		current->ac.hlistAD = g["HeroList"]["arrowDown"].String();

		current->ac.tlistX =    static_cast<int>(g["TownList"]["x"].Float());
		current->ac.tlistY =    static_cast<int>(g["TownList"]["y"].Float());
		current->ac.tlistSize = static_cast<int>(g["TownList"]["size"].Float());
		current->ac.tlistAU = g["TownList"]["arrowUp"].String();
		current->ac.tlistAD = g["TownList"]["arrowDown"].String();

		current->ac.minimapW = static_cast<int>(g["Minimap"]["width"].Float());
		current->ac.minimapH = static_cast<int>(g["Minimap"]["height"].Float());
		current->ac.minimapX = static_cast<int>(g["Minimap"]["x"].Float());
		current->ac.minimapY = static_cast<int>(g["Minimap"]["y"].Float());

		current->ac.overviewPics = static_cast<int>(g["Overview"]["pics"].Float());
		current->ac.overviewSize = static_cast<int>(g["Overview"]["size"].Float());
		current->ac.overviewBg = g["Overview"]["graphic"].String();

		current->ac.statusbarX = static_cast<int>(g["Statusbar"]["x"].Float());
		current->ac.statusbarY = static_cast<int>(g["Statusbar"]["y"].Float());
		current->ac.statusbarG = g["Statusbar"]["graphic"].String();

		current->ac.resdatabarX = static_cast<int>(g["ResDataBar"]["x"].Float());
		current->ac.resdatabarY = static_cast<int>(g["ResDataBar"]["y"].Float());
		current->ac.resOffsetX =  static_cast<int>(g["ResDataBar"]["offsetX"].Float());
		current->ac.resOffsetY =  static_cast<int>(g["ResDataBar"]["offsetY"].Float());
		current->ac.resDist =     static_cast<int>(g["ResDataBar"]["resSpace"].Float());
		current->ac.resDateDist = static_cast<int>(g["ResDataBar"]["resDateSpace"].Float());
		current->ac.resdatabarG = g["ResDataBar"]["graphic"].String();

		setButton(current->ac.kingOverview, g["ButtonKingdomOv"]);
		setButton(current->ac.underground, g["ButtonUnderground"]);
		setButton(current->ac.questlog, g["ButtonQuestLog"]);
		setButton(current->ac.sleepWake, g["ButtonSleepWake"]);
		setButton(current->ac.moveHero, g["ButtonMoveHero"]);
		setButton(current->ac.spellbook, g["ButtonSpellbook"]);
		setButton(current->ac.advOptions, g["ButtonAdvOptions"]);
		setButton(current->ac.sysOptions, g["ButtonSysOptions"]);
		setButton(current->ac.nextHero, g["ButtonNextHero"]);
		setButton(current->ac.endTurn, g["ButtonEndTurn"]);
	}

	const JsonNode& screenRes = settings["video"]["screenRes"];

	SetResolution((int)screenRes["width"].Float(), (int)screenRes["height"].Float());
}

// Force instantiation of the SettingsStorage::NodeAccessor class template.
// That way method definitions can sit in the cpp file
template struct SettingsStorage::NodeAccessor<SettingsListener>;
template struct SettingsStorage::NodeAccessor<Settings>;

VCMI_LIB_NAMESPACE_END
