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

#include "filesystem/Filesystem.h"
#include "GameConstants.h"
#include "VCMIDirs.h"
#include "json/JsonUtils.h"

VCMI_LIB_NAMESPACE_BEGIN

SettingsStorage settings;
SettingsStorage persistentStorage;
SettingsStorage keyBindingsConfig;

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor>::NodeAccessor(SettingsStorage & _parent, std::vector<std::string> _path):
	parent(_parent),
	path(std::move(_path))
{
}

template<typename Accessor>
SettingsStorage::NodeAccessor<Accessor> SettingsStorage::NodeAccessor<Accessor>::operator[](const std::string & nextNode) const
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

SettingsStorage::~SettingsStorage()
{
	// hack for possible crash due to static destruction order (setting storage can be destroyed before all listeners have died)
	for(SettingsListener * listener : listeners)
		listener->terminate();
}

void SettingsStorage::init(const std::string & dataFilename, const std::string & schema)
{
	this->dataFilename = dataFilename;
	this->schema = schema;

	JsonPath confName = JsonPath::builtin(dataFilename);

	config = JsonUtils::assembleFromFiles(confName.getOriginalName());

	// Probably new install. Create config file to save settings to
	if (!CResourceHandler::get("local")->existsResource(confName))
	{
		CResourceHandler::get("local")->createResource(dataFilename);
		if(schema.empty())
			invalidateNode(std::vector<std::string>());
	}

	if(!schema.empty())
	{
		JsonUtils::maximize(config, schema);
		JsonUtils::validate(config, schema, "settings");
	}
}

void SettingsStorage::invalidateNode(const std::vector<std::string> &changedPath)
{
	for(SettingsListener * listener : listeners)
		listener->nodeInvalidated(changedPath);

	JsonNode savedConf = config;
	savedConf.Struct().erase("session");
	if(!schema.empty())
		JsonUtils::minimize(savedConf, schema);

	std::fstream file(CResourceHandler::get()->getResourceName(JsonPath::builtin(dataFilename))->c_str(), std::ofstream::out | std::ofstream::trunc);
	file << savedConf.toString();
}

JsonNode & SettingsStorage::getNode(const std::vector<std::string> & path)
{
	JsonNode *node = &config;
	for(const std::string & value : path)
		node = &(*node)[value];

	return *node;
}

Settings SettingsStorage::get(const std::vector<std::string> & path)
{
	return Settings(*this, path);
}

const JsonNode & SettingsStorage::operator[](const std::string & value) const
{
	return config[value];
}

const JsonNode & SettingsStorage::toJsonNode() const
{
	return config;
}

SettingsListener::SettingsListener(SettingsStorage & _parent, std::vector<std::string> _path):
	parent(_parent),
	path(std::move(_path))
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

void SettingsListener::terminate()
{
	wasTerminated = true;
}

SettingsListener::~SettingsListener()
{
	if (!wasTerminated)
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
	callback = std::move(_callback);
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

const JsonNode & Settings::operator[](const std::string & value) const
{
	return node[value];
}

JsonNode & Settings::operator[](const std::string & value)
{
	return node[value];
}

// Force instantiation of the SettingsStorage::NodeAccessor class template.
// That way method definitions can sit in the cpp file
template struct SettingsStorage::NodeAccessor<SettingsListener>;
template struct SettingsStorage::NodeAccessor<Settings>;

VCMI_LIB_NAMESPACE_END
