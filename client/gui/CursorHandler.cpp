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

#include <SDL.h>

#include "SDL_Extensions.h"
#include "CGuiHandler.h"
#include "../widgets/Images.h"

#include "../CMT.h"

void CursorHandler::clearBuffer()
{
	Uint32 fillColor = SDL_MapRGBA(buffer->format, 0, 0, 0, 0);
	CSDL_Ext::fillRect(buffer, nullptr, fillColor);
}

void CursorHandler::updateBuffer(CIntObject * payload)
{
	payload->moveTo(Point(0,0));
	payload->showAll(buffer);

	needUpdate = true;
}

void CursorHandler::replaceBuffer(CIntObject * payload)
{
	clearBuffer();
	updateBuffer(payload);
}

CursorHandler::CursorHandler()
	: needUpdate(true)
	, buffer(nullptr)
	, cursorLayer(nullptr)
	, frameTime(0.f)
	, showing(false)
	, pos(0,0)
{
	cursorLayer = SDL_CreateTexture(mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 40, 40);
	SDL_SetTextureBlendMode(cursorLayer, SDL_BLENDMODE_BLEND);

	type = Cursor::Type::DEFAULT;
	dndObject = nullptr;

	cursors =
	{
		std::make_unique<CAnimImage>("CRADVNTR", 0),
		std::make_unique<CAnimImage>("CRCOMBAT", 0),
		std::make_unique<CAnimImage>("CRDEFLT",  0),
		std::make_unique<CAnimImage>("CRSPELL",  0)
	};

	currentCursor = cursors.at(static_cast<size_t>(Cursor::Type::DEFAULT)).get();

	buffer = CSDL_Ext::newSurface(40,40);

	SDL_SetSurfaceBlendMode(buffer, SDL_BLENDMODE_NONE);
	SDL_ShowCursor(SDL_DISABLE);

	set(Cursor::Map::POINTER);
}

Point CursorHandler::position() const
{
	return pos;
}

void CursorHandler::changeGraphic(Cursor::Type type, size_t index)
{
	assert(dndObject == nullptr);

	if(type != this->type)
	{
		this->type = type;
		this->frame = index;
		currentCursor = cursors.at(static_cast<size_t>(type)).get();
		currentCursor->setFrame(index);
	}
	else if(index != this->frame)
	{
		this->frame = index;
		currentCursor->setFrame(index);
	}

	replaceBuffer(currentCursor);
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

void CursorHandler::dragAndDropCursor(std::unique_ptr<CAnimImage> object)
{
	dndObject = std::move(object);
	if(dndObject)
		replaceBuffer(dndObject.get());
	else
		replaceBuffer(currentCursor);
}

void CursorHandler::cursorMove(const int & x, const int & y)
{
	pos.x = x;
	pos.y = y;
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
		{ 20, 20}, // T1_SAIL          =  6,
		{ 13, 16}, // T1_DISEMBARK     =  7,
		{  8,  9}, // T1_EXCHANGE      =  8,
		{ 14, 16}, // T1_VISIT         =  9,

		{ 15, 13}, // T2_MOVE          = 10,
		{ 13, 13}, // T2_ATTACK        = 11,
		{ 20, 20}, // T2_SAIL          = 12,
		{ 13, 16}, // T2_DISEMBARK     = 13,
		{  8,  9}, // T2_EXCHANGE      = 14,
		{ 14, 16}, // T2_VISIT         = 15,

		{ 15, 13}, // T3_MOVE          = 16,
		{ 13, 13}, // T3_ATTACK        = 17,
		{ 20, 20}, // T3_SAIL          = 18,
		{ 13, 16}, // T3_DISEMBARK     = 19,
		{  8,  9}, // T3_EXCHANGE      = 20,
		{ 14, 16}, // T3_VISIT         = 21,

		{ 15, 13}, // T4_MOVE          = 22,
		{ 13, 13}, // T4_ATTACK        = 23,
		{ 20, 20}, // T4_SAIL          = 24,
		{ 13, 16}, // T4_DISEMBARK     = 25,
		{  8,  9}, // T4_EXCHANGE      = 26,
		{ 14, 16}, // T4_VISIT         = 27,

		{ 20, 20}, // T1_SAIL_VISIT    = 28,
		{ 20, 20}, // T2_SAIL_VISIT    = 29,
		{ 20, 20}, // T3_SAIL_VISIT    = 30,
		{ 20, 20}, // T4_SAIL_VISIT    = 31,

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

	static_assert (offsets.size() == size_t(Cursor::Map::COUNT), "Invalid number of pivot offsets for cursor" );
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

	static_assert (offsets.size() == size_t(Cursor::Combat::COUNT), "Invalid number of pivot offsets for cursor" );
	assert(index < offsets.size());
	return offsets[index];
}

Point CursorHandler::getPivotOffsetSpellcast()
{
	return { 18, 28};
}

Point CursorHandler::getPivotOffset()
{
	switch (type) {
	case Cursor::Type::ADVENTURE: return getPivotOffsetMap(frame);
	case Cursor::Type::COMBAT:    return getPivotOffsetCombat(frame);
	case Cursor::Type::DEFAULT:   return getPivotOffsetDefault(frame);
	case Cursor::Type::SPELLBOOK: return getPivotOffsetSpellcast();
	};

	assert(0);
	return {0, 0};
}

void CursorHandler::centerCursor()
{
	pos.x = static_cast<int>((screen->w / 2.) - (currentCursor->pos.w / 2.));
	pos.y = static_cast<int>((screen->h / 2.) - (currentCursor->pos.h / 2.));
	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
	SDL_WarpMouse(pos.x, pos.y);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
}

void CursorHandler::updateSpellcastCursor()
{
	static const float frameDisplayDuration = 0.1f;

	frameTime += GH.mainFPSmng->getElapsedMilliseconds() / 1000.f;
	size_t newFrame = frame;

	while (frameTime > frameDisplayDuration)
	{
		frameTime -= frameDisplayDuration;
		newFrame++;
	}

	auto & animation = cursors.at(static_cast<size_t>(type));

	while (newFrame > animation->size())
		newFrame -= animation->size();

	changeGraphic(Cursor::Type::SPELLBOOK, newFrame);
}

void CursorHandler::render()
{
	if(!showing)
		return;

	if (type == Cursor::Type::SPELLBOOK)
		updateSpellcastCursor();


	//the must update texture in the main (renderer) thread, but changes to cursor type may come from other threads
	updateTexture();

	Point renderPos = pos;

	if(dndObject)
		renderPos -= dndObject->pos.dimensions() / 2;
	else
		renderPos -= getPivotOffset();

	SDL_Rect destRect;
	destRect.x = renderPos.x;
	destRect.y = renderPos.y;
	destRect.w = 40;
	destRect.h = 40;

	SDL_RenderCopy(mainRenderer, cursorLayer, nullptr, &destRect);
}

void CursorHandler::updateTexture()
{
	if(needUpdate)
	{
		SDL_UpdateTexture(cursorLayer, nullptr, buffer->pixels, buffer->pitch);
		needUpdate = false;
	}
}

CursorHandler::~CursorHandler()
{
	if(buffer)
		SDL_FreeSurface(buffer);

	if(cursorLayer)
		SDL_DestroyTexture(cursorLayer);
}
