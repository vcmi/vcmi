/*
 * EditorResourceConverter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorResourceConverter.h"

#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CCheckBox.h"
#include "../widgets/UiTheme.h"
#include "../windows/CDialogWindow.h"
#include "../windows/CPromptWindows.h"
#include "render/CAnimation.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace
{
constexpr int MAX_DEF_GROUPS = 256;

const std::vector<std::string> SHARED_DEFS = {"TwCrPort", "CPRSMALL", "FlagPort", "ITPA", "ITPt", "Un32", "Un44"};

EditorResourceConverter::Result convertPcxToPng(const boost::filesystem::path & sourceFolder, bool deleteOriginals)
{
	EditorResourceConverter::Result result;
	logGlobal->info("Converting .pcx to .png from folder: %s", TextOperations::filesystemPathToUtf8(sourceFolder));

	if(!boost::filesystem::is_directory(sourceFolder))
	{
		logGlobal->error("Folder %s does not exist", TextOperations::filesystemPathToUtf8(sourceFolder));
		return result;
	}

	for(const auto & entry : boost::filesystem::directory_iterator(sourceFolder))
	{
		const auto & file = entry.path();
		if(!boost::filesystem::is_regular_file(entry) || boost::algorithm::to_lower_copy(TextOperations::filesystemPathToUtf8(file.extension())) != ".pcx")
			continue;

		try
		{
			const auto image = ENGINE->renderHandler().loadImage(ImagePath::builtin(TextOperations::filesystemPathToUtf8(file.stem())), EImageBlitMode::OPAQUE);
			image->exportBitmap(boost::filesystem::path(file).replace_extension(".png"));

			if(deleteOriginals)
				boost::filesystem::remove(file);

			++result.converted;
		}
		catch(const std::exception & e)
		{
			logGlobal->error("Failed to convert %s: %s", TextOperations::filesystemPathToUtf8(file.filename()), e.what());
			++result.failed;
		}
	}
	return result;
}

EditorResourceConverter::Result splitDef(const std::string & name, const boost::filesystem::path & sourceFolder, bool deleteOriginals)
{
	EditorResourceConverter::Result result;
	const std::string upperName = boost::algorithm::to_upper_copy(name);

	if(!CResourceHandler::get()->existsResource(AnimationPath::builtin("SPRITES/" + upperName)))
	{
		logGlobal->error("Def file split error, %s does not exist", name);
		++result.failed;
		return result;
	}

	const auto animation = ENGINE->renderHandler().loadAnimation(AnimationPath::builtin(upperName), EImageBlitMode::SIMPLE);
	const auto targetFolder = sourceFolder / upperName;
	boost::filesystem::create_directories(targetFolder);

	for(size_t group = 0; group < MAX_DEF_GROUPS; ++group)
	{
		for(size_t frame = 0; frame < animation->size(group); ++frame)
		{
			try
			{
				const auto image = animation->getImage(frame, group);
				image->exportBitmap(targetFolder / (upperName + "_" + std::to_string(group) + "_" + std::to_string(frame) + ".png"));
				++result.converted;
			}
			catch(const std::exception & e)
			{
				logGlobal->error("Failed to export frame %d of the group %d of %s: %s", frame, group, name, e.what());
				++result.failed;
			}
		}
	}

	if(deleteOriginals)
		boost::filesystem::remove(sourceFolder / (name + ".def"));

	return result;
}

/// Window that asks which conversions to run
class ConverterDialog : public CDialogWindow
{
	std::shared_ptr<CCheckBox> splitCheck;
	std::shared_ptr<CCheckBox> pcxCheck;
	std::shared_ptr<CCheckBox> deleteCheck;

public:
	ConverterDialog()
		: CDialogWindow(Point(520, 190), EditorTexts::get("resources.title"))
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const int rowHeight = UiTheme::CONTROL_HEIGHT;
		const int width = contentSize().x;
		splitCheck = std::make_shared<CCheckBox>(Rect(0, 0, width, rowHeight), EditorTexts::get("resources.split"), false, nullptr);
		pcxCheck = std::make_shared<CCheckBox>(Rect(0, rowHeight + UiTheme::SPACING, width, rowHeight), EditorTexts::get("resources.pcx"), false, nullptr);
		deleteCheck = std::make_shared<CCheckBox>(Rect(0, 2 * (rowHeight + UiTheme::SPACING), width, rowHeight), EditorTexts::get("resources.deleteOriginals"), false, nullptr);

		onAccept = [this]()
		{
			EditorResourceConverter::Options chosen;
			chosen.splitDefs = splitCheck->isChecked();
			chosen.convertPcxToPng = pcxCheck->isChecked();
			chosen.deleteOriginals = deleteCheck->isChecked();

			afterAccept = [chosen]()
			{
				const auto result = EditorResourceConverter::convert(chosen);
				CMessageDialog::showMessage(EditorTexts::get("resources.title"), EditorTexts::format("resources.done", {std::to_string(result.converted), std::to_string(result.failed)}));
			};
			return true;
		};
	}
};
}

EditorResourceConverter::Result EditorResourceConverter::convert(const Options & options)
{
	const auto spritesPath = VCMIDirs::get().userExtractedPath() / "SPRITES";
	const auto imagesPath = VCMIDirs::get().userExtractedPath() / "IMAGES";

	Result total;
	const auto add = [&total](const Result & part)
	{
		total.converted += part.converted;
		total.failed += part.failed;
	};

	if(options.splitDefs)
	{
		logGlobal->info("Splitting def files from folder: %s", TextOperations::filesystemPathToUtf8(spritesPath));
		for(const auto & name : SHARED_DEFS)
			add(splitDef(name, spritesPath, options.deleteOriginals));
	}

	if(options.convertPcxToPng)
		add(convertPcxToPng(imagesPath, options.deleteOriginals));

	return total;
}

void EditorResourceConverter::showDialog()
{
	ENGINE->windows().createAndPushWindow<ConverterDialog>();
}
