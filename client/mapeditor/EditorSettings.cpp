/*
 * EditorSettings.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorSettings.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/texts/TextOperations.h"

namespace
{
constexpr size_t MAX_RECENT_FILES = 10;

boost::filesystem::path settingsPath()
{
	return VCMIDirs::get().userConfigPath() / "mapeditor.json";
}

JsonNode load()
{
	boost::system::error_code error;
	const auto path = settingsPath();

	if(!boost::filesystem::exists(path, error))
		return JsonNode();

	std::ifstream stream(path.c_str(), std::ios::binary);
	std::vector<char> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

	return JsonNode(reinterpret_cast<const std::byte *>(data.data()), data.size(), path.string());
}

void save(const JsonNode & node)
{
	std::ofstream stream(settingsPath().c_str(), std::ios::binary | std::ios::trunc);
	stream << node.toString();
}
}

namespace EditorSettings
{

JsonNode getValue(const std::string & key)
{
	return load()[key];
}

void setValue(const std::string & key, const JsonNode & value)
{
	JsonNode node = load();
	node[key] = value;
	save(node);
}

std::vector<boost::filesystem::path> getRecentFiles()
{
	std::vector<boost::filesystem::path> result;

	for(const auto & entry : load()["recentFiles"].Vector())
		result.push_back(TextOperations::Utf8TofilesystemPath(entry.String()));

	return result;
}

void addRecentFile(const boost::filesystem::path & file)
{
	auto recent = getRecentFiles();
	vstd::erase(recent, file);
	recent.insert(recent.begin(), file);

	if(recent.size() > MAX_RECENT_FILES)
		recent.resize(MAX_RECENT_FILES);

	JsonNode node = load();
	node["recentFiles"].Vector().clear();
	for(const auto & path : recent)
		node["recentFiles"].Vector().emplace_back(TextOperations::filesystemPathToUtf8(path));

	save(node);
}

}
