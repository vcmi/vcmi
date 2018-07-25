/*
 * Images.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Images.h"

#include "MiscWidgets.h"

#include "../gui/CAnimation.h"
#include "../gui/SDL_Pixels.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CCursorHandler.h"

#include "../battle/CBattleInterface.h"
#include "../battle/CBattleInterfaceClasses.h"

#include "../CBitmapHandler.h"
#include "../Graphics.h"
#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../CMessage.h"
#include "../CMusicHandler.h"
#include "../windows/CAdvmapInterface.h"

#include "../../CCallback.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/CGeneralTextHandler.h" //for Unicode related stuff
#include "../../lib/CRandomGenerator.h"

CPicture::CPicture( SDL_Surface *BG, int x, int y, bool Free )
{
	init();
	bg = BG;
	freeSurf = Free;
	pos.x += x;
	pos.y += y;
	pos.w = BG->w;
	pos.h = BG->h;
}

CPicture::CPicture( const std::string &bmpname, int x, int y )
{
	init();
	bg = BitmapHandler::loadBitmap(bmpname);
	freeSurf = true;
	pos.x += x;
	pos.y += y;
	if(bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
	else
	{
		pos.w = pos.h = 0;
	}
}

CPicture::CPicture(const Rect &r, const SDL_Color &color, bool screenFormat)
{
	init();
	createSimpleRect(r, screenFormat, SDL_MapRGB(bg->format, color.r, color.g,color.b));
}

CPicture::CPicture(const Rect &r, ui32 color, bool screenFormat)
{
	init();
	createSimpleRect(r, screenFormat, color);
}

CPicture::CPicture(SDL_Surface * BG, const Rect &SrcRect, int x, int y, bool free)
{
	visible = true;
	needRefresh = false;
	srcRect = new Rect(SrcRect);
	pos.x += x;
	pos.y += y;
	pos.w = srcRect->w;
	pos.h = srcRect->h;
	bg = BG;
	freeSurf = free;
}

void CPicture::setSurface(SDL_Surface *to)
{
	bg = to;
	if (srcRect)
	{
		pos.w = srcRect->w;
		pos.h = srcRect->h;
	}
	else
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
}

CPicture::~CPicture()
{
	if(freeSurf)
		SDL_FreeSurface(bg);
	delete srcRect;
}

void CPicture::init()
{
	visible = true;
	needRefresh = false;
	srcRect = nullptr;
}

void CPicture::show(SDL_Surface * to)
{
	if (visible && needRefresh)
		showAll(to);
}

void CPicture::showAll(SDL_Surface * to)
{
	if(bg && visible)
	{
		if(srcRect)
		{
			SDL_Rect srcRectCpy = *srcRect;
			SDL_Rect dstRect = srcRectCpy;
			dstRect.x = pos.x;
			dstRect.y = pos.y;

			CSDL_Ext::blitSurface(bg, &srcRectCpy, to, &dstRect);
		}
		else
			blitAt(bg, pos, to);
	}
}

void CPicture::convertToScreenBPP()
{
	SDL_Surface *hlp = bg;
	bg = SDL_ConvertSurface(hlp,screen->format,0);
	CSDL_Ext::setDefaultColorKey(bg);
	SDL_FreeSurface(hlp);
}

void CPicture::setAlpha(int value)
{
	CSDL_Ext::setAlpha (bg, value);
}

void CPicture::scaleTo(Point size)
{
	SDL_Surface * scaled = CSDL_Ext::scaleSurface(bg, size.x, size.y);

	if(freeSurf)
		SDL_FreeSurface(bg);

	setSurface(scaled);
	freeSurf = false;
}

void CPicture::createSimpleRect(const Rect &r, bool screenFormat, ui32 color)
{
	pos += r;
	pos.w = r.w;
	pos.h = r.h;
	if(screenFormat)
		bg = CSDL_Ext::newSurface(r.w, r.h);
	else
		bg = SDL_CreateRGBSurface(0, r.w, r.h, 8, 0, 0, 0, 0);

	SDL_FillRect(bg, nullptr, color);
	freeSurf = true;
}

void CPicture::colorize(PlayerColor player)
{
	assert(bg);
	graphics->blueToPlayersAdv(bg, player);
}

CFilledTexture::CFilledTexture(std::string imageName, Rect position):
    CIntObject(0, position.topLeft()),
    texture(BitmapHandler::loadBitmap(imageName))
{
	pos.w = position.w;
	pos.h = position.h;
}

CFilledTexture::~CFilledTexture()
{
	SDL_FreeSurface(texture);
}

void CFilledTexture::showAll(SDL_Surface *to)
{
	CSDL_Ext::CClipRectGuard guard(to, pos);
	CSDL_Ext::fillTexture(to, texture);
}

CAnimImage::CAnimImage(const std::string & name, size_t Frame, size_t Group, int x, int y, ui8 Flags):
	frame(Frame),
	group(Group),
	player(-1),
	flags(Flags)
{
	pos.x += x;
	pos.y += y;
	anim = std::make_shared<CAnimation>(name);
	init();
}

CAnimImage::CAnimImage(std::shared_ptr<CAnimation> Anim, size_t Frame, size_t Group, int x, int y, ui8 Flags):
	anim(Anim),
	frame(Frame),
	group(Group),
	player(-1),
	flags(Flags)
{
	pos.x += x;
	pos.y += y;
	init();
}

size_t CAnimImage::size()
{
	return anim->size(group);
}

void CAnimImage::init()
{
	visible = true;
	anim->load(frame, group);
	if (flags & CShowableAnim::BASE)
		anim->load(0,group);

	auto img = anim->getImage(frame, group);
	if (img)
	{
		pos.w = img->width();
		pos.h = img->height();
	}
}

CAnimImage::~CAnimImage()
{
}

void CAnimImage::showAll(SDL_Surface * to)
{
	if(!visible)
		return;

	if(flags & CShowableAnim::BASE && frame != 0)
		if(auto img = anim->getImage(0, group))
			img->draw(to, pos.x, pos.y);

	if(auto img = anim->getImage(frame, group))
		img->draw(to, pos.x, pos.y);
}

void CAnimImage::setFrame(size_t Frame, size_t Group)
{
	if (frame == Frame && group==Group)
		return;
	if (anim->size(Group) > Frame)
	{
		anim->load(Frame, Group);
		frame = Frame;
		group = Group;
		if(auto img = anim->getImage(frame, group))
		{
			if (flags & CShowableAnim::PLAYER_COLORED)
				img->playerColored(player);
			pos.w = img->width();
			pos.h = img->height();
		}
	}
	else
		logGlobal->error("Error: accessing unavailable frame %d:%d in CAnimation!", Group, Frame);
}

void CAnimImage::playerColored(PlayerColor currPlayer)
{
	player = currPlayer;
	flags |= CShowableAnim::PLAYER_COLORED;
	anim->getImage(frame, group)->playerColored(player);
	if (flags & CShowableAnim::BASE)
			anim->getImage(0, group)->playerColored(player);
}

CShowableAnim::CShowableAnim(int x, int y, std::string name, ui8 Flags, ui32 Delay, size_t Group):
	anim(std::make_shared<CAnimation>(name)),
	group(Group),
	frame(0),
	first(0),
	frameDelay(Delay),
	value(0),
	flags(Flags),
	xOffset(0),
	yOffset(0),
	alpha(255)
{
	anim->loadGroup(group);
	last = anim->size(group);

	pos.w = anim->getImage(0, group)->width();
	pos.h = anim->getImage(0, group)->height();
	pos.x+= x;
	pos.y+= y;
}

CShowableAnim::~CShowableAnim()
{
	anim->unloadGroup(group);
}

void CShowableAnim::setAlpha(ui32 alphaValue)
{
	alpha = std::min<ui32>(alphaValue, 255);
}

bool CShowableAnim::set(size_t Group, size_t from, size_t to)
{
	size_t max = anim->size(Group);

	if (to < max)
		max = to;

	if (max < from || max == 0)
		return false;

	anim->unloadGroup(group);
	anim->loadGroup(Group);

	group = Group;
	frame = first = from;
	last = max;
	value = 0;
	return true;
}

bool CShowableAnim::set(size_t Group)
{
	if (anim->size(Group)== 0)
		return false;
	if (group != Group)
	{
		anim->unloadGroup(group);
		anim->loadGroup(Group);

		first = 0;
		group = Group;
		last = anim->size(Group);
	}
	frame = value = 0;
	return true;
}

void CShowableAnim::reset()
{
	value = 0;
	frame = first;

	if (callback)
		callback();
}

void CShowableAnim::clipRect(int posX, int posY, int width, int height)
{
	xOffset = posX;
	yOffset = posY;
	pos.w = width;
	pos.h = height;
}

void CShowableAnim::show(SDL_Surface * to)
{
	if ( flags & BASE )// && frame != first) // FIXME: results in graphical glytch in Fortress, upgraded hydra's dwelling
		blitImage(first, group, to);
	blitImage(frame, group, to);

	if ((flags & PLAY_ONCE) && frame + 1 == last)
		return;

	if ( ++value == frameDelay )
	{
		value = 0;
		if ( ++frame >= last)
			reset();
	}
}

void CShowableAnim::showAll(SDL_Surface * to)
{
	if ( flags & BASE )// && frame != first)
		blitImage(first, group, to);
	blitImage(frame, group, to);
}

void CShowableAnim::blitImage(size_t frame, size_t group, SDL_Surface *to)
{
	assert(to);
	Rect src( xOffset, yOffset, pos.w, pos.h);
	auto img = anim->getImage(frame, group);
	if(img)
		img->draw(to, pos.x, pos.y, &src, alpha);
}

void CShowableAnim::rotate(bool on, bool vertical)
{
	ui8 flag = vertical? VERTICAL_FLIP:HORIZONTAL_FLIP;
	if (on)
		flags |= flag;
	else
		flags &= ~flag;
}

CCreatureAnim::CCreatureAnim(int x, int y, std::string name, ui8 flags, EAnimType type):
	CShowableAnim(x,y,name,flags,4,type)
{
	xOffset = 0;
	yOffset = 0;
}

void CCreatureAnim::loopPreview(bool warMachine)
{
	std::vector<EAnimType> available;

	static const EAnimType creaPreviewList[] = {HOLDING, HITTED, DEFENCE, ATTACK_FRONT, CAST_FRONT};
	static const EAnimType machPreviewList[] = {HOLDING, MOVING, SHOOT_UP, SHOOT_FRONT, SHOOT_DOWN};
	auto & previewList = warMachine ? machPreviewList : creaPreviewList;

	for (auto & elem : previewList)
		if (anim->size(elem))
			available.push_back(elem);

	size_t rnd = CRandomGenerator::getDefault().nextInt(available.size() * 2 - 1);

	if (rnd >= available.size())
	{
		EAnimType type;
		if ( anim->size(MOVING) == 0 )//no moving animation present
			type = HOLDING;
		else
			type = MOVING;

		//display this anim for ~1 second (time is random, but it looks good)
		for (size_t i=0; i< 12/anim->size(type) + 1; i++)
			addLast(type);
	}
	else
		addLast(available[rnd]);
}

void CCreatureAnim::addLast(EAnimType newType)
{
	if (type != MOVING && newType == MOVING)//starting moving - play init sequence
	{
		queue.push( MOVE_START );
	}
	else if (type == MOVING && newType != MOVING )//previous anim was moving - finish it
	{
		queue.push( MOVE_END );
	}
	if (newType == TURN_L || newType == TURN_R)
		queue.push(newType);

	queue.push(newType);
}

void CCreatureAnim::reset()
{
	//if we are in the middle of rotation - set flag
	if (type == TURN_L && !queue.empty() && queue.front() == TURN_L)
		rotate(true);
	if (type == TURN_R && !queue.empty() && queue.front() == TURN_R)
		rotate(false);

	while (!queue.empty())
	{
		EAnimType at = queue.front();
		queue.pop();
		if (set(at))
			return;
	}
	if  (callback)
		callback();
	while (!queue.empty())
	{
		EAnimType at = queue.front();
		queue.pop();
		if (set(at))
			return;
	}
	set(HOLDING);
}

void CCreatureAnim::startPreview(bool warMachine)
{
	callback = std::bind(&CCreatureAnim::loopPreview, this, warMachine);
}

void CCreatureAnim::clearAndSet(EAnimType type)
{
	while (!queue.empty())
		queue.pop();
	set(type);
}
