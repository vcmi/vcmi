#include "stdafx.h"
#include "CMessage.h"
#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
#include "CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "CLodHandler.h"

SDL_Color tytulowy, tlo, zwykly ;
SDL_Rect genRect(int hh, int ww, int xx, int yy);

extern SDL_Surface * ekran;
extern TTF_Font * TNRB16, *TNR, *GEOR13;
SDL_Color genRGB(int r, int g, int b, int a=0);
//void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran, unsigned char quality = 2);
bool isItIn(const SDL_Rect * rect, int x, int y);

using namespace NMessage;


namespace NMessage
{
	std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	SDL_Surface * background = NULL;
}

CMessage::CMessage()
{
	if (!NMessage::background)
		init();
}
void CMessage::init()
{
	{
		for (int i=0;i<PLAYER_LIMIT;i++)
		{
			CDefHandler * bluePieces = CGI->spriteh->giveDef("DIALGBOX.DEF");
			std::vector<SDL_Surface *> n;
			piecesOfBox.push_back(n);
			if (i==1)
			{
				for (int j=0;j<bluePieces->ourImages.size();j++)
				{
					piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
				}
			}
			for (int j=0;j<bluePieces->ourImages.size();j++)
			{
				CSDL_Ext::blueToPlayersAdv(bluePieces->ourImages[j].bitmap,i);
				piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
			}
		}
		NMessage::background = CGI->bitmaph->loadBitmap("DIBOXBCK.BMP");
		SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	}
}


void CMessage::dispose()
{
	for (int i=0;i<PLAYER_LIMIT;i++)
	{
		for (int j=0;j<piecesOfBox[i].size();j++)
		{
			SDL_FreeSurface(piecesOfBox[i][j]);
		}
	}
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

	//obwodka I-szego rzedu pozioma
	for (int i=0; i<w; i+=piecesOfBox[playerColor][6]->w)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][6],NULL,ret,&genRect(piecesOfBox[playerColor][6]->h,piecesOfBox[playerColor][6]->w,i,0));
		SDL_BlitSurface
			(piecesOfBox[playerColor][7],NULL,ret,&genRect(piecesOfBox[playerColor][7]->h,piecesOfBox[playerColor][7]->w,i,h-piecesOfBox[playerColor][7]->h));
	}
	//obwodka I-szego rzedu pionowa
	for (int i=0; i<h; i+=piecesOfBox[playerColor][4]->h)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][4],NULL,ret,&genRect(piecesOfBox[playerColor][4]->h,piecesOfBox[playerColor][4]->w,0,i));
		SDL_BlitSurface
			(piecesOfBox[playerColor][5],NULL,ret,&genRect(piecesOfBox[playerColor][5]->h,piecesOfBox[playerColor][5]->w,w-piecesOfBox[playerColor][5]->w,i));
	}
	//corners
	SDL_BlitSurface
		(piecesOfBox[playerColor][0],NULL,ret,&genRect(piecesOfBox[playerColor][0]->h,piecesOfBox[playerColor][0]->w,0,0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][1],NULL,ret,&genRect(piecesOfBox[playerColor][1]->h,piecesOfBox[playerColor][1]->w,w-piecesOfBox[playerColor][1]->w,0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][2],NULL,ret,&genRect(piecesOfBox[playerColor][2]->h,piecesOfBox[playerColor][2]->w,0,h-piecesOfBox[playerColor][2]->h));
	SDL_BlitSurface
		(piecesOfBox[playerColor][3],NULL,ret,&genRect(piecesOfBox[playerColor][3]->h,piecesOfBox[playerColor][3]->w,w-piecesOfBox[playerColor][3]->w,h-piecesOfBox[playerColor][3]->h));
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