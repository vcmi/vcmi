/*
 * CComponent.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CComponent.h"

#include "Images.h"

#include "../GameEngine.h"
#include "../gui/CursorHandler.h"
#include "../gui/TextAlignment.h"
#include "../gui/Shortcut.h"
#include "../render/Canvas.h"
#include "../render/IFont.h"
#include "../render/IRenderHandler.h"
#include "../render/Graphics.h"
#include "../windows/CMessage.h"
#include "../windows/InfoWindows.h"
#include "../widgets/TextControls.h"

#include "../../lib/entities/artifact/ArtifactUtils.h"
#include "../../lib/entities/artifact/CArtHandler.h"
#include "../../lib/entities/building/CBuilding.h"
#include "../../lib/entities/faction/CFaction.h"
#include "../../lib/entities/faction/CTown.h"
#include "../../lib/entities/faction/CTownHandler.h"
#include "../../lib/networkPacks/Component.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/GameLibrary.h"

#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>
#include <vcmi/HeroTypeService.h>
#include <vcmi/HeroType.h>

CComponent::CComponent(ComponentType Type, ComponentSubType Subtype, std::optional<int32_t> Val, ESize imageSize, EFonts font)
{
	init(Type, Subtype, Val, imageSize, font, "");
}

CComponent::CComponent(ComponentType Type, ComponentSubType Subtype, const std::string & Val, ESize imageSize, EFonts font)
{
	init(Type, Subtype, std::nullopt, imageSize, font, Val);
}

CComponent::CComponent(const Component & c, ESize imageSize, EFonts font)
{
	init(c.type, c.subType, c.value, imageSize, font, "");
}

void CComponent::init(ComponentType Type, ComponentSubType Subtype, std::optional<int32_t> Val, ESize imageSize, EFonts fnt, const std::string & ValText)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(SHOW_POPUP);

	data.type = Type;
	data.subType = Subtype;
	data.value = Val;

	customSubtitle = ValText;
	size = imageSize;
	font = fnt;
	newLine = false;

	assert(size < sizeInvalid);

	setSurface(getFileName()[size], (int)getIndex());

	pos.w = image->pos.w;
	pos.h = image->pos.h;

	if (imageSize < small)
		font = FONT_TINY; //for tiny images - tiny font

	pos.h += 4; //distance between text and image

	// WARNING: too low values will lead to bad line-breaks in CPlayerOptionTooltipBox - check right-click on starting town in pregame
	int max = 80;
	if (size < large)
		max = 72;
	if (size < medium)
		max = 60;
	if (size < small)
		max = 55;

	if(Type == ComponentType::RESOURCE && !ValText.empty())
		max = 80;

	std::vector<std::string> textLines = CMessage::breakText(getSubtitle(), std::max<int>(max, pos.w), font);
	const auto & fontPtr = ENGINE->renderHandler().loadFont(font);
	const int height = static_cast<int>(fontPtr->getLineHeight());

	for(auto & line : textLines)
	{
		auto label = std::make_shared<CLabel>(pos.w/2, pos.h + height/2, font, ETextAlignment::CENTER, Colors::WHITE, line);

		pos.h += height;
		if(label->pos.w > pos.w)
		{
			pos.x -= (label->pos.w - pos.w)/2;
			pos.w = label->pos.w;
		}
		lines.push_back(label);
	}
}

std::vector<AnimationPath> CComponent::getFileName() const
{
	static const std::array<std::string, 4>  primSkillsArr = {"PSKIL32",        "PSKIL32",        "PSKIL42",        "PSKILL"};
	static const std::array<std::string, 4>  secSkillsArr =  {"SECSK32",        "SECSK32",        "SECSKILL",       "SECSK82"};
	static const std::array<std::string, 4>  resourceArr =   {"SMALRES",        "RESOURCE",       "RESOURCE",       "RESOUR82"};
	static const std::array<std::string, 4>  creatureArr =   {"CPRSMALL",       "CPRSMALL",       "CPRSMALL",       "TWCRPORT"};
	static const std::array<std::string, 4>  artifactArr =   {"Artifact",       "Artifact",       "Artifact",       "Artifact"};
	static const std::array<std::string, 4>  spellsArr =     {"SpellInt",       "SpellInt",       "SpellInt",       "SPELLSCR"};
	static const std::array<std::string, 4>  moraleArr =     {"IMRL22",         "IMRL30",         "IMRL42",         "imrl82"};
	static const std::array<std::string, 4>  luckArr =       {"ILCK22",         "ILCK30",         "ILCK42",         "ilck82"};
	static const std::array<std::string, 4>  heroArr =       {"PortraitsSmall", "PortraitsSmall", "PortraitsSmall", "PortraitsLarge"};
	static const std::array<std::string, 4>  flagArr =       {"CREST58",        "CREST58",        "CREST58",        "CREST58"};

	auto gen = [](const std::array<std::string, 4> & arr) -> std::vector<AnimationPath>
	{
		return { AnimationPath::builtin(arr[0]), AnimationPath::builtin(arr[1]), AnimationPath::builtin(arr[2]), AnimationPath::builtin(arr[3]) };
	};

	switch(data.type)
	{
		case ComponentType::PRIM_SKILL:
		case ComponentType::EXPERIENCE:
		case ComponentType::MANA:
		case ComponentType::LEVEL:
			return gen(primSkillsArr);
		case ComponentType::NONE:
		case ComponentType::SEC_SKILL:
			return gen(secSkillsArr);
		case ComponentType::RESOURCE:
		case ComponentType::RESOURCE_PER_DAY:
			return gen(resourceArr);
		case ComponentType::CREATURE:
			return gen(creatureArr);
		case ComponentType::ARTIFACT:
			return gen(artifactArr);
		case ComponentType::SPELL_SCROLL:
		case ComponentType::SPELL:
			return gen(spellsArr);
		case ComponentType::MORALE:
			return gen(moraleArr);
		case ComponentType::LUCK:
			return gen(luckArr);
		case ComponentType::BUILDING:
			return std::vector<AnimationPath>(4, (*LIBRARY->townh)[data.subType.as<BuildingTypeUniqueID>().getFaction()]->town->clientInfo.buildingsIcons);
		case ComponentType::HERO_PORTRAIT:
			return gen(heroArr);
		case ComponentType::FLAG:
			return gen(flagArr);
		default:
			assert(0);
			return {};
	}
}

size_t CComponent::getIndex() const
{
	switch(data.type)
	{
		case ComponentType::NONE:
			return 0;
		case ComponentType::PRIM_SKILL:
			return data.subType.getNum();
		case ComponentType::EXPERIENCE:
		case ComponentType::LEVEL:
			return 4; // for whatever reason, in H3 experience icon is located in primary skills icons
		case ComponentType::MANA:
			return 5; // for whatever reason, in H3 mana points icon is located in primary skills icons
		case ComponentType::SEC_SKILL:
			return data.subType.getNum() * 3 + 3 + data.value.value_or(1) - 1;
		case ComponentType::RESOURCE:
		case ComponentType::RESOURCE_PER_DAY:
			return data.subType.getNum();
		case ComponentType::CREATURE:
			return LIBRARY->creatures()->getById(data.subType.as<CreatureID>())->getIconIndex();
		case ComponentType::ARTIFACT:
			return LIBRARY->artifacts()->getById(data.subType.as<ArtifactID>())->getIconIndex();
		case ComponentType::SPELL_SCROLL:
		case ComponentType::SPELL:
			return (size < large) ? data.subType.getNum() + 1 : data.subType.getNum();
		case ComponentType::MORALE:
			return std::clamp(data.value.value_or(0) + 3, 0, 6);
		case ComponentType::LUCK:
			return std::clamp(data.value.value_or(0) + 3, 0, 6);
		case ComponentType::BUILDING:
			return data.subType.as<BuildingTypeUniqueID>().getBuilding().getNum();
		case ComponentType::HERO_PORTRAIT:
			return LIBRARY->heroTypes()->getById(data.subType.as<HeroTypeID>())->getIconIndex();
		case ComponentType::FLAG:
			return data.subType.getNum();
		default:
			assert(0);
			return 0;
	}
}

std::string CComponent::getDescription() const
{
	switch(data.type)
	{
		case ComponentType::PRIM_SKILL:
			return LIBRARY->generaltexth->arraytxt[2+data.subType.getNum()];
		case ComponentType::EXPERIENCE:
		case ComponentType::LEVEL:
			return LIBRARY->generaltexth->allTexts[241];
		case ComponentType::MANA:
			return LIBRARY->generaltexth->allTexts[149];
		case ComponentType::SEC_SKILL:
			return LIBRARY->skillh->getByIndex(data.subType.getNum())->getDescriptionTranslated(data.value.value_or(1));
		case ComponentType::RESOURCE:
		case ComponentType::RESOURCE_PER_DAY:
			return LIBRARY->generaltexth->allTexts[242];
		case ComponentType::NONE:
		case ComponentType::CREATURE:
			return "";
		case ComponentType::ARTIFACT:
			return LIBRARY->artifacts()->getById(data.subType.as<ArtifactID>())->getDescriptionTranslated();
		case ComponentType::SPELL_SCROLL:
		{
			auto description = ArtifactID(ArtifactID::SPELL_SCROLL).toEntity(LIBRARY)->getDescriptionTranslated();
			ArtifactUtils::insertScrrollSpellName(description, data.subType.as<SpellID>());
			return description;
		}
		case ComponentType::SPELL:
			return LIBRARY->spells()->getById(data.subType.as<SpellID>())->getDescriptionTranslated(std::max(0, data.value.value_or(0)));
		case ComponentType::MORALE:
			return LIBRARY->generaltexth->heroscrn[ 4 - (data.value.value_or(0)>0) + (data.value.value_or(0)<0)];
		case ComponentType::LUCK:
			return LIBRARY->generaltexth->heroscrn[ 7 - (data.value.value_or(0)>0) + (data.value.value_or(0)<0)];
		case ComponentType::BUILDING:
		{
			auto index = data.subType.as<BuildingTypeUniqueID>();
			return (*LIBRARY->townh)[index.getFaction()]->town->buildings[index.getBuilding()]->getDescriptionTranslated();
		}
		case ComponentType::HERO_PORTRAIT:
			return "";
		case ComponentType::FLAG:
			return "";
		default:
			assert(0);
			return "";
	}
}

std::string CComponent::getSubtitle() const
{
	if (!customSubtitle.empty())
		return customSubtitle;

	switch(data.type)
	{
		case ComponentType::PRIM_SKILL:
			if (data.value)
				return boost::str(boost::format("%+d %s") % data.value.value_or(0) % LIBRARY->generaltexth->primarySkillNames[data.subType.getNum()]);
			else
				return LIBRARY->generaltexth->primarySkillNames[data.subType.getNum()];
		case ComponentType::EXPERIENCE:
			return std::to_string(data.value.value_or(0));
		case ComponentType::LEVEL:
		{
			std::string level = LIBRARY->generaltexth->allTexts[442];
			boost::replace_first(level, "1", std::to_string(data.value.value_or(0)));
			return level;
		}
		case ComponentType::MANA:
			return boost::str(boost::format("%+d %s") % data.value.value_or(0) % LIBRARY->generaltexth->allTexts[387]);
		case ComponentType::SEC_SKILL:
			if (data.value)
				return LIBRARY->generaltexth->levels[data.value.value_or(1)-1] + "\n" + LIBRARY->skillh->getById(data.subType.as<SecondarySkill>())->getNameTranslated();
			else
				return LIBRARY->skillh->getById(data.subType.as<SecondarySkill>())->getNameTranslated();
		case ComponentType::RESOURCE:
			return std::to_string(data.value.value_or(0));
		case ComponentType::RESOURCE_PER_DAY:
			return boost::str(boost::format(LIBRARY->generaltexth->allTexts[3]) % data.value.value_or(0));
		case ComponentType::CREATURE:
		{
			auto creature = LIBRARY->creh->getById(data.subType.as<CreatureID>());
			if(data.value)
				return std::to_string(*data.value) + " " + (*data.value > 1 ? creature->getNamePluralTranslated() : creature->getNameSingularTranslated());
			else
				return creature->getNamePluralTranslated();
		}
		case ComponentType::ARTIFACT:
			return LIBRARY->artifacts()->getById(data.subType.as<ArtifactID>())->getNameTranslated();
		case ComponentType::SPELL_SCROLL:
		case ComponentType::SPELL:
			if (data.value.value_or(0) < 0)
				return "{#A9A9A9|" + LIBRARY->spells()->getById(data.subType.as<SpellID>())->getNameTranslated() + "}";
			else
				return LIBRARY->spells()->getById(data.subType.as<SpellID>())->getNameTranslated();
		case ComponentType::NONE:
		case ComponentType::MORALE:
		case ComponentType::LUCK:
		case ComponentType::HERO_PORTRAIT:
			return "";
		case ComponentType::BUILDING:
			{
				auto index = data.subType.as<BuildingTypeUniqueID>();
				const auto & building = (*LIBRARY->townh)[index.getFaction()]->town->buildings[index.getBuilding()];
				if(!building)
				{
					logGlobal->error("Town of faction %s has no building #%d", (*LIBRARY->townh)[index.getFaction()]->town->faction->getNameTranslated(), index.getBuilding().getNum());
					return (boost::format("Missing building #%d") % index.getBuilding().getNum()).str();
				}
				return building->getNameTranslated();
			}
		case ComponentType::FLAG:
			return LIBRARY->generaltexth->capColors[data.subType.as<PlayerColor>().getNum()];
		default:
			assert(0);
			return "";
	}
}

void CComponent::setSurface(const AnimationPath & defName, int imgPos)
{
	OBJECT_CONSTRUCTION;
	image = std::make_shared<CAnimImage>(defName, imgPos);
}

void CComponent::showPopupWindow(const Point & cursorPosition)
{
	if(!getDescription().empty())
		CRClickPopup::createAndPush(getDescription());
}

void CSelectableComponent::clickPressed(const Point & cursorPosition)
{
	if(onSelect)
		onSelect();
}

void CSelectableComponent::clickDouble(const Point & cursorPosition)
{
	if (!selected)
	{
		clickPressed(cursorPosition);
	}
	else
	{
		if (onChoose)
			onChoose();
	}
}

void CSelectableComponent::init()
{
	selected = false;
}

CSelectableComponent::CSelectableComponent(const Component &c, std::function<void()> OnSelect):
	CComponent(c),onSelect(OnSelect)
{
	setRedrawParent(true);
	addUsedEvents(LCLICK | DOUBLECLICK | KEYBOARD);
	init();
}

CSelectableComponent::CSelectableComponent(ComponentType Type, ComponentSubType Sub, int Val, ESize imageSize, std::function<void()> OnSelect):
	CComponent(Type,Sub,Val, imageSize),onSelect(OnSelect)
{
	setRedrawParent(true);
	addUsedEvents(LCLICK | DOUBLECLICK | KEYBOARD);
	init();
}

void CSelectableComponent::select(bool on)
{
	if(on != selected)
	{
		selected = on;
		redraw();
	}
}

void CSelectableComponent::showAll(Canvas & to)
{
	CComponent::showAll(to);
	if(selected)
	{
		to.drawBorder(Rect::createAround(image->pos, 1), Colors::BRIGHT_YELLOW);
	}
}

void CComponentBox::selectionChanged(std::shared_ptr<CSelectableComponent> newSelection)
{
	if (newSelection == selected)
		return;

	if (selected)
		selected->select(false);

	selected = newSelection;
	if (onSelect)
		onSelect(selectedIndex());

	if (selected)
		selected->select(true);
}

int CComponentBox::selectedIndex()
{
	if (selected)
		return static_cast<int>(std::find(components.begin(), components.end(), selected) - components.begin());
	return -1;
}

Point CComponentBox::getOrTextPos(CComponent *left, CComponent *right)
{
	int leftSubtitle  = ( left->pos.w -  left->image->pos.w) / 2;
	int rightSubtitle = (right->pos.w - right->image->pos.w) / 2;
	int fullDistance = getDistance(left, right) + leftSubtitle + rightSubtitle;

	return Point(fullDistance/2 - leftSubtitle, (left->image->pos.h + right->image->pos.h) / 4);
}

int CComponentBox::getDistance(CComponent *left, CComponent *right)
{
	int leftSubtitle  = ( left->pos.w -  left->image->pos.w) / 2;
	int rightSubtitle = (right->pos.w - right->image->pos.w) / 2;
	int subtitlesOffset = leftSubtitle + rightSubtitle;

	return std::max(betweenSubtitlesMin, betweenImagesMin - subtitlesOffset);
}

void CComponentBox::placeComponents(bool selectable)
{
	OBJECT_CONSTRUCTION;
	if (components.empty())
		return;

	//prepare components
	for(auto & comp : components)
	{
		addChild(comp.get());
		comp->moveTo(Point(pos.x, pos.y));
	}

	struct RowData
	{
		size_t comps;
		int width;
		int height;
		RowData (size_t Comps, int Width, int Height):
		comps(Comps), width (Width), height (Height){};
	};
	std::vector<RowData> rows;
	rows.push_back (RowData (0,0,0));

	//split components in rows
	std::shared_ptr<CComponent> prevComp;

	for(std::shared_ptr<CComponent> comp : components)
	{
		//make sure that components are smaller than our width
		//assert(pos.w == 0 || pos.w < comp->pos.w);

		const int distance = prevComp ? getDistance(prevComp.get(), comp.get()) : 0;

		//start next row
		if ((pos.w != 0 && rows.back().width + comp->pos.w + distance > pos.w) // row is full
			|| rows.back().comps >= componentsInRow
			|| (prevComp && prevComp->newLine))
		{
			prevComp = nullptr;
			rows.push_back (RowData (0,0,0));
		}

		if (prevComp)
			rows.back().width += distance;

		rows.back().comps++;
		rows.back().width += comp->pos.w;

		vstd::amax(rows.back().height, comp->pos.h);
		prevComp = comp;
	}

	if (pos.w == 0)
	{
		for(auto & row : rows)
			vstd::amax(pos.w, row.width);
	}

	int height = ((int)rows.size() - 1) * betweenRows;
	for(auto & row : rows)
		height += row.height;

	//assert(pos.h == 0 || pos.h < height);
	if (pos.h == 0)
		pos.h = height;

	auto iter = components.begin();
	int currentY = (pos.h - height) / 2;

	//move components to their positions
	for (auto & rows_row : rows)
	{
		// amount of free space we may add on each side of every component
		int freeSpace = (pos.w - rows_row.width) / ((int)rows_row.comps * 2);
		prevComp = nullptr;

		int currentX = 0;
		for (size_t col = 0; col < rows_row.comps; col++)
		{
			currentX += freeSpace;
			if (prevComp)
			{
				if (selectable)
				{
					Point orPos = Point(currentX - freeSpace, currentY) + getOrTextPos(prevComp.get(), iter->get());

					orLabels.push_back(std::make_shared<CLabel>(orPos.x, orPos.y, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->allTexts[4]));
				}
				currentX += getDistance(prevComp.get(), iter->get());
			}

			(*iter)->moveBy(Point(currentX, currentY));
			currentX += (*iter)->pos.w;
			currentX += freeSpace;

			prevComp = *(iter++);
		}
		currentY += rows_row.height + betweenRows;
	}
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CComponent>> _components, Rect position):
	CComponentBox(_components, position, defaultBetweenImagesMin, defaultBetweenSubtitlesMin, defaultBetweenRows, defaultComponentsInRow)
{
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CComponent>> _components, Rect position, int betweenImagesMin, int betweenSubtitlesMin, int betweenRows, int componentsInRow):
	components(_components),
	betweenImagesMin(betweenImagesMin),
	betweenSubtitlesMin(betweenSubtitlesMin),
	betweenRows(betweenRows),
	componentsInRow(componentsInRow)
{
	setRedrawParent(true);
	pos = position + pos.topLeft();
	placeComponents(false);
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CSelectableComponent>> _components, Rect position, std::function<void(int newID)> _onSelect):
	CComponentBox(_components, position, _onSelect, defaultBetweenImagesMin, defaultBetweenSubtitlesMin, defaultBetweenRows, defaultComponentsInRow)
{
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CSelectableComponent>> _components, Rect position, std::function<void(int newID)> _onSelect, int betweenImagesMin, int betweenSubtitlesMin, int betweenRows, int componentsInRow):
	components(_components.begin(), _components.end()),
	onSelect(_onSelect),
	betweenImagesMin(betweenImagesMin),
	betweenSubtitlesMin(betweenSubtitlesMin),
	betweenRows(betweenRows),
	componentsInRow(componentsInRow)
{
	setRedrawParent(true);
	pos = position + pos.topLeft();
	placeComponents(true);

	assert(!components.empty());

	auto key = EShortcut::SELECT_INDEX_1;
	for(auto & comp : _components)
	{
		comp->onSelect = std::bind(&CComponentBox::selectionChanged, this, comp);
		comp->assignedKey = key;
		key = vstd::next(key, 1);
	}
	selectionChanged(_components.front());
}
