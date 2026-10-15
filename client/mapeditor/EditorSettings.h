/*
 * EditorSettings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class JsonNode;

/// Settings of the map editor that are kept between the sessions
namespace EditorSettings
{
	/// Stored value, a null node if there is none
	JsonNode getValue(const std::string & key);
	void setValue(const std::string & key, const JsonNode & value);

	/// Maps that were opened recently, the newest first
	std::vector<boost::filesystem::path> getRecentFiles();
	void addRecentFile(const boost::filesystem::path & file);
}
