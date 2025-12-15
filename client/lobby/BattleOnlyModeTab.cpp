/*
 * BattleOnlyModeTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleOnlyModeTab.h"
#include "CLobbyScreen.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"

#include "../render/IRenderHandler.h"
#include "../render/CAnimation.h"
#include "../render/Canvas.h"
#include "../render/CanvasImage.h"
#include "../render/IFont.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/Buttons.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../widgets/CComponent.h"
#include "../windows/GUIClasses.h"
#include "../windows/CHeroOverview.h"
#include "../windows/CCreatureWindow.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/networkPacks/PacksForLobby.h"
#include "../../lib/StartInfo.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/entities/hero/CHero.h"
#include "../../lib/entities/hero/CHeroClass.h"
#include "../../lib/entities/hero/CHeroHandler.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/entities/artifact/CArtifact.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapObjectConstructors/AObjectTypeHandler.h"
#include "../../lib/mapObjectConstructors/CObjectClassesHandler.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/SpellSchoolHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/serializer/JsonSerializer.h"
#include "../../lib/serializer/JsonDeserializer.h"

BattleOnlyModeTab::BattleOnlyModeTab()
	: startInfo(std::make_shared<BattleOnlyModeStartInfo>())
	, disabledColor(GAME->server().isHost() ? Colors::WHITE : Colors::ORANGE)
	, boxColor(ColorRGBA(128, 128, 128))
	, disabledBoxColor(GAME->server().isHost() ? boxColor : ColorRGBA(116, 92, 16))
{
	OBJECT_CONSTRUCTION;

	try
	{
		JsonNode node = persistentStorage["battleModeSettings"];
		if(!node.isNull())
		{
			node.setModScope(ModScope::scopeGame());
			JsonDeserializer handler(nullptr, node);
			startInfo->serializeJson(handler);
		}
	}
	catch(std::exception & e)
	{
		logGlobal->error("Error loading saved battleModeSettings, received exception: %s", e.what());
	}

	init();

	backgroundImage = std::make_shared<CPicture>(ImagePath::builtin("AdventureOptionsBackgroundClear"), 0, 6);
	title = std::make_shared<CLabel>(220, 35, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyMode"));
	subTitle = std::make_shared<CMultiLineLabel>(Rect(55, 40, 333, 40), FONT_SMALL, ETextAlignment::BOTTOMCENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSubTitle"));

	battlefieldSelector = std::make_shared<CButton>(Point(57, 552), AnimationPath::builtin("GSPButtonClear"), CButton::tooltip(), [this](){ selectTerrain(); });
	battlefieldSelector->block(GAME->server().isGuest());
	buttonReset = std::make_shared<CButton>(Point(259, 552), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](){ reset(); });
	buttonReset->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeReset"), EFonts::FONT_SMALL, Colors::WHITE);

	heroSelector1 = std::make_shared<BattleOnlyModeHeroSelector>(0, *this, Point(55, 90));
	heroSelector2 = std::make_shared<BattleOnlyModeHeroSelector>(1, *this, Point(55, 320));

	heroSelector1->setInputEnabled(GAME->server().isHost());

	onChange();
}

void BattleOnlyModeTab::reset()
{
	if(GAME->server().isHost())
	{
		startInfo->selectedTerrain = TerrainId::DIRT;
		startInfo->selectedTown = FactionID::ANY;
		startInfo->selectedHero[0] = HeroTypeID::NONE;
		startInfo->selectedArmy[0].fill(CStackBasicDescriptor(CreatureID::NONE, 1));
		startInfo->secSkillLevel[0].fill(std::make_pair(SecondarySkill::NONE, MasteryLevel::NONE));
		startInfo->artifacts[0].clear();
		startInfo->spellBook[0] = true;
		startInfo->warMachines[0] = false;
		startInfo->spells[0].clear();
		for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
			heroSelector1->selectedArmyInput.at(i)->disable();
		for(size_t i=0; i<8; i++)
			heroSelector1->selectedSecSkillInput.at(i)->disable();
	}
	startInfo->selectedHero[1] = HeroTypeID::NONE;
	startInfo->selectedArmy[1].fill(CStackBasicDescriptor(CreatureID::NONE, 1));
	startInfo->secSkillLevel[1].fill(std::make_pair(SecondarySkill::NONE, MasteryLevel::NONE));
	startInfo->artifacts[1].clear();
	startInfo->spellBook[1] = true;
	startInfo->warMachines[1] = false;
	startInfo->spells[1].clear();
	for(size_t i=0; i<8; i++)
		heroSelector2->selectedSecSkillInput.at(i)->disable();
	onChange();
}

void BattleOnlyModeTab::selectTerrain()
{
	std::vector<std::string> texts;
	std::vector<std::shared_ptr<IImage>> images;

	std::vector<std::shared_ptr<TerrainType>> terrains;
	std::copy_if(LIBRARY->terrainTypeHandler->objects.begin(), LIBRARY->terrainTypeHandler->objects.end(), std::back_inserter(terrains), [](auto terrain) { return terrain->isPassable(); });
	for (const auto & terrain : terrains)
	{
		texts.push_back(terrain->getNameTranslated());

		const auto & patterns = LIBRARY->terviewh->getTerrainViewPatterns(terrain->getId());
		TerrainViewPattern pattern;
		for(auto & p : patterns)
			if(p[0].id == "n1")
				pattern = p[0];
		auto image = ENGINE->renderHandler().loadImage(terrain->tilesFilename, pattern.mapping[0].first, 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}

	auto factions = LIBRARY->townh->getDefaultAllowed();
	for (const auto & faction : factions)
	{
		texts.push_back(faction.toFaction()->getNameTranslated());

		auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("ITPA"), faction.toFaction()->town->clientInfo.icons[true][false] + 2, 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}

	ENGINE->windows().createAndPushWindow<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefield"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefieldSelect"), [this, terrains, factions](int index){
		if(terrains.size() > index)
		{
			startInfo->selectedTerrain = terrains[index]->getId();
			startInfo->selectedTown = FactionID::ANY;
		}
		else
		{
			startInfo->selectedTerrain = TerrainId::NONE;
			auto it = std::next(factions.begin(), index - terrains.size());
			if (it != factions.end())
				startInfo->selectedTown = *it;
		}
		onChange();
	}, (startInfo->selectedTerrain != TerrainId::NONE ? static_cast<int>(startInfo->selectedTerrain) : static_cast<int>(startInfo->selectedTown + terrains.size())), images, true, true);
}

void BattleOnlyModeTab::init()
{
	map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = 10;
	map->height = 10;
	map->mapLevels = 1;
	map->battleOnly = true;
	map->name = MetaString::createFromTextID("vcmi.lobby.battleOnlyMode");

	cb = std::make_unique<EditorCallback>(map.get());
	map->cb = cb.get();
}

void BattleOnlyModeTab::onChange()
{
	GAME->server().setBattleOnlyModeStartInfo(startInfo);
}

void BattleOnlyModeTab::update()
{
	setTerrainButtonText();
	setStartButtonEnabled();
	
	heroSelector1->setHeroIcon();
	heroSelector1->setCreatureIcons();
	heroSelector1->setSecSkillIcons();
	heroSelector1->setArtifactIcons();
	heroSelector1->spellBook->setSelectedSilent(startInfo->spellBook[0]);
	heroSelector1->warMachines->setSelectedSilent(startInfo->warMachines[0]);
	heroSelector2->setHeroIcon();
	heroSelector2->setCreatureIcons();
	heroSelector2->setSecSkillIcons();
	heroSelector2->setArtifactIcons();
	heroSelector2->spellBook->setSelectedSilent(startInfo->spellBook[1]);
	heroSelector2->warMachines->setSelectedSilent(startInfo->warMachines[1]);
	redraw();

	JsonNode node;
	JsonSerializer handler(nullptr, node);
	startInfo->serializeJson(handler);
	Settings storage = persistentStorage.write["battleModeSettings"];
	storage->Struct() = node.Struct();
}

void BattleOnlyModeTab::applyStartInfo(std::shared_ptr<BattleOnlyModeStartInfo> si)
{
	startInfo = si;
	update();
}

void BattleOnlyModeTab::setTerrainButtonText()
{
	battlefieldSelector->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeBattlefield") + ":   " + (startInfo->selectedTerrain != TerrainId::NONE ? startInfo->selectedTerrain.toEntity(LIBRARY)->getNameTranslated() : startInfo->selectedTown.toEntity(LIBRARY)->getNameTranslated()), EFonts::FONT_SMALL, disabledColor);
}

void BattleOnlyModeTab::setStartButtonEnabled()
{
	bool army2Empty = std::all_of(startInfo->selectedArmy[1].begin(), startInfo->selectedArmy[1].end(), [](const auto x) { return x.getId() == CreatureID::NONE; });

	bool canStart = (startInfo->selectedTerrain != TerrainId::NONE || startInfo->selectedTown != FactionID::ANY);
	canStart &= (startInfo->selectedHero[0] != HeroTypeID::NONE && ((startInfo->selectedHero[1] != HeroTypeID::NONE) || (startInfo->selectedTown != FactionID::ANY && !army2Empty)));
	(static_cast<CLobbyScreen *>(parent))->buttonStart->block(!canStart || GAME->server().isGuest());
}

std::shared_ptr<IImage> drawBlackBox(Point size, std::string text, ColorRGBA color)
{
	auto image = ENGINE->renderHandler().createImage(size, CanvasScalingPolicy::AUTO);
	Canvas canvas = image->getCanvas();
	canvas.drawColor(Rect(0, 0, size.x, size.y), Colors::BLACK);

	std::vector<std::string> lines;
	boost::split(lines, text, boost::is_any_of("\n"));
	int lineH = ENGINE->renderHandler().loadFont(FONT_TINY)->getLineHeight();
	int totalH = lines.size() * lineH;
	int startY = (size.y - totalH) / 2 + lineH / 2;

	for (size_t i = 0; i < lines.size(); ++i)
		canvas.drawText(Point(size.x / 2, startY + i * lineH), FONT_TINY, color, ETextAlignment::CENTER, lines[i]);

	return image;
}

BattleOnlyModeHeroSelector::BattleOnlyModeHeroSelector(int id, BattleOnlyModeTab& p, Point position)
: parent(p)
, id(id)
{
	OBJECT_CONSTRUCTION;

	pos.x += position.x;
	pos.y += position.y;

	backgroundImage = std::make_shared<CPicture>(ImagePath::builtin("heroSlotsBlue"), Point(3, 4));

	for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
	{
		auto image = std::make_shared<CAnimImage>(AnimationPath::builtin("PSKIL32"), i, 0, 78 + i * 36, 26);
		primSkills.push_back(image);
		primSkillsBorder.push_back(std::make_shared<GraphicalPrimitiveCanvas>(Rect(78 + i * 36, 26, 32, 32)));
		primSkillsBorder.back()->addRectangle(Point(0, 0), Point(32, 32), ColorRGBA(44, 108, 255));
		primSkillsInput.push_back(std::make_shared<CTextInput>(Rect(78 + i * 36, 58, 32, 16), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		primSkillsInput.back()->setColor(id == 1 ? Colors::WHITE : parent.disabledColor);
		primSkillsInput.back()->setFilterNumber(0, 100);
		primSkillsInput.back()->setText("0");
		primSkillsInput.back()->setCallback([this, i, id](const std::string & text){
			parent.startInfo->primSkillLevel[id][i] = std::stoi(primSkillsInput[i]->getText());
			parent.onChange();
		});
	}

	creatureImage.resize(GameConstants::ARMY_SIZE);
	for(size_t i=0; i<GameConstants::ARMY_SIZE; i++)
	{
		selectedArmyInput.push_back(std::make_shared<CTextInput>(Rect(5 + i * 36, 113, 32, 16), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		selectedArmyInput.back()->setColor(id == 1 ? Colors::WHITE : parent.disabledColor);
		selectedArmyInput.back()->setFilterNumber(0, 10000000, 3);
		selectedArmyInput.back()->setText("1");
		selectedArmyInput.back()->setCallback([this, i, id](const std::string & text){
			if(parent.startInfo->selectedArmy[id][i].getId() != CreatureID::NONE)
			{
				parent.startInfo->selectedArmy[id][i].setCount(TextOperations::parseMetric<int>(text));
				parent.onChange();
				selectedArmyInput[i]->enable();
			}
			else
				selectedArmyInput[i]->disable();
		});
	}

	for(size_t i=0; i<8; i++)
	{
		bool isLeft = (i % 2 == 0);
		int line = (i / 2);
		Point textPos(261 + (isLeft ? 0 : 36), 41 + line * 54);

		selectedSecSkillInput.push_back(std::make_shared<CTextInput>(Rect(textPos, Point(32, 16)), EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		selectedSecSkillInput.back()->setColor(id == 1 ? Colors::WHITE : parent.disabledColor);
		selectedSecSkillInput.back()->setFilterNumber(0, 3);
		selectedSecSkillInput.back()->setText("3");
		selectedSecSkillInput.back()->setCallback([this, i, id](const std::string & text){
			if(parent.startInfo->secSkillLevel[id][i].second != MasteryLevel::NONE)
			{
				parent.startInfo->secSkillLevel[id][i].second = static_cast<MasteryLevel::Type>(std::stoi(text));
				parent.onChange();
				selectedSecSkillInput[i]->enable();
			}
			else
				selectedSecSkillInput[i]->disable();
		});
	}
	secSkillImage.resize(8);

	artifactImage.resize(14);

	auto tmpIcon = ENGINE->renderHandler().loadImage(AnimationPath::builtin("Artifact"), ArtifactID(ArtifactID::SPELLBOOK).toArtifact()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
	tmpIcon->scaleTo(Point(16, 16), EScalingAlgorithm::NEAREST);
	addIcon.push_back(std::make_shared<CPicture>(tmpIcon, Point(220, 32)));
	addIcon.back()->addLClickCallback([this](){ manageSpells(); });
	addIcon.back()->addRClickCallback([this, id](){
		std::vector<std::shared_ptr<CComponent>> comps;
		for(auto & spell : parent.startInfo->spells[id])
			comps.push_back(std::make_shared<CComponent>(ComponentType::SPELL, spell, std::nullopt, CComponent::ESize::large));
		CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("artifact.core.spellBook.name"), comps);
	});

	spellBook = std::make_shared<CToggleButton>(Point(235, 31), AnimationPath::builtin("lobby/checkboxSmall"), CButton::tooltip(), [this, id](bool enabled){
		parent.startInfo->spellBook[id] = enabled;
		parent.onChange();
		redraw();
	});
	spellBook->setSelectedSilent(parent.startInfo->spellBook[id]);

	tmpIcon = ENGINE->renderHandler().loadImage(AnimationPath::builtin("Artifact"), ArtifactID(ArtifactID::BALLISTA).toArtifact()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
	tmpIcon->scaleTo(Point(16, 16), EScalingAlgorithm::NEAREST);
	addIcon.push_back(std::make_shared<CPicture>(tmpIcon, Point(220, 56)));
	warMachines = std::make_shared<CToggleButton>(Point(235, 55), AnimationPath::builtin("lobby/checkboxSmall"), CButton::tooltip(), [this, id](bool enabled){
		parent.startInfo->warMachines[id] = enabled;
		parent.onChange();
		redraw();
	});
	warMachines->setSelectedSilent(parent.startInfo->warMachines[id]);

	setHeroIcon();
	setCreatureIcons();
	setSecSkillIcons();
	setArtifactIcons();
}

void BattleOnlyModeHeroSelector::manageSpells()
{
	std::vector<std::shared_ptr<CComponent>> resComps;
	for(auto & spellId : parent.startInfo->spells[id])
		resComps.push_back(std::make_shared<CComponent>(ComponentType::SPELL, spellId, std::nullopt, CComponent::ESize::large));

	std::vector<std::pair<AnimationPath, CFunctionList<void()>>> pom;
	for(int i = 0; i < 3; i++)
		pom.emplace_back(AnimationPath::builtin("settingsWindow/button80"), nullptr);

	auto allowedSet = LIBRARY->spellh->getDefaultAllowed();
	std::vector<SpellID> allSpells(allowedSet.begin(), allowedSet.end());
	allSpells.erase(std::remove_if(allSpells.begin(), allSpells.end(), [](const SpellID& spell) {
		return !spell.toSpell()->isCombat();
	}), allSpells.end());
	std::sort(allSpells.begin(), allSpells.end(), [](auto a, auto b) {
		auto A = a.toSpell();
		auto B = b.toSpell();
		if(A->getLevel() != B->getLevel())
			return A->getLevel() < B->getLevel();
		for (const auto schoolId : LIBRARY->spellSchoolHandler->getAllObjects())
		{
			if(A->schools.count(schoolId) && !B->schools.count(schoolId))
				return true;
			if(!A->schools.count(schoolId) && B->schools.count(schoolId))
				return false;
		}
		return TextOperations::compareLocalizedStrings(A->getNameTranslated(), B->getNameTranslated());
	});

	std::vector<SpellID> toAdd;
	std::vector<SpellID> toRemove;
	for (const auto& spell : allSpells)
	{
		bool inCurrent = std::find(parent.startInfo->spells[id].begin(), parent.startInfo->spells[id].end(), spell) != parent.startInfo->spells[id].end();
		if (inCurrent)
			toRemove.push_back(spell);
		else
			toAdd.push_back(spell);
	}

	auto openList = [this](std::vector<SpellID> list, bool add){
		std::vector<std::string> texts;
		std::vector<std::shared_ptr<IImage>> images;
		for (const auto & s : list)
		{
			texts.push_back(s.toSpell()->getNameTranslated());

			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("SpellInt"), s.toSpell()->getIconIndex() + 1, 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
			images.push_back(image);
		}

		std::string title = LIBRARY->generaltexth->translate(add ? "vcmi.lobby.battleOnlySpellAdd" : "vcmi.lobby.battleOnlySpellRemove");
		auto window = std::make_shared<CObjectListWindow>(texts, nullptr, title, title, [this, list, add](int index){
			auto & v = parent.startInfo->spells[id];
			if(add)	
				v.push_back(list[index]);
			else
				v.erase(std::remove(v.begin(), v.end(), list[index]), v.end());

			parent.onChange();
			manageSpells();
		}, 0, images, true, true);
		window->onPopup = [list](int index) {
			std::shared_ptr<CComponent> comp = std::make_shared<CComponent>(ComponentType::SPELL, list[index]);
			CRClickPopup::createAndPush(list[index].toSpell()->getDescriptionTranslated(0), CInfoWindow::TCompsInfo(1, comp));
		};
		ENGINE->windows().pushWindow(window);
	};

	auto temp = std::make_shared<CInfoWindow>(LIBRARY->generaltexth->translate(parent.startInfo->spells[id].size() ? "vcmi.lobby.battleOnlySpellSelectCurrent" : "vcmi.lobby.battleOnlySpellSelect"), PlayerColor(0), resComps, pom);
	temp->buttons[0]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/addChannel")));
	temp->buttons[0]->addCallback([openList, toAdd](){ openList(toAdd, true); });
	temp->buttons[0]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlySpellAdd")); });
	temp->buttons[1]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("lobby/removeChannel")));
	temp->buttons[1]->addCallback([openList, toRemove](){ openList(toRemove, false); });
	temp->buttons[1]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlySpellRemove")); });
	temp->buttons[1]->setEnabled(parent.startInfo->spells[id].size());
	temp->buttons[2]->setOverlay(std::make_shared<CPicture>(ImagePath::builtin("spellResearch/close")));
	temp->buttons[2]->addPopupCallback([](){ CRClickPopup::createAndPush(LIBRARY->generaltexth->translate("core.genrltxt.600")); });

	ENGINE->windows().pushWindow(temp);
}

void BattleOnlyModeHeroSelector::selectHero()
{		
	auto allowedSet = LIBRARY->heroh->getDefaultAllowed();
	std::vector<HeroTypeID> heroes(allowedSet.begin(), allowedSet.end());
	std::sort(heroes.begin(), heroes.end(), [](auto a, auto b) {
		auto heroA = a.toHeroType();
		auto heroB = b.toHeroType();
		if(heroA->heroClass->faction != heroB->heroClass->faction)
			return heroA->heroClass->faction < heroB->heroClass->faction;
		if(heroA->heroClass->getId() != heroB->heroClass->getId())
			return heroA->heroClass->getId() < heroB->heroClass->getId();
		return heroA->getNameTranslated() < heroB->getNameTranslated();
	});

	int selectedIndex = parent.startInfo->selectedHero[id] == HeroTypeID::NONE ? 0 : (1 + std::distance(heroes.begin(), std::find_if(heroes.begin(), heroes.end(), [this](auto heroID) {
		return heroID == parent.startInfo->selectedHero[id];
	})));
	
	std::vector<std::string> texts;
	std::vector<std::shared_ptr<IImage>> images;
	// Add "no hero" option
	texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
	images.push_back(nullptr);
	for (const auto & h : heroes)
	{
		texts.push_back(h.toHeroType()->getNameTranslated());

		auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("PortraitsSmall"), h.toHeroType()->imageIndex, 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(35, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}
	auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeHeroSelect"), [this, heroes](int index){
		if(index == 0)
		{
			parent.startInfo->selectedHero[id] = HeroTypeID::NONE;
			parent.onChange();
			return;
		}
		index--;

		parent.startInfo->selectedHero[id] = heroes[index];

		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			parent.startInfo->primSkillLevel[id][i] = heroes[index].toHeroType()->heroClass->primarySkillInitial[i];

		for(size_t i=0; i<8; i++)
			if(heroes[index].toHeroType()->secSkillsInit.size() > i)
				parent.startInfo->secSkillLevel[id][i] = std::make_pair(heroes[index].toHeroType()->secSkillsInit[i].first, MasteryLevel::Type(heroes[index].toHeroType()->secSkillsInit[i].second));
			else
				parent.startInfo->secSkillLevel[id][i] = std::make_pair(SecondarySkill::NONE, MasteryLevel::NONE);
		
		parent.startInfo->spellBook[id] = heroes[index].toHeroType()->haveSpellBook;

		parent.onChange();
	}, selectedIndex, images, true, true);
	window->onPopup = [heroes](int index) {
		if(index == 0)
			return;
		index--;

		ENGINE->windows().createAndPushWindow<CHeroOverview>(heroes.at(index));
	};
	ENGINE->windows().pushWindow(window);
}

void BattleOnlyModeHeroSelector::setHeroIcon()
{
	OBJECT_CONSTRUCTION;

	if(parent.startInfo->selectedHero[id] == HeroTypeID::NONE)
	{
		heroImage = std::make_shared<CPicture>(drawBlackBox(Point(58, 64), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSelectHero"), id == 1 ? parent.boxColor : parent.disabledBoxColor), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, id == 1 ? Colors::WHITE : parent.disabledColor, LIBRARY->generaltexth->translate("core.genrltxt.507"));
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			primSkillsInput[i]->setText("0");
	}
	else
	{
		heroImage = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("PortraitsLarge"), EImageBlitMode::COLORKEY)->getImage(parent.startInfo->selectedHero[id].toHeroType()->imageIndex), Point(6, 7));
		heroLabel = std::make_shared<CLabel>(160, 16, FONT_SMALL, ETextAlignment::CENTER, id == 1 ? Colors::WHITE : parent.disabledColor, parent.startInfo->selectedHero[id].toHeroType()->getNameTranslated());
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			primSkillsInput[i]->setText(std::to_string(parent.startInfo->primSkillLevel[id][i]));
	}

	heroImage->addLClickCallback([this](){ selectHero(); });

	heroImage->addRClickCallback([this](){
		if(parent.startInfo->selectedHero[id] == HeroTypeID::NONE)
			return;
		
		ENGINE->windows().createAndPushWindow<CHeroOverview>(parent.startInfo->selectedHero[id].toHeroType()->getId());
	});
}

void BattleOnlyModeHeroSelector::selectCreature(int slot)
{
	auto allowedSet = LIBRARY->creh->getDefaultAllowed();
	std::vector<CreatureID> creatures(allowedSet.begin(), allowedSet.end());
	std::sort(creatures.begin(), creatures.end(), [](auto a, auto b) {
		auto creatureA = a.toCreature();
		auto creatureB = b.toCreature();
		if ((creatureA->getFactionID() == FactionID::NEUTRAL) != (creatureB->getFactionID() == FactionID::NEUTRAL))
			return creatureA->getFactionID() != FactionID::NEUTRAL;
		if(creatureA->getFactionID() != creatureB->getFactionID())
			return creatureA->getFactionID() < creatureB->getFactionID();
		if(creatureA->getLevel() != creatureB->getLevel())
			return creatureA->getLevel() < creatureB->getLevel();
		if(creatureA->upgrades.size() != creatureB->upgrades.size())
			return creatureA->upgrades.size() > creatureB->upgrades.size();
		return creatureA->getNameSingularTranslated() < creatureB->getNameSingularTranslated();
	});

	int selectedIndex = parent.startInfo->selectedArmy[id][slot].getId() == CreatureID::NONE ? 0 : (1 + std::distance(creatures.begin(), std::find_if(creatures.begin(), creatures.end(), [this, slot](auto creatureID) {
		return creatureID == parent.startInfo->selectedArmy[id][slot].getId();
	})));
	
	std::vector<std::string> texts;
	std::vector<std::shared_ptr<IImage>> images;
	// Add "no creature" option
	texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
	images.push_back(nullptr);
	for (const auto & c : creatures)
	{
		texts.push_back(c.toCreature()->getNameSingularTranslated());

		auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("CPRSMALL"), c.toCreature()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}
	auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeCreatureSelect"), [this, creatures, slot](int index){
		if(index == 0)
		{
			parent.startInfo->selectedArmy[id][slot] = CStackBasicDescriptor(CreatureID::NONE, 1);
			parent.onChange();
			return;
		}
		index--;

		auto creature = creatures.at(index).toCreature();
		parent.startInfo->selectedArmy[id][slot] = CStackBasicDescriptor(creature->getId(), 100);
		parent.onChange();
	}, selectedIndex, images, true, true);
	window->onPopup = [creatures](int index) {
		if(index == 0)
			return;
		index--;

		ENGINE->windows().createAndPushWindow<CStackWindow>(creatures.at(index).toCreature(), true);
	};
	ENGINE->windows().pushWindow(window);
}

void BattleOnlyModeHeroSelector::setCreatureIcons()
{
	OBJECT_CONSTRUCTION;

	for(int i = 0; i < creatureImage.size(); i++)
	{
		if(parent.startInfo->selectedArmy[id][i].getId() == CreatureID::NONE)
		{
			MetaString str;
			str.appendTextID("vcmi.lobby.battleOnlyModeSelectUnit");
			str.replaceNumber(i + 1);
			creatureImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), str.toString(), id == 1 ? parent.boxColor : parent.disabledBoxColor), Point(6 + i * 36, 78));
			selectedArmyInput[i]->disable();
		}
		else
		{
			auto unit = parent.startInfo->selectedArmy[id][i];
			auto creatureID = unit.getId();
			creatureImage[i] = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("CPRSMALL"), EImageBlitMode::COLORKEY)->getImage(LIBRARY->creh->objects.at(creatureID)->getIconIndex()), Point(6 + i * 36, 78));
			selectedArmyInput[i]->setText(TextOperations::formatMetric(unit.getCount(), 3));
			selectedArmyInput[i]->enable();
		}

		creatureImage[i]->addLClickCallback([this, i](){ selectCreature(i); });

		creatureImage[i]->addRClickCallback([this, i](){
			if(parent.startInfo->selectedArmy[id][i].getId() == CreatureID::NONE)
				return;
			
			ENGINE->windows().createAndPushWindow<CStackWindow>(LIBRARY->creh->objects.at(parent.startInfo->selectedArmy[id][i].getId()).get(), true);
		});
	}
}

void BattleOnlyModeHeroSelector::selectSecSkill(int slot)
{
	auto allowedSet = LIBRARY->skillh->getDefaultAllowed();
	std::vector<SecondarySkill> skills(allowedSet.begin(), allowedSet.end());
	skills.erase( // remove already added skills from selection
		std::remove_if(
			skills.begin(),
			skills.end(),
			[this, slot](auto & skill) {
				return std::any_of(
					parent.startInfo->secSkillLevel[id].begin(), parent.startInfo->secSkillLevel[id].end(),
					[&skill](auto & s) { return s.first == skill; }
				) && parent.startInfo->secSkillLevel[id][slot].first != skill;
			}
		),
		skills.end()
	);
	std::sort(skills.begin(), skills.end(), [](auto a, auto b) {
		auto skillA = a.toSkill();
		auto skillB = b.toSkill();
		return skillA->getNameTranslated() < skillB->getNameTranslated();
	});

	int selectedIndex = parent.startInfo->secSkillLevel[id][slot].second == MasteryLevel::NONE ? 0 : (1 + std::distance(skills.begin(), std::find_if(skills.begin(), skills.end(), [this, slot](auto skillID) {
		return skillID == parent.startInfo->secSkillLevel[id][slot].first;
	})));
	
	std::vector<std::string> texts;
	std::vector<std::shared_ptr<IImage>> images;
	// Add "no skill" option
	texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
	images.push_back(nullptr);
	for (const auto & c : skills)
	{
		texts.push_back(c.toSkill()->getNameTranslated());

		auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("SECSK32"), c.toSkill()->getIconIndex(0), 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}
	auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSecSkillSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeSecSkillSelect"), [this, skills, slot](int index){
		if(index == 0)
		{
			parent.startInfo->secSkillLevel[id][slot] = std::make_pair(SecondarySkill::NONE, MasteryLevel::NONE);
			parent.onChange();
			return;
		}
		index--;

		auto skill = skills.at(index).toSkill();
		parent.startInfo->secSkillLevel[id][slot] = std::make_pair(skill->getId(), MasteryLevel::EXPERT);
		parent.onChange();
	}, selectedIndex, images, true, true);
	window->onPopup = [skills](int index) {
		if(index == 0)
			return;
		index--;

		auto skillId = skills.at(index);
		std::shared_ptr<CComponent> comp = std::make_shared<CComponent>(ComponentType::SEC_SKILL, skillId, MasteryLevel::EXPERT);
		CRClickPopup::createAndPush(skillId.toSkill()->getDescriptionTranslated(MasteryLevel::EXPERT), CInfoWindow::TCompsInfo(1, comp));
	};
	ENGINE->windows().pushWindow(window);
}

void BattleOnlyModeHeroSelector::setSecSkillIcons()
{
	OBJECT_CONSTRUCTION;

	for(int i = 0; i < secSkillImage.size(); i++)
	{
		bool isLeft = (i % 2 == 0);
		int line = (i / 2);
		Point imgPos(261 + (isLeft ? 0 : 36), 7 + line * 54);
		auto skillInfo = parent.startInfo->secSkillLevel[id][i];
		if(skillInfo.second == MasteryLevel::NONE)
		{
			MetaString str;
			str.appendTextID("vcmi.lobby.battleOnlyModeSelectSkill");
			str.replaceNumber(i + 1);
			secSkillImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), str.toString(), id == 1 ? parent.boxColor : parent.disabledBoxColor), imgPos);
			selectedSecSkillInput[i]->disable();
		}
		else
		{
			secSkillImage[i] = std::make_shared<CPicture>(ENGINE->renderHandler().loadAnimation(AnimationPath::builtin("SECSK32"), EImageBlitMode::COLORKEY)->getImage(skillInfo.first.toSkill()->getIconIndex(skillInfo.second - 1)), imgPos);
			selectedSecSkillInput[i]->setText(std::to_string(skillInfo.second));
			selectedSecSkillInput[i]->enable();
		}

		secSkillImage[i]->addLClickCallback([this, i](){ selectSecSkill(i); });

		secSkillImage[i]->addRClickCallback([this, i](){
			auto skillId = parent.startInfo->secSkillLevel[id][i].first;
			auto skillLevel = parent.startInfo->secSkillLevel[id][i].second;

			if(skillLevel == MasteryLevel::NONE)
				return;

			std::shared_ptr<CComponent> comp = std::make_shared<CComponent>(ComponentType::SEC_SKILL, skillId, skillLevel);
			CRClickPopup::createAndPush(skillId.toSkill()->getDescriptionTranslated(skillLevel), CInfoWindow::TCompsInfo(1, comp));
		});
	}
}

std::vector<ArtifactPosition> getArtPos()
{
	std::vector<ArtifactPosition> artPos = {
		ArtifactPosition::HEAD, ArtifactPosition::SHOULDERS, ArtifactPosition::NECK, ArtifactPosition::RIGHT_HAND, ArtifactPosition::LEFT_HAND, ArtifactPosition::TORSO, ArtifactPosition::FEET,
		ArtifactPosition::RIGHT_RING, ArtifactPosition::LEFT_RING, ArtifactPosition::MISC1, ArtifactPosition::MISC2, ArtifactPosition::MISC3, ArtifactPosition::MISC4, ArtifactPosition::MISC5
	};
	return artPos;
}

void BattleOnlyModeHeroSelector::selectArtifact(int slot, ArtifactID artifactId)
{
	auto artPos = getArtPos();

	auto allowedSet = LIBRARY->arth->getDefaultAllowed();
	std::vector<ArtifactID> artifacts(allowedSet.begin(), allowedSet.end());
	artifacts.erase( // remove already added and not for that slot allowed artifacts from selection
		std::remove_if(
			artifacts.begin(),
			artifacts.end(),
			[this, slot, artPos](auto & artifact) {
				auto possibleSlots = artifact.toArtifact()->getPossibleSlots();
				std::vector<ArtifactPosition> allowedSlots;
				if(possibleSlots.find(ArtBearer::HERO) != possibleSlots.end() && !possibleSlots.at(ArtBearer::HERO).empty())
					allowedSlots = possibleSlots.at(ArtBearer::HERO);
				
				return (std::any_of(parent.startInfo->artifacts[id].begin(), parent.startInfo->artifacts[id].end(), [&artifact](auto & a) {return a.second == artifact;})
					&& parent.startInfo->artifacts[id][artPos[slot]] != artifact)
					|| !std::any_of(allowedSlots.begin(), allowedSlots.end(), [slot, artPos](auto & p){ return p == artPos[slot]; });
			}
		),
		artifacts.end()
	);
	std::sort(artifacts.begin(), artifacts.end(), [](auto a, auto b) {
		auto artifactA = a.toArtifact();
		auto artifactB = b.toArtifact();
		return artifactA->getNameTranslated() < artifactB->getNameTranslated();
	});

	int selectedIndex = artifactId == ArtifactID::NONE ? 0 : (1 + std::distance(artifacts.begin(), std::find_if(artifacts.begin(), artifacts.end(), [artifactId](auto artID) {
		return artID == artifactId;
	})));
	
	std::vector<std::string> texts;
	std::vector<std::shared_ptr<IImage>> images;
	// Add "no artifact" option
	texts.push_back(LIBRARY->generaltexth->translate("core.genrltxt.507"));
	images.push_back(nullptr);
	for (const auto & a : artifacts)
	{
		texts.push_back(a.toArtifact()->getNameTranslated());

		auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("Artifact"), a.toArtifact()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
		image->scaleTo(Point(23, 23), EScalingAlgorithm::NEAREST);
		images.push_back(image);
	}
	auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeArtifactSelect"), LIBRARY->generaltexth->translate("vcmi.lobby.battleOnlyModeArtifactSelect"), [this, artifacts, slot, artPos](int index){
		if(index == 0)
		{
			parent.startInfo->artifacts[id][artPos[slot]] = ArtifactID::NONE;
			parent.onChange();
			return;
		}
		index--;

		auto artifact = artifacts.at(index);
		parent.startInfo->artifacts[id][artPos[slot]] = artifact;
		parent.onChange();
	}, selectedIndex, images, true, true);
	window->onPopup = [artifacts](int index) {
		if(index == 0)
			return;
		index--;

		auto artifactId = artifacts.at(index);
		std::shared_ptr<CComponent> comp = std::make_shared<CComponent>(ComponentType::ARTIFACT, artifactId);
		CRClickPopup::createAndPush(artifactId.toArtifact()->getDescriptionTranslated(), CInfoWindow::TCompsInfo(1, comp));
	};
	ENGINE->windows().pushWindow(window);
}

void BattleOnlyModeHeroSelector::setArtifactIcons()
{
	OBJECT_CONSTRUCTION;

	auto artPos = getArtPos();

	for(int i = 0; i < artifactImage.size(); i++)
	{
		int xPos = i % 7;
		int yPos = i / 7;
		Point imgPos(6 + xPos * 36, 137 + yPos * 36);
		auto artifactId = parent.startInfo->artifacts[id][artPos[i]];
		if(artifactId == ArtifactID::NONE)
		{
			MetaString str;
			str.appendTextID("vcmi.lobby.battleOnlyModeSelectArtifact");
			str.replaceTextID("vcmi.lobby.battleOnlyModeSelectArtifact." + std::to_string(artPos[i]));
			artifactImage[i] = std::make_shared<CPicture>(drawBlackBox(Point(32, 32), str.toString(), id == 1 ? parent.boxColor : parent.disabledBoxColor), imgPos);
		}
		else
		{
			auto image = ENGINE->renderHandler().loadImage(AnimationPath::builtin("Artifact"), artifactId.toArtifact()->getIconIndex(), 0, EImageBlitMode::OPAQUE);
			image->scaleTo(Point(32, 32), EScalingAlgorithm::NEAREST);
			artifactImage[i] = std::make_shared<CPicture>(image, imgPos);
		}
			
		artifactImage[i]->addLClickCallback([this, i, artifactId](){ selectArtifact(i, artifactId); });

		artifactImage[i]->addRClickCallback([this, i, artPos](){
			auto artId = parent.startInfo->artifacts[id][artPos[i]];
			if(artId == ArtifactID::NONE)
				return;

			std::shared_ptr<CComponent> comp = std::make_shared<CComponent>(ComponentType::ARTIFACT, artId);
			CRClickPopup::createAndPush(artId.toArtifact()->getDescriptionTranslated(), CInfoWindow::TCompsInfo(1, comp));
		});
	}
}

void BattleOnlyModeTab::startBattle()
{
	auto rng = &CRandomGenerator::getDefault();
	
	map->initTerrain();
	map->getEditManager()->clearTerrain(rng);

	map->getEditManager()->getTerrainSelection().selectAll();
	map->getEditManager()->drawTerrain(startInfo->selectedTerrain == TerrainId::NONE ? TerrainId::DIRT : startInfo->selectedTerrain, 0, rng);

	map->players[0].canComputerPlay = true;
	map->players[0].canHumanPlay = true;
	map->players[1] = map->players[0];

	auto knownHeroes = LIBRARY->objtypeh->knownSubObjects(Obj::HERO);

	auto addHero = [&, this](int sel, PlayerColor color, const int3 & position)
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::HERO, startInfo->selectedHero[sel].toHeroType()->heroClass->getId());
		auto templates = factory->getTemplates();
		auto obj = std::dynamic_pointer_cast<CGHeroInstance>(factory->create(cb.get(), templates.front()));
		obj->setHeroType(startInfo->selectedHero[sel]);

		obj->setOwner(color);
		obj->pos = position;
		for(size_t i=0; i<GameConstants::PRIMARY_SKILLS; i++)
			obj->pushPrimSkill(PrimarySkill(i), startInfo->primSkillLevel[sel][i]);
		obj->clearSlots();
		for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
			if(startInfo->selectedArmy[sel][slot].getId() != CreatureID::NONE && startInfo->selectedArmy[sel][slot].getCount() > 0)
				obj->setCreature(SlotID(slot), startInfo->selectedArmy[sel][slot].getId(), startInfo->selectedArmy[sel][slot].getCount());

		// give spellbook
		if(!obj->getArt(ArtifactPosition::SPELLBOOK) && startInfo->spellBook[sel])
			obj->putArtifact(ArtifactPosition::SPELLBOOK, map->createArtifact(ArtifactID::SPELLBOOK));
		else if(obj->getArt(ArtifactPosition::SPELLBOOK) && !startInfo->spellBook[sel])
			obj->removeArtifact(ArtifactPosition::SPELLBOOK);
		
		if(startInfo->warMachines[sel])
		{
			obj->putArtifact(ArtifactPosition::MACH1, map->createArtifact(ArtifactID::BALLISTA));
			obj->putArtifact(ArtifactPosition::MACH2, map->createArtifact(ArtifactID::AMMO_CART));
			obj->putArtifact(ArtifactPosition::MACH3, map->createArtifact(ArtifactID::FIRST_AID_TENT));
		}
		
		for(const auto & spell : startInfo->spells[sel])
			obj->addSpellToSpellbook(spell);

		for(auto & artifact : startInfo->artifacts[sel])
			if(artifact.second != ArtifactID::NONE)
				obj->putArtifact(artifact.first, map->createArtifact(artifact.second));

		for(const auto & skill : LIBRARY->skillh->objects) // reset all standard skills
			obj->setSecSkillLevel(SecondarySkill(skill->getId()), MasteryLevel::NONE, ChangeValueMode::ABSOLUTE);
		for(int skillSlot = 0; skillSlot < 8; skillSlot++)
			obj->setSecSkillLevel(startInfo->secSkillLevel[sel][skillSlot].first, startInfo->secSkillLevel[sel][skillSlot].second, ChangeValueMode::ABSOLUTE);

		map->getEditManager()->insertObject(obj);
	};

	addHero(0, PlayerColor(0), int3(5, 6, 0));
	if(startInfo->selectedTown == FactionID::ANY)
		addHero(1, PlayerColor(1), int3(5, 5, 0));
	else
	{
		auto factory = LIBRARY->objtypeh->getHandlerFor(Obj::TOWN, startInfo->selectedTown);
		auto templates = factory->getTemplates();
		auto obj = factory->create(cb.get(), templates.front());
		auto townObj = std::dynamic_pointer_cast<CGTownInstance>(obj);
		obj->setOwner(PlayerColor(1));
		obj->pos = int3(5, 5, 0);
		for (const auto & building : townObj->getTown()->getAllBuildings())
			townObj->addBuilding(building);
		if(startInfo->selectedHero[1] == HeroTypeID::NONE)
		{
			for(int slot = 0; slot < GameConstants::ARMY_SIZE; slot++)
				if(startInfo->selectedArmy[1][slot].getId() != CreatureID::NONE && startInfo->selectedArmy[1][slot].getCount() > 0)
					townObj->getArmy()->setCreature(SlotID(slot), startInfo->selectedArmy[1][slot].getId(), startInfo->selectedArmy[1][slot].getCount());
		}
		else
			addHero(1, PlayerColor(1), int3(5, 5, 0));

		map->getEditManager()->insertObject(townObj);
	}

	auto path = VCMIDirs::get().userDataPath() / "Maps";
	boost::filesystem::create_directories(path);
	const std::string fileName = "BattleOnlyMode.vmap";
	const auto fullPath = path / fileName;
	CMapService mapService;
	mapService.saveMap(map, fullPath);
	CResourceHandler::get()->updateFilteredFiles([&](const std::string & mount) { return true; });

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit("Maps/BattleOnlyMode");
	GAME->server().setMapInfo(mapInfo);
	ExtraOptionsInfo extraOptions;
	extraOptions.unlimitedReplay = true;
	GAME->server().setExtraOptionsInfo(extraOptions);
	GAME->server().sendStartGame(false, false);
}
