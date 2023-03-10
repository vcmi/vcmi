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

#include "CArtifactHolder.h"
#include "Images.h"

#include <vcmi/spells/Service.h>
#include <vcmi/spells/Spell.h>

#include "../gui/CGuiHandler.h"
#include "../gui/CursorHandler.h"
#include "../gui/TextAlignment.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../windows/CMessage.h"
#include "../windows/InfoWindows.h"
#include "../widgets/TextControls.h"
#include "../CGameInfo.h"

#include "../../lib/CTownHandler.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CSkillHandler.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/NetPacksBase.h"
#include "../../lib/CArtHandler.h"

CComponent::CComponent(Etype Type, int Subtype, int Val, ESize imageSize, EFonts font):
	perDay(false)
{
	init(Type, Subtype, Val, imageSize, font);
}

CComponent::CComponent(const Component & c, ESize imageSize, EFonts font)
	: perDay(false)
{
	if(c.id == Component::RESOURCE && c.when==-1)
		perDay = true;

	init((Etype)c.id, c.subtype, c.val, imageSize, font);
}

void CComponent::init(Etype Type, int Subtype, int Val, ESize imageSize, EFonts fnt)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);

	addUsedEvents(RCLICK);

	compType = Type;
	subtype = Subtype;
	val = Val;
	size = imageSize;
	font = fnt;

	assert(compType < typeInvalid);
	assert(size < sizeInvalid);

	setSurface(getFileName()[size], (int)getIndex());

	pos.w = image->pos.w;
	pos.h = image->pos.h;

	if (imageSize < small)
		font = FONT_TINY; //for tiny images - tiny font

	pos.h += 4; //distance between text and image

	auto max = 80;
	if (size < large)
		max = 60;
	if (size < medium)
		max = 40;
	if (size < small)
		max = 30;

	std::vector<std::string> textLines = CMessage::breakText(getSubtitle(), std::max<int>(max, pos.w), font);
	for(auto & line : textLines)
	{
		int height = static_cast<int>(graphics->fonts[font]->getLineHeight());
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

const std::vector<std::string> CComponent::getFileName()
{
	static const std::string  primSkillsArr [] = {"PSKIL32",        "PSKIL32",        "PSKIL42",        "PSKILL"};
	static const std::string  secSkillsArr [] =  {"SECSK32",        "SECSK32",        "SECSKILL",       "SECSK82"};
	static const std::string  resourceArr [] =   {"SMALRES",        "RESOURCE",       "RESOURCE",       "RESOUR82"};
	static const std::string  creatureArr [] =   {"CPRSMALL",       "CPRSMALL",       "CPRSMALL",       "TWCRPORT"};
	static const std::string  artifactArr[]  =   {"Artifact",       "Artifact",       "Artifact",       "Artifact"};
	static const std::string  spellsArr [] =     {"SpellInt",       "SpellInt",       "SpellInt",       "SPELLSCR"};
	static const std::string  moraleArr [] =     {"IMRL22",         "IMRL30",         "IMRL42",         "imrl82"};
	static const std::string  luckArr [] =       {"ILCK22",         "ILCK30",         "ILCK42",         "ilck82"};
	static const std::string  heroArr [] =       {"PortraitsSmall", "PortraitsSmall", "PortraitsSmall", "PortraitsLarge"};
	static const std::string  flagArr [] =       {"CREST58",        "CREST58",        "CREST58",        "CREST58"};

	auto gen = [](const std::string * arr)
	{
		return std::vector<std::string>(arr, arr + 4);
	};

	switch(compType)
	{
	case primskill:  return gen(primSkillsArr);
	case secskill:   return gen(secSkillsArr);
	case resource:   return gen(resourceArr);
	case creature:   return gen(creatureArr);
	case artifact:   return gen(artifactArr);
	case experience: return gen(primSkillsArr);
	case spell:      return gen(spellsArr);
	case morale:     return gen(moraleArr);
	case luck:       return gen(luckArr);
	case building:   return std::vector<std::string>(4, (*CGI->townh)[subtype]->town->clientInfo.buildingsIcons);
	case hero:       return gen(heroArr);
	case flag:       return gen(flagArr);
	}
	assert(0);
	return std::vector<std::string>();
}

size_t CComponent::getIndex()
{
	switch(compType)
	{
	case primskill:  return subtype;
	case secskill:   return subtype*3 + 3 + val - 1;
	case resource:   return subtype;
	case creature:   return CGI->creatures()->getByIndex(subtype)->getIconIndex();
	case artifact:   return CGI->artifacts()->getByIndex(subtype)->getIconIndex();
	case experience: return 4;
	case spell:      return subtype;
	case morale:     return val+3;
	case luck:       return val+3;
	case building:   return val;
	case hero:       return subtype;
	case flag:       return subtype;
	}
	assert(0);
	return 0;
}

std::string CComponent::getDescription()
{
	switch(compType)
	{
	case primskill:  return (subtype < 4)? CGI->generaltexth->arraytxt[2+subtype] //Primary skill
										 : CGI->generaltexth->allTexts[149]; //mana
	case secskill:   return CGI->skillh->getByIndex(subtype)->getDescriptionTranslated(val);
	case resource:   return CGI->generaltexth->allTexts[242];
	case creature:   return "";
	case artifact:
	{
		auto artID = ArtifactID(subtype);
		std::unique_ptr<CArtifactInstance> art;
		if (artID != ArtifactID::SPELL_SCROLL)
		{
			art.reset(CArtifactInstance::createNewArtifactInstance(artID));
		}
		else
		{
			art.reset(CArtifactInstance::createScroll(SpellID(val)));
		}
		return art->getEffectiveDescription();
	}
	case experience: return CGI->generaltexth->allTexts[241];
	case spell:      return (*CGI->spellh)[subtype]->getDescriptionTranslated(val);
	case morale:     return CGI->generaltexth->heroscrn[ 4 - (val>0) + (val<0)];
	case luck:       return CGI->generaltexth->heroscrn[ 7 - (val>0) + (val<0)];
	case building:   return (*CGI->townh)[subtype]->town->buildings[BuildingID(val)]->getDescriptionTranslated();
	case hero:       return "";
	case flag:       return "";
	}
	assert(0);
	return "";
}

std::string CComponent::getSubtitle()
{
	if(!perDay)
		return getSubtitleInternal();

	std::string ret = CGI->generaltexth->allTexts[3];
	boost::replace_first(ret, "%d", getSubtitleInternal());
	return ret;
}

std::string CComponent::getSubtitleInternal()
{
	//FIXME: some of these are horrible (e.g creature)
	switch(compType)
	{
	case primskill:  return boost::str(boost::format("%+d %s") % val % (subtype < 4 ? CGI->generaltexth->primarySkillNames[subtype] : CGI->generaltexth->allTexts[387]));
	case secskill:   return CGI->generaltexth->levels[val-1] + "\n" + CGI->skillh->getByIndex(subtype)->getNameTranslated();
	case resource:   return std::to_string(val);
	case creature:
		{
			auto creature = CGI->creh->getByIndex(subtype);
			if ( val )
				return std::to_string(val) + " " + (val > 1 ? creature->getNamePluralTranslated() : creature->getNameSingularTranslated());
			else
				return val > 1 ? creature->getNamePluralTranslated() : creature->getNameSingularTranslated();
		}
	case artifact:   return CGI->artifacts()->getByIndex(subtype)->getNameTranslated();
	case experience:
		{
			if(subtype == 1) //+1 level - tree of knowledge
			{
				std::string level = CGI->generaltexth->allTexts[442];
				boost::replace_first(level, "1", std::to_string(val));
				return level;
			}
			else
			{
				return std::to_string(val); //amount of experience OR level required for seer hut;
			}
		}
	case spell:      return CGI->spells()->getByIndex(subtype)->getNameTranslated();
	case morale:     return "";
	case luck:       return "";
	case building:
		{
			auto building = (*CGI->townh)[subtype]->town->buildings[BuildingID(val)];
			if(!building)
			{
				logGlobal->error("Town of faction %s has no building #%d", (*CGI->townh)[subtype]->town->faction->getNameTranslated(), val);
				return (boost::format("Missing building #%d") % val).str();
			}
			return building->getNameTranslated();
		}
	case hero:       return "";
	case flag:       return CGI->generaltexth->capColors[subtype];
	}
	logGlobal->error("Invalid CComponent type: %d", (int)compType);
	return "";
}

void CComponent::setSurface(std::string defName, int imgPos)
{
	OBJECT_CONSTRUCTION_CUSTOM_CAPTURING(255-DISPOSE);
	image = std::make_shared<CAnimImage>(defName, imgPos);
}

void CComponent::clickRight(tribool down, bool previousState)
{
	if(down && !getDescription().empty())
		CRClickPopup::createAndPush(getDescription());
}

void CSelectableComponent::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		if(onSelect)
			onSelect();
	}
}

void CSelectableComponent::init()
{
	selected = false;
}

CSelectableComponent::CSelectableComponent(const Component &c, std::function<void()> OnSelect):
	CComponent(c),onSelect(OnSelect)
{
	type |= REDRAW_PARENT;
	addUsedEvents(LCLICK | KEYBOARD);
	init();
}

CSelectableComponent::CSelectableComponent(Etype Type, int Sub, int Val, ESize imageSize, std::function<void()> OnSelect):
	CComponent(Type,Sub,Val, imageSize),onSelect(OnSelect)
{
	type |= REDRAW_PARENT;
	addUsedEvents(LCLICK | KEYBOARD);
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

void CSelectableComponent::showAll(SDL_Surface * to)
{
	CComponent::showAll(to);
	if(selected)
	{
		CSDL_Ext::drawBorder(to, Rect::createAround(image->pos, 1), Colors::BRIGHT_YELLOW);
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
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
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
			|| rows.back().comps >= 4) // no more than 4 comps per row
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

					orLabels.push_back(std::make_shared<CLabel>(orPos.x, orPos.y, FONT_MEDIUM, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[4]));
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
	CComponentBox(_components, position, defaultBetweenImagesMin, defaultBetweenSubtitlesMin, defaultBetweenRows)
{
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CComponent>> _components, Rect position, int betweenImagesMin, int betweenSubtitlesMin, int betweenRows):
	components(_components),
	betweenImagesMin(betweenImagesMin),
	betweenSubtitlesMin(betweenSubtitlesMin),
	betweenRows(betweenRows)
{
	type |= REDRAW_PARENT;
	pos = position + pos.topLeft();
	placeComponents(false);
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CSelectableComponent>> _components, Rect position, std::function<void(int newID)> _onSelect):
	CComponentBox(_components, position, _onSelect, defaultBetweenImagesMin, defaultBetweenSubtitlesMin, defaultBetweenRows)
{
}

CComponentBox::CComponentBox(std::vector<std::shared_ptr<CSelectableComponent>> _components, Rect position, std::function<void(int newID)> _onSelect, int betweenImagesMin, int betweenSubtitlesMin, int betweenRows):
	components(_components.begin(), _components.end()),
	onSelect(_onSelect),
	betweenImagesMin(betweenImagesMin),
	betweenSubtitlesMin(betweenSubtitlesMin),
	betweenRows(betweenRows)
{
	type |= REDRAW_PARENT;
	pos = position + pos.topLeft();
	placeComponents(true);

	assert(!components.empty());

	int key = SDLK_1;
	for(auto & comp : _components)
	{
		comp->onSelect = std::bind(&CComponentBox::selectionChanged, this, comp);
		comp->assignedKeys.insert(key++);
	}
	selectionChanged(_components.front());
}
