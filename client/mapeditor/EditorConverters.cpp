/*
 * EditorConverters.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorConverters.h"

#include "EditorFiles.h"
#include "EditorMap.h"
#include "EditorTexts.h"

#include "../windows/CFileDialog.h"
#include "../windows/CPromptWindows.h"

#include "../../lib/mapping/CMap.h"
#include "../../lib/texts/TextOperations.h"

namespace EditorConverters
{

void convertMaps(EditorCallback * callback)
{
	CFileDialog::openFiles(EditorTexts::get("converter.selectMaps"), {"h3m"}, [callback](const std::vector<boost::filesystem::path> & mapFiles)
	{
		CFileDialog::chooseDirectory(EditorTexts::get("converter.selectDirectory"), [callback, mapFiles](const boost::filesystem::path & directory)
		{
			try
			{
				for(const auto & file : mapFiles)
				{
					auto map = EditorFiles::openMap(file, callback);
					callback->setMap(map.get());
					EditorMap::repairMap(map.get());

					EditorFiles::saveMap(map, directory / (file.stem().string() + ".vmap"));
				}
				CMessageDialog::showMessage(EditorTexts::get("converter.completed"), EditorTexts::format("converter.convertedMaps", {std::to_string(mapFiles.size())}));
			}
			catch(const std::exception & e)
			{
				CMessageDialog::showMessage(EditorTexts::get("converter.failedMap"), e.what());
			}
		});
	});
}

void convertCampaign()
{
	CFileDialog::openFile(EditorTexts::get("converter.selectCampaign"), {"h3c"}, [](const boost::filesystem::path & source)
	{
		CFileDialog::saveFile(EditorTexts::get("converter.selectDestination"), "vcmp", source.stem().string(), [source](const boost::filesystem::path & destination)
		{
			try
			{
				auto campaign = EditorFiles::openCampaign(source);
				EditorFiles::saveCampaign(campaign, destination);
				CMessageDialog::showMessage(EditorTexts::get("converter.completed"), EditorTexts::get("converter.convertedCampaign"));
			}
			catch(const std::exception & e)
			{
				CMessageDialog::showMessage(EditorTexts::get("converter.failedCampaign"), e.what());
			}
		});
	});
}

}
