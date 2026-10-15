/*
 * CFileDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CDialogWindow.h"

class CSelectList;
class CTextField;
class CDropDown;

/// Dialog to choose files or a directory in the file system
class CFileDialog : public CDialogWindow
{
public:
	enum class Mode
	{
		OPEN,
		OPEN_MULTIPLE,
		SAVE,
		DIRECTORY
	};

private:
	struct Entry
	{
		boost::filesystem::path path;
		bool directory;
	};

	Mode mode;
	std::vector<std::string> extensions;
	boost::filesystem::path currentDirectory;
	std::vector<Entry> entries;
	std::vector<boost::filesystem::path> places;

	std::shared_ptr<CDropDown> placesField;
	std::shared_ptr<CTextField> pathField;
	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CTextField> nameField;

	void navigate(const boost::filesystem::path & directory);
	void onRowSelected(int row);
	void onRowActivated(int row);
	bool matchesFilter(const boost::filesystem::path & file) const;
	boost::filesystem::path addExtension(const boost::filesystem::path & file) const;

	static void present(std::shared_ptr<CFileDialog> dialog, const std::function<void(const std::vector<boost::filesystem::path> &)> & onChosen);

public:
	/// extensions are lowercase and without the dot, an empty list accepts any file
	CFileDialog(const std::string & title, Mode mode, const boost::filesystem::path & startDirectory, std::vector<std::string> extensions, const std::string & defaultName);

	/// Selected paths. For SAVE mode and OPEN with a single file this is one path, for DIRECTORY the opened directory
	std::vector<boost::filesystem::path> getResult() const;

	static void openFile(const std::string & title, const std::vector<std::string> & extensions, const std::function<void(const boost::filesystem::path &)> & onChosen);
	static void openFiles(const std::string & title, const std::vector<std::string> & extensions, const std::function<void(const std::vector<boost::filesystem::path> &)> & onChosen);
	static void saveFile(const std::string & title, const std::string & extension, const std::string & defaultName, const std::function<void(const boost::filesystem::path &)> & onChosen);
	static void chooseDirectory(const std::string & title, const std::function<void(const boost::filesystem::path &)> & onChosen);
};
