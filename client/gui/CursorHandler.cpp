/*
 * CCursorHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CursorHandler.h"

#include "CGuiHandler.h"
#include "../renderSDL/CursorSoftware.h"
#include "../renderSDL/CursorHardware.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../renderSDL/SDL_Extensions.h"
#include "../CMT.h"

#include "../../lib/CConfigHandler.h"

std::unique_ptr<ICursor> CursorHandler::createCursor()
{
	if (settings["video"]["cursor"].String() == "auto")
	{
#if defined(VCMI_ANDROID) || defined(VCMI_IOS)
		return std::make_unique<CursorSoftware>();
#else
		return std::make_unique<CursorHardware>();
#endif
	}

	if (settings["video"]["cursor"].String() == "hardware")
		return std::make_unique<CursorHardware>();

	assert(settings["video"]["cursor"].String() == "software");
	return std::make_unique<CursorSoftware>();
}

CursorHandler::CursorHandler()
	: cursor(createCursor())
	, frameTime(0.f)
	, showing(false)
	, pos(0,0)
{

	dndObject = nullptr;

	cursors =
	{
		std::make_unique<CAnimation>("CRADVNTR"),
		std::make_unique<CAnimation>("CRCOMBAT"),
		std::make_unique<CAnimation>("CRDEFLT"),
		std::make_unique<CAnimation>("CRSPELL")
	};

	for (auto & cursor : cursors)
		cursor->preload();

	set(Cursor::Map::POINTER);
}

Point CursorHandler::position() const
{
	return pos;
}

void CursorHandler::changeGraphic(Cursor::Type type, size_t index)
{
	assert(dndObject == nullptr);

	if (type == this->type && index == this->frame)
		return;

	this->type = type;
	this->frame = index;

	cursor->setImage(getCurrentImage(), getPivotOffset());
}

void CursorHandler::set(Cursor::Default index)
{
	changeGraphic(Cursor::Type::DEFAULT, static_cast<size_t>(index));
}

void CursorHandler::set(Cursor::Map index)
{
	changeGraphic(Cursor::Type::ADVENTURE, static_cast<size_t>(index));
}

void CursorHandler::set(Cursor::Combat index)
{
	changeGraphic(Cursor::Type::COMBAT, static_cast<size_t>(index));
}

void CursorHandler::set(Cursor::Spellcast index)
{
	//Note: this is animated cursor, ignore specified frame and only change type
	changeGraphic(Cursor::Type::SPELLBOOK, frame);
}

void CursorHandler::dragAndDropCursor(std::shared_ptr<IImage> image)
{
	dndObject = image;
	cursor->setImage(getCurrentImage(), getPivotOffset());
}

void CursorHandler::dragAndDropCursor (std::string path, size_t index)
{
	CAnimation anim(path);
	anim.load(index);
	dragAndDropCursor(anim.getImage(index));
}

void CursorHandler::cursorMove(const int & x, const int & y)
{
	pos.x = x;
	pos.y = y;

	cursor->setCursorPosition(pos);
}

Point CursorHandler::getPivotOffsetDefault(size_t index)
{
	return {0, 0};
}

Point CursorHandler::getPivotOffsetMap(size_t index)
{
	static const std::array<Point, 43> offsets = {{
		{  0,  0}, // POINTER          =  0,
		{  0,  0}, // HOURGLASS        =  1,
		{ 12, 10}, // HERO             =  2,
		{ 12, 12}, // TOWN             =  3,

		{ 15, 13}, // T1_MOVE          =  4,
		{ 13, 13}, // T1_ATTACK        =  5,
		{ 16, 32}, // T1_SAIL          =  6,
		{ 13, 20}, // T1_DISEMBARK     =  7,
		{  8,  9}, // T1_EXCHANGE      =  8,
		{ 14, 16}, // T1_VISIT         =  9,

		{ 15, 13}, // T2_MOVE          = 10,
		{ 13, 13}, // T2_ATTACK        = 11,
		{ 16, 32}, // T2_SAIL          = 12,
		{ 13, 20}, // T2_DISEMBARK     = 13,
		{  8,  9}, // T2_EXCHANGE      = 14,
		{ 14, 16}, // T2_VISIT         = 15,

		{ 15, 13}, // T3_MOVE          = 16,
		{ 13, 13}, // T3_ATTACK        = 17,
		{ 16, 32}, // T3_SAIL          = 18,
		{ 13, 20}, // T3_DISEMBARK     = 19,
		{  8,  9}, // T3_EXCHANGE      = 20,
		{ 14, 16}, // T3_VISIT         = 21,

		{ 15, 13}, // T4_MOVE          = 22,
		{ 13, 13}, // T4_ATTACK        = 23,
		{ 16, 32}, // T4_SAIL          = 24,
		{ 13, 20}, // T4_DISEMBARK     = 25,
		{  8,  9}, // T4_EXCHANGE      = 26,
		{ 14, 16}, // T4_VISIT         = 27,

		{ 16, 32}, // T1_SAIL_VISIT    = 28,
		{ 16, 32}, // T2_SAIL_VISIT    = 29,
		{ 16, 32}, // T3_SAIL_VISIT    = 30,
		{ 16, 32}, // T4_SAIL_VISIT    = 31,

		{  6,  1}, // SCROLL_NORTH     = 32,
		{ 16,  2}, // SCROLL_NORTHEAST = 33,
		{ 21,  6}, // SCROLL_EAST      = 34,
		{ 16, 16}, // SCROLL_SOUTHEAST = 35,
		{  6, 21}, // SCROLL_SOUTH     = 36,
		{  1, 16}, // SCROLL_SOUTHWEST = 37,
		{  1,  5}, // SCROLL_WEST      = 38,
		{  2,  1}, // SCROLL_NORTHWEST = 39,

		{  0,  0}, // POINTER_COPY     = 40,
		{ 14, 16}, // TELEPORT         = 41,
		{ 20, 20}, // SCUTTLE_BOAT     = 42
	}};

	assert(offsets.size() == size_t(Cursor::Map::COUNT)); //Invalid number of pivot offsets for cursor
	assert(index < offsets.size());
	return offsets[index];
}

Point CursorHandler::getPivotOffsetCombat(size_t index)
{
	static const std::array<Point, 20> offsets = {{
		{ 12, 12 }, // BLOCKED        = 0,
		{ 10, 14 }, // MOVE           = 1,
		{ 14, 14 }, // FLY            = 2,
		{ 12, 12 }, // SHOOT          = 3,
		{ 12, 12 }, // HERO           = 4,
		{  8, 12 }, // QUERY          = 5,
		{  0,  0 }, // POINTER        = 6,
		{ 21,  0 }, // HIT_NORTHEAST  = 7,
		{ 31,  5 }, // HIT_EAST       = 8,
		{ 21, 21 }, // HIT_SOUTHEAST  = 9,
		{  0, 21 }, // HIT_SOUTHWEST  = 10,
		{  0,  5 }, // HIT_WEST       = 11,
		{  0,  0 }, // HIT_NORTHWEST  = 12,
		{  6,  0 }, // HIT_NORTH      = 13,
		{  6, 31 }, // HIT_SOUTH      = 14,
		{ 14,  0 }, // SHOOT_PENALTY  = 15,
		{ 12, 12 }, // SHOOT_CATAPULT = 16,
		{ 12, 12 }, // HEAL           = 17,
		{ 12, 12 }, // SACRIFICE      = 18,
		{ 14, 20 }, // TELEPORT       = 19
	}};

	assert(offsets.size() == size_t(Cursor::Combat::COUNT)); //Invalid number of pivot offsets for cursor
	assert(index < offsets.size());
	return offsets[index];
}

Point CursorHandler::getPivotOffsetSpellcast()
{
	return { 18, 28};
}

Point CursorHandler::getPivotOffset()
{
	if (dndObject)
		return dndObject->dimensions() / 2;

	switch (type) {
	case Cursor::Type::ADVENTURE: return getPivotOffsetMap(frame);
	case Cursor::Type::COMBAT:    return getPivotOffsetCombat(frame);
	case Cursor::Type::DEFAULT:   return getPivotOffsetDefault(frame);
	case Cursor::Type::SPELLBOOK: return getPivotOffsetSpellcast();
	};

	assert(0);
	return {0, 0};
}

std::shared_ptr<IImage> CursorHandler::getCurrentImage()
{
	if (dndObject)
		return dndObject;

	return cursors[static_cast<size_t>(type)]->getImage(frame);
}

void CursorHandler::updateSpellcastCursor()
{
	static const float frameDisplayDuration = 0.1f; // H3 uses 100 ms per frame

	frameTime += GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	size_t newFrame = frame;

	while (frameTime >= frameDisplayDuration)
	{
		frameTime -= frameDisplayDuration;
		newFrame++;
	}

	auto & animation = cursors.at(static_cast<size_t>(type));

	while (newFrame >= animation->size())
		newFrame -= animation->size();

	changeGraphic(Cursor::Type::SPELLBOOK, newFrame);
}

void CursorHandler::render()
{
	if(!showing)
		return;

	if (type == Cursor::Type::SPELLBOOK)
		updateSpellcastCursor();

	cursor->render();
}

void CursorHandler::hide()
{
	if (!showing)
		return;

	showing = false;
	cursor->setVisible(false);
}

void CursorHandler::show()
{
	if (showing)
		return;

	showing = true;
	cursor->setVisible(true);
}

