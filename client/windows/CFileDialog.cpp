/*
 * CFileDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CFileDialog.h"

#include "CPromptWindows.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CSelectList.h"
#include "../widgets/CTextField.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace
{
constexpr int DIALOG_WIDTH = 560;
constexpr int DIALOG_HEIGHT = 520;

/// Directory shown by the next dialog, so repeated opening starts where the previous one ended
boost::filesystem::path lastDirectory;

std::string toUtf8(const boost::filesystem::path & path)
{
	return TextOperations::filesystemPathToUtf8(path);
}

std::string formatSize(boost::uintmax_t bytes)
{
	if(bytes < 1024)
		return std::to_string(bytes) + " B";
	if(bytes < 1024 * 1024)
		return std::to_string(bytes / 1024) + " KB";
	return std::to_string(bytes / (1024 * 1024)) + " MB";
}
}

CFileDialog::CFileDialog(const std::string & title, Mode mode, const boost::filesystem::path & startDirectory, std::vector<std::string> extensions, const std::string & defaultName)
	: CDialogWindow(Point(DIALOG_WIDTH, DIALOG_HEIGHT), title)
	, mode(mode)
	, extensions(std::move(extensions))
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int controlHeight = UiTheme::CONTROL_HEIGHT;
	const bool needsName = mode == Mode::SAVE;

	places.push_back(VCMIDirs::get().userDataPath());
	places.push_back(VCMIDirs::get().userDataPath() / "Maps");
	places.push_back(VCMIDirs::get().userSavePath());
	for(const auto & path : VCMIDirs::get().dataPaths())
		places.push_back(path);
	places.push_back(boost::filesystem::current_path());
#ifndef VCMI_WINDOWS
	if(const char * home = std::getenv("HOME"))
		places.emplace_back(home);
	places.emplace_back("/");
#endif

	std::vector<std::string> placeNames;
	for(const auto & place : places)
		placeNames.push_back(toUtf8(place));

	placesField = std::make_shared<CDropDown>(Rect(0, 0, size.x, controlHeight), placeNames, -1, [this](int index) { navigate(places.at(index)); },
		LIBRARY->generaltexth->translate("vcmi.ui.places"));

	pathField = std::make_shared<CTextField>(Rect(0, controlHeight + UiTheme::SPACING, size.x, controlHeight), "", [this](const std::string & text)
	{
		boost::system::error_code error;
		const auto typed = TextOperations::Utf8TofilesystemPath(text);
		if(boost::filesystem::is_directory(typed, error))
			navigate(typed);
	});

	const int listTop = 2 * (controlHeight + UiTheme::SPACING);
	const int listHeight = size.y - listTop - (needsName ? controlHeight + UiTheme::SPACING : 0);

	list = std::make_shared<CSelectList>(Rect(0, listTop, size.x, listHeight), mode == Mode::OPEN_MULTIPLE ? CSelectList::Mode::CHECK_SELECT : CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);
	list->onSelect = [this](int row) { onRowSelected(row); };
	list->onActivate = [this](int row) { onRowActivated(row); };

	if(needsName)
	{
		nameField = std::make_shared<CTextField>(Rect(0, listTop + listHeight + UiTheme::SPACING, size.x, controlHeight), defaultName);
		nameField->setFilterFilename();
	}

	if(mode == Mode::DIRECTORY)
		setAcceptText(LIBRARY->generaltexth->translate("vcmi.ui.select"));
	else if(mode == Mode::SAVE)
		setAcceptText(LIBRARY->generaltexth->translate("vcmi.ui.save"));
	else
		setAcceptText(LIBRARY->generaltexth->translate("vcmi.ui.open"));

	boost::system::error_code error;
	boost::filesystem::path start = startDirectory;
	if(start.empty() || !boost::filesystem::is_directory(start, error))
		start = lastDirectory.empty() ? VCMIDirs::get().userDataPath() : lastDirectory;
	if(!boost::filesystem::is_directory(start, error))
		start = boost::filesystem::current_path();

	navigate(start);
}

void CFileDialog::navigate(const boost::filesystem::path & directory)
{
	boost::system::error_code error;

	currentDirectory = boost::filesystem::canonical(directory, error);
	if(error)
		currentDirectory = directory;

	lastDirectory = currentDirectory;
	pathField->setText(toUtf8(currentDirectory));

	std::vector<Entry> directories;
	std::vector<Entry> files;

	for(boost::filesystem::directory_iterator it(currentDirectory, error), end; !error && it != end; it.increment(error))
	{
		boost::system::error_code entryError;
		const auto & path = it->path();

		if(boost::filesystem::is_directory(path, entryError))
			directories.push_back({path, true});
		else if(mode != Mode::DIRECTORY && matchesFilter(path))
			files.push_back({path, false});
	}

	auto byName = [](const Entry & left, const Entry & right)
	{
		return boost::algorithm::to_lower_copy(left.path.filename().string()) < boost::algorithm::to_lower_copy(right.path.filename().string());
	};
	std::sort(directories.begin(), directories.end(), byName);
	std::sort(files.begin(), files.end(), byName);

	entries.clear();
	std::vector<CSelectList::Row> rows;

	if(currentDirectory.has_parent_path() && currentDirectory.parent_path() != currentDirectory)
	{
		entries.push_back({currentDirectory.parent_path(), true});

		CSelectList::Row row;
		row.text = "..";
		rows.push_back(std::move(row));
	}

	for(const auto & entry : directories)
	{
		entries.push_back(entry);

		CSelectList::Row row;
		row.text = "[" + toUtf8(entry.path.filename()) + "]";
		rows.push_back(std::move(row));
	}

	for(const auto & entry : files)
	{
		entries.push_back(entry);

		CSelectList::Row row;
		row.text = toUtf8(entry.path.filename());

		boost::system::error_code sizeError;
		const auto size = boost::filesystem::file_size(entry.path, sizeError);
		if(!sizeError)
			row.detail = formatSize(size);

		rows.push_back(std::move(row));
	}

	list->setRows(std::move(rows));
}

bool CFileDialog::matchesFilter(const boost::filesystem::path & file) const
{
	if(extensions.empty())
		return true;

	std::string extension = file.extension().string();
	if(!extension.empty() && extension.front() == '.')
		extension.erase(0, 1);
	boost::algorithm::to_lower(extension);

	return std::find(extensions.begin(), extensions.end(), extension) != extensions.end();
}

boost::filesystem::path CFileDialog::addExtension(const boost::filesystem::path & file) const
{
	if(extensions.empty() || matchesFilter(file))
		return file;

	return boost::filesystem::path(file.string() + "." + extensions.front());
}

void CFileDialog::onRowSelected(int row)
{
	const Entry & entry = entries.at(row);

	if(entry.directory)
	{
		navigate(entry.path);
		return;
	}

	if(nameField)
		nameField->setText(toUtf8(entry.path.filename()));
}

void CFileDialog::onRowActivated(int row)
{
	const Entry & entry = entries.at(row);

	if(entry.directory)
	{
		navigate(entry.path);
		return;
	}

	if(mode != Mode::OPEN_MULTIPLE)
		accept();
}

std::vector<boost::filesystem::path> CFileDialog::getResult() const
{
	std::vector<boost::filesystem::path> result;

	switch(mode)
	{
		case Mode::DIRECTORY:
			result.push_back(currentDirectory);
			break;

		case Mode::SAVE:
			if(!nameField->getText().empty())
				result.push_back(addExtension(currentDirectory / TextOperations::Utf8TofilesystemPath(nameField->getText())));
			break;

		case Mode::OPEN_MULTIPLE:
			for(int row : list->getCheckedIndexes())
				if(!entries.at(row).directory)
					result.push_back(entries.at(row).path);
			break;

		case Mode::OPEN:
		{
			const int row = list->getSelected();
			if(row >= 0 && !entries.at(row).directory)
				result.push_back(entries.at(row).path);
			break;
		}
	}
	return result;
}

void CFileDialog::present(std::shared_ptr<CFileDialog> dialog, const std::function<void(const std::vector<boost::filesystem::path> &)> & onChosen)
{
	auto * raw = dialog.get();
	auto result = std::make_shared<std::vector<boost::filesystem::path>>();
	auto overwriteConfirmed = std::make_shared<bool>(false);

	dialog->onAccept = [raw, result, overwriteConfirmed]()
	{
		*result = raw->getResult();
		if(result->empty())
			return false;

		boost::system::error_code error;
		if(raw->mode == Mode::SAVE && !*overwriteConfirmed && boost::filesystem::exists(result->front(), error))
		{
			CMessageDialog::askConfirmation(LIBRARY->generaltexth->translate("vcmi.ui.overwriteTitle"), LIBRARY->generaltexth->translate("vcmi.ui.overwriteText"), [raw, overwriteConfirmed]()
			{
				*overwriteConfirmed = true;
				raw->accept();
			});
			return false;
		}
		return true;
	};
	dialog->afterAccept = [result, onChosen]()
	{
		if(onChosen)
			onChosen(*result);
	};
	ENGINE->windows().pushWindow(dialog);
}

void CFileDialog::openFile(const std::string & title, const std::vector<std::string> & extensions, const std::function<void(const boost::filesystem::path &)> & onChosen)
{
	present(std::make_shared<CFileDialog>(title, Mode::OPEN, boost::filesystem::path(), extensions, ""), [onChosen](const std::vector<boost::filesystem::path> & paths)
	{
		if(onChosen)
			onChosen(paths.front());
	});
}

void CFileDialog::openFiles(const std::string & title, const std::vector<std::string> & extensions, const std::function<void(const std::vector<boost::filesystem::path> &)> & onChosen)
{
	present(std::make_shared<CFileDialog>(title, Mode::OPEN_MULTIPLE, boost::filesystem::path(), extensions, ""), onChosen);
}

void CFileDialog::saveFile(const std::string & title, const std::string & extension, const std::string & defaultName, const std::function<void(const boost::filesystem::path &)> & onChosen)
{
	present(std::make_shared<CFileDialog>(title, Mode::SAVE, boost::filesystem::path(), std::vector<std::string>{extension}, defaultName), [onChosen](const std::vector<boost::filesystem::path> & paths)
	{
		if(onChosen)
			onChosen(paths.front());
	});
}

void CFileDialog::chooseDirectory(const std::string & title, const std::function<void(const boost::filesystem::path &)> & onChosen)
{
	present(std::make_shared<CFileDialog>(title, Mode::DIRECTORY, boost::filesystem::path(), std::vector<std::string>{}, ""), [onChosen](const std::vector<boost::filesystem::path> & paths)
	{
		if(onChosen)
			onChosen(paths.front());
	});
}
