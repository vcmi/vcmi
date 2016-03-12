#include "StdInc.h"
#include "CCreatureAnimation.h"

#include "../../lib/vcmi_endian.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CBinaryReader.h"
#include "../../lib/filesystem/CMemoryStream.h"

#include "../gui/SDL_Extensions.h"
#include "../gui/SDL_Pixels.h"

/*
 * CCreatureAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static const SDL_Color creatureBlueBorder = { 0, 255, 255, 255 };
static const SDL_Color creatureGoldBorder = { 255, 255, 0, 255 };
static const SDL_Color creatureNoBorder  =  { 0, 0, 0, 0 };

SDL_Color AnimationControls::getBlueBorder()
{
	return creatureBlueBorder;
}

SDL_Color AnimationControls::getGoldBorder()
{
	return creatureGoldBorder;
}

SDL_Color AnimationControls::getNoBorder()
{
	return creatureNoBorder;
}

CCreatureAnimation * AnimationControls::getAnimation(const CCreature * creature)
{
	auto func = std::bind(&AnimationControls::getCreatureAnimationSpeed, creature, _1, _2);
	return new CCreatureAnimation(creature->animDefName, func);
}

float AnimationControls::getCreatureAnimationSpeed(const CCreature * creature, const CCreatureAnimation * anim, size_t group)
{
	CCreatureAnim::EAnimType type = CCreatureAnim::EAnimType(group);

	assert(creature->animation.walkAnimationTime != 0);
	assert(creature->animation.attackAnimationTime != 0);
	assert(anim->framesInGroup(type) != 0);

	// possible new fields for creature format:
	//split "Attack time" into "Shoot Time" and "Cast Time"

	// a lot of arbitrary multipliers, mostly to make animation speed closer to H3
	const float baseSpeed = 0.1;
	const float speedMult = settings["battle"]["animationSpeed"].Float();
	const float speed = baseSpeed / speedMult;

	switch (type)
	{
	case CCreatureAnim::MOVING:
		return speed * 2 * creature->animation.walkAnimationTime / anim->framesInGroup(type);

	case CCreatureAnim::MOUSEON:
		return baseSpeed;
	case CCreatureAnim::HOLDING:
		return baseSpeed * creature->animation.idleAnimationTime / anim->framesInGroup(type);

	case CCreatureAnim::SHOOT_UP:
	case CCreatureAnim::SHOOT_FRONT:
	case CCreatureAnim::SHOOT_DOWN:
	case CCreatureAnim::CAST_UP:
	case CCreatureAnim::CAST_FRONT:
	case CCreatureAnim::CAST_DOWN:
		return speed * 4 * creature->animation.attackAnimationTime / anim->framesInGroup(type);

	// as strange as it looks like "attackAnimationTime" does not affects melee attacks
	// necessary because length of these animations must be same for all creatures for synchronization
	case CCreatureAnim::ATTACK_UP:
	case CCreatureAnim::ATTACK_FRONT:
	case CCreatureAnim::ATTACK_DOWN:
	case CCreatureAnim::HITTED:
	case CCreatureAnim::DEFENCE:
	case CCreatureAnim::DEATH:
		return speed * 3 / anim->framesInGroup(type);

	case CCreatureAnim::TURN_L:
	case CCreatureAnim::TURN_R:
		return speed / 3;

	case CCreatureAnim::MOVE_START:
	case CCreatureAnim::MOVE_END:
		return speed / 3;

	case CCreatureAnim::DEAD:
		return speed;

	default:
		assert(0);
		return 1;
	}
}

float AnimationControls::getProjectileSpeed()
{
	return settings["battle"]["animationSpeed"].Float() * 100;
}

float AnimationControls::getSpellEffectSpeed()
{
	return settings["battle"]["animationSpeed"].Float() * 60;
}

float AnimationControls::getMovementDuration(const CCreature * creature)
{
	return settings["battle"]["animationSpeed"].Float() * 4 / creature->animation.walkAnimationTime;
}

float AnimationControls::getFlightDistance(const CCreature * creature)
{
	return creature->animation.flightAnimationDistance * 200;
}

CCreatureAnim::EAnimType CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(CCreatureAnim::EAnimType type)
{
	assert(type >= 0);
	assert(framesInGroup(type) != 0);

	this->type = type;
	currentFrame = 0;
	once = false;

	play();
}

CCreatureAnimation::CCreatureAnimation(std::string name, TSpeedController controller)
    : defName(name),
      speed(0.1),
      currentFrame(0),
      elapsedTime(0),
	  type(CCreatureAnim::HOLDING),
	  border(CSDL_Ext::makeColor(0, 0, 0, 0)),
      speedController(controller),
      once(false)
{
	// separate block to avoid accidental use of "data" after it was moved into "pixelData"
	{
		ResourceID resID(std::string("SPRITES/") + name, EResType::ANIMATION);

		auto data = CResourceHandler::get()->load(resID)->readAll();

		pixelData = std::move(data.first);
		pixelDataSize = data.second;
	}

	CBinaryReader reader(new CMemoryStream(pixelData.get(), pixelDataSize));

	reader.readInt32(); // def type, unused

	fullWidth  = reader.readInt32();
	fullHeight = reader.readInt32();

	int totalBlocks = reader.readInt32();

	for (auto & elem : palette)
	{
		elem.r = reader.readUInt8();
		elem.g = reader.readUInt8();
		elem.b = reader.readUInt8();
		elem.a = SDL_ALPHA_OPAQUE;
	}

	for (int i=0; i<totalBlocks; i++)
	{
		int groupID = reader.readInt32();

		int totalInBlock = reader.readInt32();

		reader.skip(4 + 4 + 13 * totalInBlock); // some unused data

		for (int j=0; j<totalInBlock; j++)
			dataOffsets[groupID].push_back(reader.readUInt32());
	}

	// if necessary, add one frame into vcmi-only group DEAD
	if (dataOffsets.count(CCreatureAnim::DEAD) == 0)
		dataOffsets[CCreatureAnim::DEAD].push_back(dataOffsets[CCreatureAnim::DEATH].back());

	play();
}

void CCreatureAnimation::endAnimation()
{
	once = false;
	auto copy = onAnimationReset;
	onAnimationReset.clear();
	copy();
}

bool CCreatureAnimation::incrementFrame(float timePassed)
{
	elapsedTime += timePassed;
	currentFrame += timePassed * speed;
	if (currentFrame >= float(framesInGroup(type)))
	{
		// just in case of extremely low fps (or insanely high speed)
		while (currentFrame >= float(framesInGroup(type)))
			currentFrame -= framesInGroup(type);

		if (once)
			setType(CCreatureAnim::HOLDING);

		endAnimation();
		return true;
	}
	return false;
}

void CCreatureAnimation::setBorderColor(SDL_Color palette)
{
	border = palette;
}

int CCreatureAnimation::getWidth() const
{
	return fullWidth;
}

int CCreatureAnimation::getHeight() const
{
	return fullHeight;
}

float CCreatureAnimation::getCurrentFrame() const
{
	return currentFrame;
}

void CCreatureAnimation::playOnce( CCreatureAnim::EAnimType type )
{
	setType(type);
	once = true;
}

inline int getBorderStrength(float time)
{
	float borderStrength = fabs(vstd::round(time) - time) * 2; // generate value in range 0-1

	return borderStrength * 155 + 100; // scale to 0-255
}

static SDL_Color genShadow(ui8 alpha)
{
	return CSDL_Ext::makeColor(0, 0, 0, alpha);
}

static SDL_Color genBorderColor(ui8 alpha, const SDL_Color & base)
{
	return CSDL_Ext::makeColor(base.r, base.g, base.b, ui8(base.a * alpha / 256));
}

static ui8 mixChannels(ui8 c1, ui8 c2, ui8 a1, ui8 a2)
{
	return c1*a1 / 256 + c2*a2*(255 - a1) / 256 / 256;
}

static SDL_Color addColors(const SDL_Color & base, const SDL_Color & over)
{
	return CSDL_Ext::makeColor(
			mixChannels(over.r, base.r, over.a, base.a),
			mixChannels(over.g, base.g, over.a, base.a),
			mixChannels(over.b, base.b, over.a, base.a),
			ui8(over.a + base.a * (255 - over.a) / 256)
			);
}

std::array<SDL_Color, 8> CCreatureAnimation::genSpecialPalette()
{
	std::array<SDL_Color, 8> ret;

	ret[0] = genShadow(0);
	ret[1] = genShadow(64);
	ret[2] = genShadow(128);
	ret[3] = genShadow(128);
	ret[4] = genShadow(128);
	ret[5] = genBorderColor(getBorderStrength(elapsedTime), border);
	ret[6] = addColors(genShadow(128), genBorderColor(getBorderStrength(elapsedTime), border));
	ret[7] = addColors(genShadow(64),  genBorderColor(getBorderStrength(elapsedTime), border));

	return ret;
}

template<int bpp>
void CCreatureAnimation::nextFrameT(SDL_Surface * dest, bool rotate)
{
	assert(dataOffsets.count(type) && dataOffsets.at(type).size() > size_t(currentFrame));

	ui32 offset = dataOffsets.at(type).at(floor(currentFrame));

	CBinaryReader reader(new CMemoryStream(pixelData.get(), pixelDataSize));

	reader.getStream()->seek(offset);

	reader.readUInt32(); // unused, size of pixel data for this frame
	const ui32 defType2 = reader.readUInt32();
	const ui32 fullWidth = reader.readUInt32();
	/*const ui32 fullHeight =*/ reader.readUInt32();
	const ui32 spriteWidth = reader.readUInt32();
	const ui32 spriteHeight = reader.readUInt32();
	const int leftMargin = reader.readInt32();
	const int topMargin = reader.readInt32();

	const int rightMargin = fullWidth - spriteWidth - leftMargin;
	//const int bottomMargin = fullHeight - spriteHeight - topMargin;

	const size_t baseOffset = reader.getStream()->tell();

	assert(defType2 == 1);
	UNUSED(defType2);

	auto specialPalette = genSpecialPalette();

	for (ui32 i=0; i<spriteHeight; i++)
	{
		//NOTE: if this loop will be optimized to skip empty lines - recheck this read access
		ui8 * lineData = pixelData.get() + baseOffset + reader.readUInt32();

		size_t destX = pos.x;
		if (rotate)
			destX += rightMargin + spriteWidth - 1;
		else
			destX += leftMargin;

		size_t destY = pos.y + topMargin + i;
		size_t currentOffset = 0;
		size_t totalRowLength = 0;

		while (totalRowLength < spriteWidth)
		{
			ui8 type = lineData[currentOffset++];
			ui32 length = lineData[currentOffset++] + 1;

			if (type==0xFF)//Raw data
			{
				for (size_t j=0; j<length; j++)
					putPixelAt<bpp>(dest, destX + (rotate?(-j):(j)), destY, lineData[currentOffset + j], specialPalette);

				currentOffset += length;
			}
			else// RLE
			{
				if (type != 0) // transparency row, handle it here for speed
				{
					for (size_t j=0; j<length; j++)
						putPixelAt<bpp>(dest, destX + (rotate?(-j):(j)), destY, type, specialPalette);
				}
			}

			destX += rotate ? (-length) : (length);
			totalRowLength += length;
		}
	}
}

void CCreatureAnimation::nextFrame(SDL_Surface *dest, bool attacker)
{
	// Note: please notice that attacker value is inversed when passed further.
	// This is intended behavior because "attacker" actually does not needs rotation
	switch(dest->format->BytesPerPixel)
	{
	case 2: return nextFrameT<2>(dest, !attacker);
	case 3: return nextFrameT<3>(dest, !attacker);
	case 4: return nextFrameT<4>(dest, !attacker);
	default:
		logGlobal->errorStream() << (int)dest->format->BitsPerPixel << " bpp is not supported!!!";
	}
}

int CCreatureAnimation::framesInGroup(CCreatureAnim::EAnimType group) const
{
	if(dataOffsets.count(group) == 0)
		return 0;

	return dataOffsets.at(group).size();
}

ui8 * CCreatureAnimation::getPixelAddr(SDL_Surface * dest, int X, int Y) const
{
	return (ui8*)dest->pixels + X * dest->format->BytesPerPixel + Y * dest->pitch;
}

template<int bpp>
inline void CCreatureAnimation::putPixelAt(SDL_Surface * dest, int X, int Y, size_t index, const std::array<SDL_Color, 8> & special) const
{
	if ( X < pos.x + pos.w && Y < pos.y + pos.h && X >= 0 && Y >= 0)
		putPixel<bpp>(getPixelAddr(dest, X, Y), palette[index], index, special);
}

template<int bpp>
inline void CCreatureAnimation::putPixel(ui8 * dest, const SDL_Color & color, size_t index, const std::array<SDL_Color, 8> & special) const
{
	if (index < 8)
	{
		const SDL_Color & pal = special[index];
		ColorPutter<bpp, 0>::PutColor(dest, pal.r, pal.g, pal.b, pal.a);
	}
	else
	{
		ColorPutter<bpp, 0>::PutColor(dest, color.r, color.g, color.b);
	}
}

bool CCreatureAnimation::isDead() const
{
	return getType() == CCreatureAnim::DEAD
	    || getType() == CCreatureAnim::DEATH;
}

bool CCreatureAnimation::isIdle() const
{
	return getType() == CCreatureAnim::HOLDING
	    || getType() == CCreatureAnim::MOUSEON;
}

bool CCreatureAnimation::isMoving() const
{
	return getType() == CCreatureAnim::MOVE_START
	    || getType() == CCreatureAnim::MOVING
	    || getType() == CCreatureAnim::MOVE_END;
}

bool CCreatureAnimation::isShooting() const
{
	return getType() == CCreatureAnim::SHOOT_UP
	    || getType() == CCreatureAnim::SHOOT_FRONT
	    || getType() == CCreatureAnim::SHOOT_DOWN;
}

void CCreatureAnimation::pause()
{
	speed = 0;
}

void CCreatureAnimation::play()
{
    speed = 0;
    if (speedController(this, type) != 0)
        speed = 1 / speedController(this, type);
}
