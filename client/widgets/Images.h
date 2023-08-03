/*
 * Images.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"
#include "../battle/BattleConstants.h"

VCMI_LIB_NAMESPACE_BEGIN
class Rect;
VCMI_LIB_NAMESPACE_END

struct SDL_Color;
class CAnimImage;
class CLabel;
class CAnimation;
class IImage;

// Image class
class CPicture : public CIntObject
{
	std::shared_ptr<IImage> bg;

public:
	/// if set, only specified section of internal image will be rendered
	std::optional<Rect> srcRect;

	/// If set to true, iamge will be redrawn on each frame
	bool needRefresh;

	/// If set to false, image will not be rendered
	/// Deprecated, use CIntObject::disable()/enable() instead
	bool visible;

	std::shared_ptr<IImage> getSurface()
	{
		return bg;
	}

	/// wrap existing image
	CPicture(std::shared_ptr<IImage> image, const Point & position);

	/// wrap section of an existing Image
	CPicture(std::shared_ptr<IImage> image, const Rect &SrcRext, int x = 0, int y = 0); //wrap subrect of given surface

	/// Loads image from specified file name
	CPicture(const std::string & bmpname);
	CPicture(const std::string & bmpname, const Point & position);
	CPicture(const std::string & bmpname, int x, int y);

	/// set alpha value for whole surface. Note: may be messed up if surface is shared
	/// 0=transparent, 255=opaque
	void setAlpha(int value);
	void scaleTo(Point size);
	void colorize(PlayerColor player);

	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
};

/// area filled with specific texture
class CFilledTexture : public CIntObject
{
protected:
	std::shared_ptr<IImage> texture;
	Rect imageArea;

public:
	CFilledTexture(std::string imageName, Rect position);
	CFilledTexture(std::shared_ptr<IImage> image, Rect position, Rect imageArea);

	void showAll(Canvas & to) override;
};

class FilledTexturePlayerColored : public CFilledTexture
{
public:
	FilledTexturePlayerColored(std::string imageName, Rect position);

	void playerColored(PlayerColor player);
};

/// Class for displaying one image from animation
class CAnimImage: public CIntObject
{
private:
	std::shared_ptr<CAnimation> anim;
	//displayed frame/group
	size_t frame;
	size_t group;
	ui8 flags;
	const Point scaledSize;

	/// If set, then image is colored using player-specific palette
	std::optional<PlayerColor> player;

	bool isScaled() const;
	void setSizeFromImage(const IImage &img);
	void init();
public:
	bool visible;

	CAnimImage(const std::string & name, size_t Frame, size_t Group=0, int x=0, int y=0, ui8 Flags=0);
	CAnimImage(std::shared_ptr<CAnimation> Anim, size_t Frame, size_t Group=0, int x=0, int y=0, ui8 Flags=0);
	CAnimImage(std::shared_ptr<CAnimation> Anim, size_t Frame, Rect targetPos, size_t Group=0, ui8 Flags=0);
	~CAnimImage();

	/// size of animation
	size_t size();

	/// change displayed frame on this one
	void setFrame(size_t Frame, size_t Group=0);

	/// makes image player-colored to specific player
	void playerColored(PlayerColor player);

	/// returns true if image has player-colored effect applied
	bool isPlayerColored() const;

	void showAll(Canvas & to) override;
};

/// Base class for displaying animation, used as superclass for different animations
class CShowableAnim: public CIntObject
{
public:
	enum EFlags
	{
		BASE=1,            //base frame will be blitted before current one
		HORIZONTAL_FLIP=2, //TODO: will be displayed rotated
		VERTICAL_FLIP=4,   //TODO: will be displayed rotated
		PLAY_ONCE=32       //play animation only once and stop at last frame
	};
protected:
	std::shared_ptr<CAnimation> anim;

	size_t group, frame;//current frame

	size_t first, last; //animation range

	/// total time on scren for each frame in animation
	ui32 frameTimeTotal;

	/// how long was current frame visible on screen
	ui32 frameTimePassed;

	ui8 flags;//Flags from EFlags enum

	//blit image with optional rotation, fitting into rect, etc
	void blitImage(size_t frame, size_t group, Canvas & to);

	//For clipping in rect, offsets of picture coordinates
	int xOffset, yOffset;

	ui8 alpha;

public:
	//called when next animation sequence is required
	std::function<void()> callback;

	//Set per-surface alpha, 0 = transparent, 255 = opaque
	void setAlpha(ui32 alphaValue);

	CShowableAnim(int x, int y, std::string name, ui8 flags, ui32 frameTime, size_t Group=0, uint8_t alpha = UINT8_MAX);
	~CShowableAnim();

	//set animation to group or part of group
	bool set(size_t Group);
	bool set(size_t Group, size_t from, size_t to=-1);

	//set rotation flags
	void rotate(bool on, bool vertical=false);

	//move displayed part of picture (if picture is clipped to rect)
	void clipRect(int posX, int posY, int width, int height);

	//set frame to first, call callback
	virtual void reset();

	//set animation duration
	void setDuration(int durationMs);

	//show current frame and increase counter
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	void tick(uint32_t msPassed) override;
};

/// Creature-dependend animations like attacking, moving,...
class CCreatureAnim: public CShowableAnim
{
private:
	//queue of animations waiting to be displayed
	std::queue<ECreatureAnimType> queue;

	//this function is used as callback if preview flag was set during construction
	void loopPreview(bool warMachine);

public:
	//change anim to next if queue is not empty, call callback othervice
	void reset() override;

	//add sequence to the end of queue
	void addLast(ECreatureAnimType newType);

	void startPreview(bool warMachine);

	//clear queue and set animation to this sequence
	void clearAndSet(ECreatureAnimType type);

	CCreatureAnim(int x, int y, std::string name, ui8 flags = 0, ECreatureAnimType = ECreatureAnimType::HOLDING);

};
