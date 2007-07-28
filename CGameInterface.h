#include "SDL.h"
#include "CDefHandler.h"
#include "SDL_Extensions.h"
class CGameInterface
{
};
class CAdvMapInt : public CGameInterface //adventure map interface
{
	SDL_Surface * bg;
};
class CAICallback : public CGameInterface // callback for AI
{
};
class CIntObject //interface object
{
public:
	SDL_Rect pos;
	int ID;
};
class CButtonBase : public CIntObject
{
public:
	int type;
	bool abs;
	struct Offset
	{
		int x, y;
	}  *offset;
	CIntObject * ourObj;
	int state;
	std::vector<SDL_Surface*> imgs;
	virtual void show() ;
	CButtonBase(){abs=true;ourObj=NULL;}
};
class ClickableL : public virtual CButtonBase //for left-clicks
{
	bool pressed;
	virtual void press (bool down)=0;
};
class ClickableR : public virtual CButtonBase //for right-clicks
{
	bool pressed;
	virtual void click (bool down)=0;
};
class Hoverable : public virtual CButtonBase 
{
	bool hovered;
	virtual void hover (bool on)=0;
};
class KeyInterested : public virtual CButtonBase 
{
	virtual void keyPressed (SDL_KeyboardEvent & key)=0;
};
class CPlayerInterface
{
	static CGameInterface * gamein;
	std::vector<ClickableL*> lclickable;
	std::vector<ClickableR*> rclickable;
	std::vector<Hoverable*> hoverable;
	std::vector<KeyInterested*> keyinterested;
	void handleEvent(SDL_Event * sEvent);
};