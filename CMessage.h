#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
typedef void(CPreGame::*ttt)();
template <class T=ttt> class CGroup;
template <class T=ttt> class CPoinGroup ;
template <class T=ttt> struct Button
{
	int type;
	SDL_Rect pos;
	T fun;
	CSemiDefHandler* imgs;
	Button( SDL_Rect Pos, T Fun,CSemiDefHandler* Imgs, bool Sel=false, CGroup<T>* gr=NULL, int id=-1)
		:state(0),selectable(Sel),selected(false),imgs(Imgs),pos(Pos),fun(Fun),ourGroup(gr), type(0), ID(id){};
	Button(){};
	bool selectable, selected;
	bool highlightable, highlighted;
	int state;
	int ID;
	virtual	void hover(bool on=true);
	virtual void press(bool down=true);
	virtual void select(bool on=true);
	CGroup<T> * ourGroup;
};	
//template<class T=void(CPreGame::*)(int)>
template<class T=ttt>  struct IntSelBut: public Button<T>
{
public:
	CPoinGroup<T> * ourGroup;
	int key;
	IntSelBut(){};
	IntSelBut( SDL_Rect Pos, T Fun,CSemiDefHandler* Imgs, bool Sel=false, CPoinGroup<T>* gr=NULL, int My=-1)
		: Button(Pos,Fun,Imgs,Sel,gr),key(My){ourGroup=gr;};
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
	std::vector<std::string> * breakText(std::string text);
	CSemiDefHandler * piecesOfBox;
	SDL_Surface * background;
	SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly, 
								std::vector<CSemiDefHandler*> *addPics=NULL, void * cb=NULL);
	SDL_Surface * drawBox1(int w, int h);
	CMessage();
};

//

#endif //CMESSAGE_H