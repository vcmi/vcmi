/*
 * EditorCampaignPropertiesDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorCampaignPropertiesDialog.h"

#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"
#include "../../windows/CPromptWindows.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/campaign/CampaignRegionsHandler.h"
#include "../../../lib/campaign/CampaignState.h"
#include "../../../lib/json/JsonNode.h"

namespace
{
constexpr int LABEL_WIDTH = 190;
constexpr int DESCRIPTION_HEIGHT = 90;
constexpr int REGION_LIST_HEIGHT = 150;
constexpr int MAX_COORDINATE = 9999;
constexpr int MAX_SUFFIX_LENGTH = 10;
const char * const DATE_FORMAT = "%Y-%m-%d %H:%M";

std::string formatDate(std::time_t time)
{
	std::ostringstream stream;
	const std::tm local = *std::localtime(&time);
	stream << std::put_time(&local, DATE_FORMAT);
	return stream.str();
}

bool parseDate(const std::string & text, std::time_t & result)
{
	std::tm parsed = {};
	std::istringstream stream(text);
	stream >> std::get_time(&parsed, DATE_FORMAT);
	if(stream.fail())
		return false;

	parsed.tm_isdst = -1;
	result = std::mktime(&parsed);
	return result != static_cast<std::time_t>(-1);
}

/// Window that edits one region of the campaign map
class RegionDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> infixField;
	std::array<std::shared_ptr<CNumberInput>, 4> inputs;

public:
	RegionDialog(const std::string & infix, const Point & position, const std::optional<Point> & label, const std::function<void(const std::string &, const Point &, const std::optional<Point> &)> & onSaved)
		: CDialogWindow(Point(420, 260), EditorTexts::get("campaign.region"))
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH / 2);
		infixField = form->addText(EditorTexts::get("campaign.infix"), infix, nullptr);
		inputs[0] = form->addNumber(EditorTexts::get("campaign.x"), position.x, 0, MAX_COORDINATE, nullptr);
		inputs[1] = form->addNumber(EditorTexts::get("campaign.y"), position.y, 0, MAX_COORDINATE, nullptr);
		// -1 means that the region has no label
		inputs[2] = form->addNumber(EditorTexts::get("campaign.labelX"), label ? label->x : -1, -1, MAX_COORDINATE, nullptr);
		inputs[3] = form->addNumber(EditorTexts::get("campaign.labelY"), label ? label->y : -1, -1, MAX_COORDINATE, nullptr);
		form->finish();

		onAccept = [this, onSaved]()
		{
			const int labelX = inputs[2]->getValue();
			const int labelY = inputs[3]->getValue();
			std::optional<Point> labelPosition;
			if(labelX != -1 && labelY != -1)
				labelPosition = Point(labelX, labelY);

			onSaved(infixField->getText(), Point(inputs[0]->getValue(), inputs[1]->getValue()), labelPosition);
			return true;
		};
	}
};
}

EditorCampaignPropertiesDialog::EditorCampaignPropertiesDialog(const std::shared_ptr<CampaignState> & campaign, const std::function<void()> & onChanged)
	: CDialogWindow(Point(760, 640), EditorTexts::get("campaign.propertiesTitle"))
	, campaign(campaign)
	, regions(campaign->campaignRegions)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int controlWidth = size.x - LABEL_WIDTH - UiTheme::SLIDER_WIDTH - UiTheme::SPACING;

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);

	nameField = form->addText(EditorTexts::get("campaign.name"), EditorTexts::translate(campaign->name), nullptr);

	descriptionField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, DESCRIPTION_HEIGHT), EditorTexts::translate(campaign->description));
	form->addWidget(EditorTexts::get("campaign.description"), descriptionField);

	authorField = form->addText(EditorTexts::get("campaign.author"), EditorTexts::translate(campaign->author), nullptr);
	contactField = form->addText(EditorTexts::get("campaign.contact"), EditorTexts::translate(campaign->authorContact), nullptr);
	dateField = form->addText(EditorTexts::get("campaign.created"), formatDate(campaign->creationDateTime), nullptr);
	versionField = form->addText(EditorTexts::get("campaign.version"), EditorTexts::translate(campaign->campaignVersion), nullptr);
	musicField = form->addText(EditorTexts::get("campaign.music"), campaign->music.getName(), nullptr);
	difficultyCheck = form->addCheck(EditorTexts::get("campaign.difficultyChosen"), campaign->difficultyChosenByPlayer, nullptr);
	loadingBackgroundField = form->addText(EditorTexts::get("campaign.loadingBackground"), campaign->loadingBackground.getName(), nullptr);

	const bool hasRim = campaign->videoRim != ImagePath::builtin("NONE");
	videoRimCheck = form->addCheck(EditorTexts::get("campaign.videoRim"), hasRim, [this](bool on)
	{
		videoRimField->setReadOnly(!on);
		videoRimField->setText(on ? "" : "NONE");
	});
	videoRimField = form->addText(EditorTexts::get("campaign.videoRimName"), campaign->videoRim.getName(), nullptr);
	videoRimField->setReadOnly(!hasRim);

	introField = form->addText(EditorTexts::get("campaign.introVideo"), campaign->introVideo.getName(), nullptr);
	outroField = form->addText(EditorTexts::get("campaign.outroVideo"), campaign->outroVideo.getName(), nullptr);

	form->addHeading(EditorTexts::get("campaign.regions"));

	const JsonNode legacyRegionConfig(JsonPath::builtin("config/campaignRegions.json"));
	std::vector<std::string> presetNames;
	for(const auto & preset : legacyRegionConfig.Struct())
		presetNames.push_back(preset.first);
	const int presetCount = static_cast<int>(presetNames.size());
	presetNames.push_back(EditorTexts::get("campaign.custom"));

	presetChoice = form->addChoice(EditorTexts::get("campaign.preset"), presetNames, presetCount, [this, presetCount](int index)
	{
		if(index == presetCount)
			return;

		regions = *LIBRARY->campaignRegions->getByIndex(index);
		loadRegions();
	});

	backgroundField = form->addText(EditorTexts::get("campaign.background"), "", nullptr);
	for(size_t i = 0; i < suffixFields.size(); ++i)
		suffixFields[i] = form->addText(EditorTexts::format("campaign.suffix", {std::to_string(i + 1)}), "", nullptr);
	prefixField = form->addText(EditorTexts::get("campaign.prefix"), "", nullptr);
	colorSuffixInput = form->addNumber(EditorTexts::get("campaign.colorSuffixLength"), 0, 0, MAX_SUFFIX_LENGTH, nullptr);

	regionList = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, REGION_LIST_HEIGHT), true);
	regionList->onAdd = [this]()
	{
		CampaignRegions::RegionDescription region;
		region.infix = "INFIX";
		regions.regions.push_back(region);
		refreshRegionList(static_cast<int>(regions.regions.size()) - 1);
	};
	regionList->onEdit = [this](int index) { editRegion(index); };
	regionList->onRemove = [this](int index)
	{
		regions.regions.erase(regions.regions.begin() + index);
		refreshRegionList(-1);
	};
	form->addWidget(EditorTexts::get("campaign.regionList"), regionList);
	form->finish();

	loadRegions();

	onAccept = [this]() { return commit(); };
	afterAccept = onChanged;
}

void EditorCampaignPropertiesDialog::loadRegions()
{
	backgroundField->setText(regions.campBackground.empty() ? regions.campPrefix + "_BG" : regions.campBackground);

	static const std::array<std::string, 3> DEFAULT_SUFFIXES{"En", "Se", "Co"};
	for(size_t i = 0; i < suffixFields.size(); ++i)
		suffixFields[i]->setText(i < regions.campSuffix.size() ? regions.campSuffix[i] : DEFAULT_SUFFIXES[i]);

	prefixField->setText(regions.campPrefix);
	colorSuffixInput->setValue(regions.colorSuffixLength);
	refreshRegionList(-1);
}

void EditorCampaignPropertiesDialog::readRegionFields()
{
	regions.campBackground = backgroundField->getText();
	regions.campSuffix.clear();
	for(const auto & field : suffixFields)
		regions.campSuffix.push_back(field->getText());
	regions.campPrefix = prefixField->getText();
	regions.colorSuffixLength = colorSuffixInput->getValue();
}

void EditorCampaignPropertiesDialog::refreshRegionList(int selected)
{
	std::vector<std::string> names;
	for(const auto & region : regions.regions)
		names.push_back(EditorTexts::format("campaign.regionEntry", {region.infix, std::to_string(region.pos.x), std::to_string(region.pos.y)}));

	regionList->setEntries(names, selected);
}

void EditorCampaignPropertiesDialog::editRegion(int index)
{
	const auto & region = regions.regions.at(index);
	ENGINE->windows().createAndPushWindow<RegionDialog>(region.infix, region.pos, region.labelPos, [this, index](const std::string & infix, const Point & position, const std::optional<Point> & label)
	{
		auto & edited = regions.regions.at(index);
		edited.infix = infix;
		edited.pos = position;
		edited.labelPos = label;
		refreshRegionList(index);
	});
}

bool EditorCampaignPropertiesDialog::commit()
{
	std::time_t created = 0;
	if(!parseDate(dateField->getText(), created))
	{
		CMessageDialog::showMessage(EditorTexts::get("campaign.propertiesTitle"), EditorTexts::get("campaign.invalidDate"));
		return false;
	}

	readRegionFields();

	if(campaign->campaignRegions.regions.size() > regions.regions.size() && !shrinkConfirmed)
	{
		CMessageDialog::askConfirmation(EditorTexts::get("campaign.fewerScenarios"), EditorTexts::get("campaign.fewerScenariosText"), [this]()
		{
			shrinkConfirmed = true;
			accept();
		});
		return false;
	}

	campaign->campaignRegions = regions;
	while(campaign->scenarios.size() < campaign->campaignRegions.regions.size())
		campaign->scenarios.emplace(CampaignScenarioID(std::prev(campaign->scenarios.end())->first + 1), CampaignScenario());
	while(campaign->scenarios.size() > campaign->campaignRegions.regions.size())
	{
		const auto last = std::prev(campaign->scenarios.end());
		campaign->mapPieces.erase(last->first);
		campaign->scenarios.erase(last);
	}

	campaign->name = MetaString::createFromRawString(nameField->getText());
	campaign->description = MetaString::createFromRawString(descriptionField->getText());
	campaign->author = MetaString::createFromRawString(authorField->getText());
	campaign->authorContact = MetaString::createFromRawString(contactField->getText());
	campaign->creationDateTime = created;
	campaign->campaignVersion = MetaString::createFromRawString(versionField->getText());
	campaign->music = AudioPath::builtin(musicField->getText());
	campaign->difficultyChosenByPlayer = difficultyCheck->isChecked();
	campaign->loadingBackground = ImagePath::builtin(loadingBackgroundField->getText());
	campaign->videoRim = ImagePath::builtin(videoRimField->getText());
	campaign->introVideo = VideoPath::builtin(introField->getText());
	campaign->outroVideo = VideoPath::builtin(outroField->getText());
	return true;
}
