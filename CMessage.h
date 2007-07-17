#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
#include "CDefHandler.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
class MapSel;
typedef void(CPreGame::*ttt)();
template <class T=ttt> class CGroup;
template <class T=ttt> class CPoinGroup ;
struct OverButton
{	
	int ID;
	int type;
	SDL_Rect pos;
	CDefHandler* imgs;
	int state;
	virtual void show() ;
	virtual void press(bool down=true);
};
template <class T=ttt> struct Button: public OverButton
{
	T fun;
	CGroup<T> * ourGroup;
	Button( SDL_Rect Pos, T Fun,CDefHandler* Imgs, bool Sel=false, CGroup<T>* gr=NULL, int id=-1)
		:fun(Fun),ourGroup(gr){type=0;imgs=Imgs;selectable=Sel;selected=false;state=0;pos=Pos;ID=id;};
	Button(){ourGroup=NULL;};
	bool selectable, selected;
	bool highlightable, highlighted;
	virtual	void hover(bool on=true);
	virtual void select(bool on=true);
};	
template <class T=ttt> struct SetrButton: public Button<T>
{
	int key, * poin;
	virtual void press(bool down=true);
	SetrButton(){type=0;selectable=selected=false;state=0;}
};
template<class T=CPreGame>  class Slider
{ //
public:
	bool vertical; // false means horizontal
	SDL_Rect pos; // position
	Button<void(Slider::*)()> up, down, //or left/right
		slider; 
	int positionsAmnt, capacity;// capacity - amount of positions dispplayed at once
	int whereAreWe; // first displayed thing
	bool moving;
	void(T::*fun)(int);
	void clickDown(int x, int y, bool bzgl=true);
	void clickUp(int x, int y, bool bzgl=true);
	void mMove(int x, int y, bool bzgl=true);
	void moveUp();
	void moveDown();
	void activate();
	Slider(int x, int y, int h, int amnt, int cap, bool ver);
	void updateSlid();
	void handleIt(SDL_Event sev);

};
//template<class T=void(CPreGame::*)(int)>
template<class T=ttt>  struct IntBut: public Button<T>
{
public:
	int key;
	int * what;
	IntBut(){type=2;fun=NULL;};
	IntBut( SDL_Rect Pos, T Fun,CDefHandler* Imgs, bool Sel, int Key, int * What)
		: Button(Pos,Fun,Imgs,Sel,gr),key(My),key(Key),what(What){ourGroup=gr;type=2;fun=NULL;};
	void set(){*what=key;};
};
template<class T=ttt>  struct IntSelBut: public Button<T>
{
public:
	CPoinGroup<T> * ourGroup;
	int key;
	IntSelBut(){};
	IntSelBut( SDL_Rect Pos, T Fun,CDefHandler* Imgs, bool Sel=false, CPoinGroup<T>* gr=NULL, int My=-1)
		: Button(Pos,Fun,Imgs,Sel,gr),key(My){ourGroup=gr;type=1;};
	void select(bool on=true) {(*this).Button::select(on);ourGroup->setYour(this);}
};
template <class T=ttt> class CPoinGroup :public CGroup<T>
{
public:
	int * gdzie; //where (po polsku, bo by by³o s³owo kluczowe :/)
	void setYour(IntSelBut<T> * your){*gdzie=your->key;};
};
template <class T=ttt> class CGroup
{
public:
	Button<T> * selected;
	int type; // 1=sinsel
	CGroup():selected(NULL),type(0){};
};
class CMessage
{
public:
	static std::vector<std::string> * breakText(std::string text, int line=30);
	CDefHandler * piecesOfBox;
	SDL_Surface * background;
	SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly, 
								std::vector<CDefHandler*> *addPics=NULL, void * cb=NULL);
	SDL_Surface * drawBox1(int w, int h);
	CMessage();
};

//

#endif //CMESSAGE_H