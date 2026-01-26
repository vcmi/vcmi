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

#include "GameEngine.h"
#include "FramerateManager.h"
#include "../renderSDL/CursorSoftware.h"
#include "../renderSDL/CursorHardware.h"
#include "../render/CAnimation.h"
#include "../render/IImage.h"
#include "../render/IScreenHandler.h"
#include "../render/IRenderHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/json/JsonUtils.h"

std::unique_ptr<ICursor> CursorHandler::createCursor()
{
#if defined(VCMI_MOBILE) || defined(VCMI_PORTMASTER)
	if (settings["general"]["userRelativePointer"].Bool())
		return std::make_unique<CursorSoftware>();
#endif

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
	, dndObject(nullptr)
{
	showType = dynamic_cast<CursorSoftware *>(cursor.get()) ? Cursor::ShowType::SOFTWARE : Cursor::ShowType::HARDWARE;
}

CursorHandler::~CursorHandler() = default;

void CursorHandler::init()
{
	JsonNode cursorConfig = JsonUtils::assembleFromFiles("config/cursors.json");
	std::vector<AnimationPath> animations;

	for (const auto & cursorEntry : cursorConfig.Struct())
	{
		CursorParameters parameters;
		parameters.cursorID = cursorEntry.first;
		parameters.image = ImagePath::fromJson(cursorEntry.second["image"]);
		parameters.animation = AnimationPath::fromJson(cursorEntry.second["animation"]);
		parameters.animationFrameIndex = cursorEntry.second["frame"].Integer();
		parameters.isAnimated = cursorEntry.second["animated"].Bool();
		parameters.pivot.x = cursorEntry.second["pivotX"].Integer();
		parameters.pivot.y = cursorEntry.second["pivotY"].Integer();

		cursors.push_back(parameters);
	}

	set(Cursor::Map::POINTER);
}

void CursorHandler::set(const std::string & index)
{
	assert(dndObject == nullptr);

	if (index == currentCursorID)
		return;

	currentCursorID = index;
	currentCursorIndex = 0;
	frameTime = 0;
	for (size_t i = 0; i < cursors.size(); ++i)
	{
		if (cursors[i].cursorID == index)
		{
			currentCursorIndex = i;
			break;
		}
	}

	const auto & currentCursor = cursors.at(currentCursorIndex);

	if (currentCursor.image.empty())
	{
		if (!loadedAnimations.count(currentCursor.animation))
			loadedAnimations[currentCursor.animation] = ENGINE->renderHandler().loadAnimation(currentCursor.animation, EImageBlitMode::COLORKEY);

		if (currentCursor.isAnimated)
			cursorImage = loadedAnimations[currentCursor.animation]->getImage(0);
		else
			cursorImage = loadedAnimations[currentCursor.animation]->getImage(currentCursor.animationFrameIndex);
	}
	else
	{
		if (!loadedImages.count(currentCursor.image))
			loadedImages[currentCursor.image] = ENGINE->renderHandler().loadImage(currentCursor.image, EImageBlitMode::COLORKEY);
		cursorImage = loadedImages[currentCursor.image];
	}

	cursor->setImage(getCurrentImage(), getPivotOffset());
}

void CursorHandler::set(Cursor::Map index)
{
	constexpr std::array mapCursorNames =
	{
		"mapPointer",
		"mapHourglass",
		"mapHero",
		"mapTown",
		"mapTurn1Move",
		"mapTurn1Attack",
		"mapTurn1Sail",
		"mapTurn1Disembark",
		"mapTurn1Exchange",
		"mapTurn1Visit",
		"mapTurn2Move",
		"mapTurn2Attack",
		"mapTurn2Sail",
		"mapTurn2Disembark",
		"mapTurn2Exchange",
		"mapTurn2Visit",
		"mapTurn3Move",
		"mapTurn3Attack",
		"mapTurn3Sail",
		"mapTurn3Disembark",
		"mapTurn3Exchange",
		"mapTurn3Visit",
		"mapTurn4Move",
		"mapTurn4Attack",
		"mapTurn4Sail",
		"mapTurn4Disembark",
		"mapTurn4Exchange",
		"mapTurn4Visit",
		"mapTurn1SailVisit",
		"mapTurn2SailVisit",
		"mapTurn3SailVisit",
		"mapTurn4SailVisit",
		"mapScrollNorth",
		"mapScrollNorthEast",
		"mapScrollEast",
		"mapScrollSouthEast",
		"mapScrollSouth",
		"mapScrollSouthWest",
		"mapScrollWest",
		"mapScrollNorthWest",
		"UNUSED",
		"mapDimensionDoor",
		"mapScuttleBoat"
	};

	set(mapCursorNames.at(static_cast<int>(index)));
}

void CursorHandler::set(Cursor::Combat index)
{
	constexpr std::array combatCursorNames =
	{
		"combatBlocked",
		"combatMove",
		"combatFly",
		"combatShoot",
		"combatHero",
		"combatQuery",
		"combatPointer",
		"combatHitNorthEast",
		"combatHitEast",
		"combatHitSouthEast",
		"combatHitSouthWest",
		"combatHitWest",
		"combatHitNorthWest",
		"combatHitNorth",
		"combatHitSouth",
		"combatShootPenalty",
		"combatShootCatapult",
		"combatHeal",
		"combatSacrifice",
		"combatTeleport"
	};

	set(combatCursorNames.at(static_cast<int>(index)));
}

void CursorHandler::set(Cursor::Spellcast index)
{
	//Note: this is animated cursor, ignore requested frame and only change type
	set("castSpell");
}

void CursorHandler::dragAndDropCursor(std::shared_ptr<IImage> image)
{
	dndObject = image;
	cursor->setImage(getCurrentImage(), getPivotOffset());
}

void CursorHandler::dragAndDropCursor (const AnimationPath & path, size_t index)
{
	auto anim = ENGINE->renderHandler().loadAnimation(path, EImageBlitMode::COLORKEY);
	dragAndDropCursor(anim->getImage(index));
}

void CursorHandler::cursorMove(const int & x, const int & y)
{
	pos.x = x;
	pos.y = y;

	cursor->setCursorPosition(pos);
}

Point CursorHandler::getPivotOffset()
{
	if (dndObject)
		return dndObject->dimensions() / 2;

	return cursors.at(currentCursorIndex).pivot;
}

std::shared_ptr<IImage> CursorHandler::getCurrentImage()
{
	if (dndObject)
		return dndObject;

	return cursorImage;
}

void CursorHandler::updateAnimatedCursor()
{
	static const float frameDisplayDuration = 0.1f; // H3 uses 100 ms per frame

	frameTime += ENGINE->framerate().getElapsedMilliseconds() / 1000.f;
	int32_t newFrame = currentFrame;
	const auto & animationName = cursors.at(currentCursorIndex).animation;
	const auto & animation = loadedAnimations.at(animationName);

	while (frameTime >= frameDisplayDuration)
	{
		frameTime -= frameDisplayDuration;
		newFrame++;
	}

	while (newFrame >= animation->size())
		newFrame -= animation->size();

	currentFrame = newFrame;
	cursorImage = animation->getImage(currentFrame);
	cursor->setImage(getCurrentImage(), getPivotOffset());
}

void CursorHandler::render()
{
	if(!showing)
		return;

	cursor->render();
}

void CursorHandler::update()
{
	if(!showing)
		return;

	if (cursors.at(currentCursorIndex).isAnimated)
		updateAnimatedCursor();

	cursor->update();
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

Cursor::ShowType CursorHandler::getShowType() const
{
	return showType;
}

void CursorHandler::changeCursor(Cursor::ShowType newShowType)
{
	if(newShowType == showType)
		return;

	switch(newShowType)
	{
		case Cursor::ShowType::SOFTWARE:
			cursor.reset(new CursorSoftware());
			showType = Cursor::ShowType::SOFTWARE;
			cursor->setImage(getCurrentImage(), getPivotOffset());
			break;
		case Cursor::ShowType::HARDWARE:
			cursor.reset(new CursorHardware());
			showType = Cursor::ShowType::HARDWARE;
			cursor->setImage(getCurrentImage(), getPivotOffset());
			break;
	}
}

void CursorHandler::onScreenResize()
{
	cursor->setImage(getCurrentImage(), getPivotOffset());
}
