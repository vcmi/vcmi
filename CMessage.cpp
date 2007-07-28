#include "stdafx.h"
#include "CMessage.h"
#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
#include "CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#define CGI (CGameInfo::mainObj)

SDL_Color tytulowy, tlo, zwykly ;
SDL_Rect genRect(int hh, int ww, int xx, int yy);

extern SDL_Surface * ekran;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Color genRGB(int r, int g, int b, int a=0);
//void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2);
extern CPreGame * CPG;
bool isItIn(const SDL_Rect * rect, int x, int y);


CMessage::CMessage()
{
	piecesOfBox = CGI->spriteh->giveDef("DIALGBOX.DEF");
	background = CGI->bitmaph->loadBitmap("DIBOXBCK.BMP");
	SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
}
CMessage::~CMessage()
{
	delete piecesOfBox;
	SDL_FreeSurface(background);
}
SDL_Surface * CMessage::drawBox1(int w, int h, int playerColor)
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(ekran->flags, w, h, ekran->format->BitsPerPixel, ekran->format->Rmask, ekran->format->Gmask, ekran->format->Bmask, ekran->format->Amask);
	for (int i=0; i<h; i+=background->h)//background
	{
		for (int j=0; j<w; j+=background->w-1)
			SDL_BlitSurface(background,&genRect(background->h,background->w-1,1,0),ret,&genRect(h,w,j,i));
	}
	//SDL_Flip(ekran);
	//CSDL_Ext::update(ekran);
	
	std::vector<SDL_Surface*> pieces;
	for (int i=0;i<piecesOfBox->ourImages.size();i++)
	{
		pieces.push_back(piecesOfBox->ourImages[i].bitmap);
		if (playerColor!=1)
		{
			CSDL_Ext::blueToPlayersAdv(pieces[pieces.size()-1],playerColor);
		}
	}
	//obwodka I-szego rzedu pozioma
	for (int i=0; i<w; i+=pieces[6]->w)
	{
		SDL_BlitSurface
			(pieces[6],NULL,ret,&genRect(pieces[6]->h,pieces[6]->w,i,0));
		SDL_BlitSurface
			(pieces[7],NULL,ret,&genRect(pieces[7]->h,pieces[7]->w,i,h-pieces[7]->h));
	}
	//obwodka I-szego rzedu pionowa
	for (int i=0; i<h; i+=piecesOfBox->ourImages[4].bitmap->h)
	{
		SDL_BlitSurface
			(pieces[4],NULL,ret,&genRect(pieces[4]->h,pieces[4]->w,0,i));
		SDL_BlitSurface
			(pieces[5],NULL,ret,&genRect(pieces[5]->h,pieces[5]->w,w-pieces[5]->w,i));
	}
	//corners
	SDL_BlitSurface
		(pieces[0],NULL,ret,&genRect(pieces[0]->h,pieces[0]->w,0,0));
	SDL_BlitSurface
		(pieces[1],NULL,ret,&genRect(pieces[1]->h,pieces[1]->w,w-pieces[1]->w,0));
	SDL_BlitSurface
		(pieces[2],NULL,ret,&genRect(pieces[2]->h,pieces[2]->w,0,h-pieces[2]->h));
	SDL_BlitSurface
		(pieces[3],NULL,ret,&genRect(pieces[3]->h,pieces[3]->w,w-pieces[3]->w,h-pieces[3]->h));
	//box gotowy!
	return ret;
}

std::vector<std::string> * CMessage::breakText(std::string text, int line, bool userBreak)
{
	std::vector<std::string> * ret = new std::vector<std::string>();
	while (text.length()>line)
	{
		int whereCut = -1;
		bool pom = true;
		for (int i=0; i<line; i++)
		{
			if (text[i]==10) //end of line sign
			{
				whereCut=i+1;
				pom=false;
				break;
			}
		}
		for (int i=line; i>0&&pom; i--)
		{
			if (text[i]==' ')
			{
				whereCut = i;
				break;
			}
		}
		ret->push_back(text.substr(0,whereCut));
		text.erase(0,whereCut);
	}
	for (int i=0;i<text.length();i++)
	{		
		if (text[i]==10) //end of line sign
		{
			ret->push_back(text.substr(0,i));
			text.erase(0,i);
			i=0;
		}
	}
	if (text.length() > 0)
		ret->push_back(text);
	return ret;
}
SDL_Surface * CMessage::genMessage
(std::string title, std::string text, EWindowType type, std::vector<CDefHandler*> *addPics, void * cb)
{
//max x 320 okolo 30 znakow
	std::vector<std::string> * tekst;
	if (text.length() < 30) //nie trzeba polamac
	{
		tekst = new std::vector<std::string>();
		tekst->push_back(text);
	}
	else tekst = breakText(text);
	int ww, hh; //wymiary boksa
	if (319>30+13*text.length())
		ww = 30+13*text.length();
	else ww = 319;
	if (title.length())
		hh=110+(21*tekst->size());
	else hh=60+(21*tekst->size());
	if (type==EWindowType::yesOrNO) //make place for buttons
	{
		if (ww<200) ww=200; 
		hh+=70;
	}

	SDL_Surface * ret = drawBox1(ww,hh,0);
	//prepare title text
	
	if (title.length())
	{
		//SDL_Surface * titleText = TTF_RenderText_Shaded(TNRB16,title.c_str(),tytulowy,tlo);	
		SDL_Surface * titleText = TTF_RenderText_Blended(TNRB16,title.c_str(),tytulowy);	

		//draw title
		SDL_Rect tytul = genRect(titleText->h,titleText->w,((ret->w/2)-(titleText->w/2)),37);
		SDL_BlitSurface(titleText,NULL,ret,&tytul);
		SDL_FreeSurface(titleText);
	}
	//draw text
	for (int i=0; i<tekst->size(); i++) 
	{
		int by = 37+i*21;
		if (title.length()) by+=40;
		//SDL_Surface * tresc = TTF_RenderText_Shaded(TNRB16,(*tekst)[i].c_str(),zwykly,tlo);
		SDL_Surface * tresc = TTF_RenderText_Blended(TNRB16,(*tekst)[i].c_str(),zwykly);
		SDL_Rect trescRect = genRect(tresc->h,tresc->w,((ret->w/2)-(tresc->w/2)),by);
		SDL_BlitSurface(tresc,NULL,ret,&trescRect);
		SDL_FreeSurface(tresc);
	}
	if (type==EWindowType::yesOrNO) // add buttons
	{
		int by = 77+tekst->size()*21;
		if (title.length()) by+=40;
		int hwo = (*addPics)[0]->ourImages[0].bitmap->w, hwc=(*addPics)[0]->ourImages[0].bitmap->w;
		//ok
		SDL_Rect trescRect = genRect((*addPics)[0]->ourImages[0].bitmap->h,hwo,((ret->w/2)-hwo-10),by);
		SDL_BlitSurface((*addPics)[0]->ourImages[0].bitmap,NULL,ret,&trescRect);
		((std::vector<SDL_Rect>*)(cb))->push_back(trescRect);
		//cancel
		trescRect = genRect((*addPics)[1]->ourImages[0].bitmap->h,hwc,((ret->w/2)+10),by);
		SDL_BlitSurface((*addPics)[1]->ourImages[0].bitmap,NULL,ret,&trescRect);
		((std::vector<SDL_Rect>*)(cb))->push_back(trescRect);
	}
	delete tekst;
	return ret;
}