#include "stdafx.h"
#include "CMessage.h"
#include "SDL_TTF.h"
#include "CSemiDefHandler.h"

SDL_Color tytulowy, tlo, zwykly ;
SDL_Rect genRect(int hh, int ww, int xx, int yy);

extern SDL_Surface * ekran;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Color genRGB(int r, int g, int b, int a=0);
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran);
void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran);
extern CPreGame * CPG;
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran);
bool isItIn(const SDL_Rect * rect, int x, int y);
CMessage::CMessage()
{
	piecesOfBox = new CSemiDefHandler();
	piecesOfBox->openDef("DIALGBOX.DEF","H3sprite.lod");
	background = SDL_LoadBMP("H3bitmap.lod\\DIBOXBCK.BMP");
	SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
}
SDL_Surface * CMessage::drawBox1(int w, int h)
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(ekran->flags, w, h, ekran->format->BitsPerPixel, ekran->format->Rmask, ekran->format->Gmask, ekran->format->Bmask, ekran->format->Amask);
	for (int i=0; i<h; i+=background->h)//background
	{
		for (int j=0; j<w; j+=background->w-1)
			SDL_BlitSurface(background,&genRect(background->h,background->w-1,1,0),ret,&genRect(h,w,j,i));
	}SDL_Flip(ekran);
	//obwodka I-szego rzedu pozioma
	for (int i=0; i<w; i+=piecesOfBox->ourImages[6].bitmap->w)
	{
		SDL_BlitSurface
			(piecesOfBox->ourImages[6].bitmap,NULL,
			ret,&genRect(piecesOfBox->ourImages[6].bitmap->h,piecesOfBox->ourImages[6].bitmap->w,i,0));
		SDL_BlitSurface
			(piecesOfBox->ourImages[7].bitmap,NULL,
			ret,&genRect(piecesOfBox->ourImages[7].bitmap->h,piecesOfBox->ourImages[7].bitmap->w,i,h-piecesOfBox->ourImages[7].bitmap->h));
	}
	//obwodka I-szego rzedu pionowa
	for (int i=0; i<h; i+=piecesOfBox->ourImages[4].bitmap->h)
	{
		SDL_BlitSurface
			(piecesOfBox->ourImages[4].bitmap,NULL,
			ret,&genRect(piecesOfBox->ourImages[4].bitmap->h,piecesOfBox->ourImages[4].bitmap->w,0,i));
		SDL_BlitSurface
			(piecesOfBox->ourImages[5].bitmap,NULL,
			ret,&genRect(piecesOfBox->ourImages[5].bitmap->h,piecesOfBox->ourImages[5].bitmap->w,w-piecesOfBox->ourImages[5].bitmap->w,i));
	}
	//corners
	SDL_BlitSurface
		(piecesOfBox->ourImages[0].bitmap,NULL,
		ret,&genRect(piecesOfBox->ourImages[0].bitmap->h,piecesOfBox->ourImages[0].bitmap->w,0,0));
	SDL_BlitSurface
		(piecesOfBox->ourImages[1].bitmap,NULL,
		ret,&genRect(piecesOfBox->ourImages[1].bitmap->h,piecesOfBox->ourImages[1].bitmap->w,w-piecesOfBox->ourImages[1].bitmap->w,0));
	SDL_BlitSurface
		(piecesOfBox->ourImages[2].bitmap,NULL,
		ret,&genRect(piecesOfBox->ourImages[2].bitmap->h,piecesOfBox->ourImages[2].bitmap->w,0,h-piecesOfBox->ourImages[2].bitmap->h));
	SDL_BlitSurface
		(piecesOfBox->ourImages[3].bitmap,NULL,
		ret,&genRect(piecesOfBox->ourImages[3].bitmap->h,piecesOfBox->ourImages[3].bitmap->w,w-piecesOfBox->ourImages[3].bitmap->w,h-piecesOfBox->ourImages[3].bitmap->h));
	//box gotowy!

	return ret;
}

std::vector<std::string> * CMessage::breakText(std::string text)
{
	std::vector<std::string> * ret = new std::vector<std::string>();
	while (text.length()>30)
	{
		int whereCut = -1;
		for (int i=30; i>0; i--)
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
	if (text.length() > 0)
		ret->push_back(text);
	return ret;
}
SDL_Surface * CMessage::genMessage
(std::string title, std::string text, EWindowType type, std::vector<CSemiDefHandler*> *addPics, void * cb)
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

	SDL_Surface * ret = drawBox1(ww,hh);
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