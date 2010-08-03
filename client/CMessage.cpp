#include "../stdafx.h"
#include "CMessage.h"
#include "SDL_ttf.h"
#include "../hch/CDefHandler.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include "../hch/CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <sstream>
#include "../hch/CGeneralTextHandler.h"
#include "Graphics.h"
#include "GUIClasses.h"
#include "AdventureMapButton.h"
#include "CConfigHandler.h"

/*
 * CMessage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

SDL_Color tytulowy = {229, 215, 123, 0}, 
	tlo = {66, 44, 24, 0}, 
	zwykly = {255, 255, 255, 0},
	darkTitle = {215, 175, 78, 0};

extern SDL_Surface * screen;

using namespace NMessage;

const int COMPONENT_TO_SUBTITLE = 5;
const int BETWEEN_COMPS_ROWS = 10;
const int BEFORE_COMPONENTS = 30;
const int SIDE_MARGIN = 30;

template <typename T, typename U> std::pair<T,U> max(const std::pair<T,U> &x, const std::pair<T,U> &y)
{
	std::pair<T,U> ret;
	ret.first = std::max(x.first,y.first);
	ret.second = std::max(x.second,y.second);
	return ret;
}

namespace NMessage
{
	CDefHandler * ok, *cancel;
	std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	SDL_Surface * background = NULL;
}


void CMessage::init()
{
	{
		for (int i=0;i<PLAYER_LIMIT;i++)
		{
			CDefHandler * bluePieces = CDefHandler::giveDef("DIALGBOX.DEF");
			std::vector<SDL_Surface *> n;
			piecesOfBox.push_back(n);
			if (i==1)
			{
				for (size_t j=0;j<bluePieces->ourImages.size();++j)
				{
					piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
				}
			}
			for (size_t j=0;j<bluePieces->ourImages.size();++j)
			{
				graphics->blueToPlayersAdv(bluePieces->ourImages[j].bitmap,i);
				piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
			}
		}
		NMessage::background = BitmapHandler::loadBitmap("DIBOXBCK.BMP");
		SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	}
	ok = CDefHandler::giveDef("IOKAY.DEF");
	cancel = CDefHandler::giveDef("ICANCEL.DEF");
}


void CMessage::dispose()
{
	for (int i=0;i<PLAYER_LIMIT;i++)
	{
		for (size_t j=0; j<piecesOfBox[i].size(); ++j)
		{
			SDL_FreeSurface(piecesOfBox[i][j]);
		}
	}
	SDL_FreeSurface(background);
	delete ok;
	delete cancel;
}
SDL_Surface * CMessage::drawBox1(int w, int h, int playerColor) //draws box for window
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	for (int i=0; i<h; i+=background->h)//background
	{
		for (int j=0; j<w; j+=background->w)
		{
			SDL_BlitSurface(background,&genRect(background->h,background->w,0,0),ret,&genRect(h,w,j,i)); //FIXME taking address of temporary
		}
	}
	drawBorder(playerColor, ret, w, h);
	return ret;
}

/* The map file contains long texts, with or without line breaks. This
 * method takes such a text and breaks it into into several lines. */
std::vector<std::string> CMessage::breakText( std::string text, size_t maxLineSize/*=30*/, const boost::function<int(char)> &charMetric /*= 0*/, bool allowLeadingWhitespace /*= false*/ )
{
	std::vector<std::string> ret;

	boost::algorithm::trim_right_if(text,boost::algorithm::is_any_of(" "));

	while (text.length())
	{
		unsigned int lineLength = 0;	//in characters or given char metric
		unsigned int z = 0; //our position in text
		bool opened = false;//if we have an unclosed brace in current line
		bool lineManuallyBroken = false;

		while(z < text.length()  &&  text[z] != 0x0a  &&  lineLength < maxLineSize)
		{
			/* We don't count braces in string length. */
			if (text[z] == '{')
				opened=true;
			else if (text[z]=='}')
				opened=false;
			else if(charMetric)
				lineLength += charMetric(text[z]);
			else
				lineLength++;

			z++;
		}

		if (z < text.length()  &&  (text[z] != 0x0a))
		{
			/* We have a long line. Try to do a nice line break, if
			 * possible. We backtrack on the line until we find a
			 * suitable character. */
			int pos = z-1;

			// Do not break an ellipsis, backtrack until whitespace.
			if (text[pos] == '.' && text[z] == '.') 
			{
				while (pos != 0 && text[pos] != ' ')
					pos--;
			} 
			else 
			{
			/* TODO: boost should have a nice method to do that. */
				while(pos > 0 && text[pos]>' ')
					/*  text[pos] != ' ' && 
					  text[pos] != ',' &&
					  text[pos] != '.' &&
					  text[pos] != ';' &&
					  text[pos] != '!' &&
					  text[pos] != '?')*/
					pos --;
			}
			if (pos > 0)
				z = pos+1;
		}
		
		if(z) //non-blank line 
		{
			ret.push_back(text.substr(0, z));

			if (opened)
				/* Close the brace for the current line. */
				ret.back() += '}';

			text.erase(0, z);
		}
		else if(text[z] == 0x0a) //blank line 
		{
			ret.push_back(""); //add empty string, no extra actions needed
		}

		if (text.length() && text[0] == 0x0a)
		{
			/* Braces do not carry over lines. The map author forgot
			 * to close it. */
			opened = false;

			/* Remove LF */
			text.erase(0, 1);

			lineManuallyBroken = true;
		}

		if(!allowLeadingWhitespace || !lineManuallyBroken)
			boost::algorithm::trim_left_if(text,boost::algorithm::is_any_of(" ")); 

		if (opened)
		{
			/* Add an opening brace for the next line. */
			if (text.length())
				text.insert(0, "{");
		}
	}

	/* Trim whitespaces of every line. */
	if(!allowLeadingWhitespace)
		for (size_t i=0; i<ret.size(); i++)
			boost::algorithm::trim(ret[i]);

	return ret;
}

std::vector<std::string> CMessage::breakText( std::string text, size_t maxLineWidth, EFonts font )
{
	return breakText(text, maxLineWidth, boost::bind(&Font::getCharWidth, graphics->fonts[font], _1), true);
}

std::pair<int,int> CMessage::getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight)
{
	std::pair<int,int> ret;
	ret.first = -1;
	ret.second=0;
	for (size_t i=0; i<txtg->size();i++) //we are searching widest line and total height
	{
		int lw=0;
		for (size_t j=0;j<(*txtg)[i].size();j++)
		{
			lw+=(*txtg)[i][j]->w;
			ret.second+=(*txtg)[i][j]->h;
		}
		if(!(*txtg)[i].size())
			ret.second+=fontHeight;
		if (ret.first<lw)
			ret.first=lw;
	}
	return ret;
}

// Blit the text in txtg onto one surface. txtg contains lines of
// text. Each line can be split into pieces. Currently only lines with
// the same height are supported (ie. fontHeight).
SDL_Surface * CMessage::blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight, int & curh, SDL_Surface * ret, int xCenterPos)
{
	for (size_t i=0; i<txtg->size(); i++, curh += fontHeight)
	{
		int lw=0; //line width
		for (size_t j=0;j<(*txtg)[i].size();j++)
			lw+=(*txtg)[i][j]->w; 

		int pw = (xCenterPos < 0)  ?  ret->w/2  :  xCenterPos; 
		pw -= lw/2; //x coord for the start of the text

		int tw = pw;
		for (size_t j=0;j<(*txtg)[i].size();j++) //blit text
		{
			SDL_Surface *surf = (*txtg)[i][j];
			blitAt(surf, tw, curh, ret);
			tw+=surf->w;
			SDL_FreeSurface(surf);
			(*txtg)[i][j] = NULL;
		}
	}

	return ret;
}

SDL_Surface * FNT_RenderText (EFonts font, std::string text, SDL_Color kolor= zwykly)
{
	if (graphics->fontsTrueType[font])
		return TTF_RenderText_Blended(graphics->fontsTrueType[font], text.c_str(), kolor);
	const Font *f = graphics->fonts[font];
	int w = f->getWidth(text.c_str()),
	    h = f->height;
	SDL_Surface * ret = CSDL_Ext::newSurface(w, h, screen);
	SDL_FillRect (ret, NULL, SDL_MapRGB(ret->format,128,128,128));//if use default black - no shadowing
	SDL_SetColorKey(ret,SDL_SRCCOLORKEY,SDL_MapRGB(ret->format,128,128,128));
	CSDL_Ext::printAt(text.c_str(), 0, 0, font, kolor, ret);
	return ret;
}

std::vector<std::vector<SDL_Surface*> > * CMessage::drawText(std::vector<std::string> * brtext, int &fontHeigh, EFonts font)
{
	std::vector<std::vector<SDL_Surface*> > * txtg = new std::vector<std::vector<SDL_Surface*> >();
	txtg->resize(brtext->size());
	if (graphics->fontsTrueType[font])
		fontHeigh = TTF_FontHeight(graphics->fontsTrueType[font]);
	else
		fontHeigh = graphics->fonts[font]->height;

	for (size_t i=0; i<brtext->size();i++) //foreach line
	{
		while((*brtext)[i].length()) //if something left
		{
			size_t z;

			/* Handle normal text. */
			z = 0;
			while(z < (*brtext)[i].length() && (*brtext)[i][z] != ('{'))
				z++;

			if (z)
				(*txtg)[i].push_back(FNT_RenderText(font, (*brtext)[i].substr(0,z), zwykly));
			(*brtext)[i].erase(0,z);

			if ((*brtext)[i].length() && (*brtext)[i][0] == '{')
				/* Remove '{' */
				(*brtext)[i].erase(0,1);

			if ((*brtext)[i].length()==0)
				/* End of line */
				continue;

			/* This text will be highlighted. */
			z = 0;
			while(z < (*brtext)[i].length() && (*brtext)[i][z] != ('}'))
				z++;

			if (z)
				(*txtg)[i].push_back(FNT_RenderText(font, (*brtext)[i].substr(0,z), tytulowy));
			(*brtext)[i].erase(0,z);

			if ((*brtext)[i].length() && (*brtext)[i][0] == '}')
				/* Remove '}' */
				(*brtext)[i].erase(0,1);

		} //ends while((*brtext)[i].length())
	} //ends for(int i=0; i<brtext->size();i++)
	return txtg;
}
CSimpleWindow * CMessage::genWindow(std::string text, int player, bool centerOnMouse, int Lmar, int Rmar, int Tmar, int Bmar)
{
	CSimpleWindow * ret = new CSimpleWindow();
	int fontHeight;
	std::vector<std::string> brtext = breakText(text,32);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(&brtext, fontHeight);
	std::pair<int,int> txts = getMaxSizes(txtg, fontHeight);
	ret->bitmap = drawBox1(txts.first+Lmar+Rmar,txts.second+Tmar+Bmar,player);
	ret->pos.h = ret->bitmap->h;
	ret->pos.w = ret->bitmap->w;
	if (centerOnMouse) 
	{
		ret->pos.x = GH.current->motion.x - ret->pos.w/2;
		ret->pos.y = GH.current->motion.y - ret->pos.h/2;
		// Put the window back on screen if necessary
		amax(ret->pos.x, 0);
		amax(ret->pos.y, 0);
		amin(ret->pos.x, conf.cc.resx - ret->pos.w);
		amin(ret->pos.y, conf.cc.resy - ret->pos.h);
	} 
	else 
	{
		// Center on screen
		ret->pos.x = screen->w/2 - (ret->pos.w/2);
		ret->pos.y = screen->h/2 - (ret->pos.h/2);
	}
	int curh = ret->bitmap->h/2 - (fontHeight*txtg->size())/2;
	blitTextOnSur(txtg,fontHeight,curh,ret->bitmap);
	delete txtg;
	return ret;
}
SDL_Surface * CMessage::drawBoxTextBitmapSub( int player, std::string text, SDL_Surface* bitmap, std::string sub, int charperline/*=30*/, int imgToBmp/*=55*/ )
{
	int curh;
	int fontHeight;
	std::vector<std::string> tekst = breakText(text,charperline);
	std::vector<std::vector<SDL_Surface*> > * txtg = drawText(&tekst, fontHeight);
	std::pair<int,int> txts = getMaxSizes(txtg, fontHeight), boxs;
	boxs.first = std::max(txts.first,bitmap->w) // text/bitmap max width
		+ 50; //side margins
	boxs.second =
		(curh=45) //top margin
		+ txts.second //text total height
		+ imgToBmp //text <=> img
		+ bitmap->h
		+ 5 // to sibtitle
		+ (*txtg)[0][0]->h
		+ 30;
	SDL_Surface *ret = drawBox1(boxs.first,boxs.second,player);
	blitTextOnSur(txtg,fontHeight,curh,ret);
	curh += imgToBmp;
	blitAt(bitmap,(ret->w/2)-(bitmap->w/2),curh,ret);
	curh += bitmap->h + 5;
	CSDL_Ext::printAtMiddle(sub,ret->w/2,curh+10,FONT_SMALL,zwykly,ret);
	delete txtg;
	return ret;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, int player)
{
	SDL_Surface * _or = NULL;
	const Font &f = *graphics->fonts[FONT_MEDIUM];
	int fontHeight = f.height;

	if(dynamic_cast<CSelWindow*>(ret)) //it's selection window, so we'll blit "or" between components
		_or = FNT_RenderText(FONT_MEDIUM,CGI->generaltexth->allTexts[4],zwykly);

	const int sizes[][2] = {{400, 125}, {500, 150}, {600, 200}};
	for(int i = 0; 
		i < ARRAY_COUNT(sizes) 
			&& sizes[i][0] < conf.cc.resx - 150  
			&& sizes[i][1] < conf.cc.resy - 150
			&& ret->text->slider;
		i++)
	{
		ret->text->setBounds(sizes[i][0], sizes[i][1]);
	}

	if(ret->text->slider)
		ret->text->slider->changeUsedEvents(CIntObject::WHEEL | CIntObject::KEYBOARD, true);

	std::pair<int,int> winSize(ret->text->pos.w, ret->text->pos.h); //start with text size

	ComponentsToBlit comps(ret->components,500,_or);
	if (ret->components.size())
		winSize.second += 30 + comps.h; //space to first component

	int bw = 0;
	if (ret->buttons.size())
	{
		// Compute total width of buttons
		bw = 20*(ret->buttons.size()-1); // space between all buttons
		for(size_t i=0; i<ret->buttons.size(); i++) //and add buttons width
			bw+=ret->buttons[i]->imgs[0][0]->w; 
		winSize.second += 20 + //before button
		ok->ourImages[0].bitmap->h; //button	
	}

	// Clip window size
	amax(winSize.second, 50);
	amax(winSize.first, 80);
	amax(winSize.first, comps.w);
	amax(winSize.first, bw);

	amin(winSize.first, conf.cc.resx - 150);

	ret->bitmap = drawBox1 (winSize.first + 2*SIDE_MARGIN, winSize.second + 2*SIDE_MARGIN, player);
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;
	ret->center();


	int curh = SIDE_MARGIN;

	int xOffset = (ret->pos.w - ret->text->pos.w)/2;
	ret->text->moveBy(Point(xOffset, SIDE_MARGIN));

	//blitTextOnSur (txtg, fontHeight, curh, ret->bitmap);

	curh += ret->text->pos.h;

	if (ret->components.size())
	{
		curh += BEFORE_COMPONENTS;
		comps.blitCompsOnSur (_or, 10, curh, ret->bitmap);
	}
	if(ret->buttons.size())
	{
		// Position the buttons at the bottom of the window
		bw = (ret->bitmap->w/2) - (bw/2);
		curh = ret->bitmap->h - SIDE_MARGIN - ret->buttons[0]->imgs[0][0]->h;

		for(size_t i=0; i<ret->buttons.size(); i++)
		{
			ret->buttons[i]->pos.x = bw + ret->pos.x;
			ret->buttons[i]->pos.y = curh + ret->pos.y;
			bw += ret->buttons[i]->imgs[0][0]->w + 20;
		}
	}
	for(size_t i=0; i<ret->components.size(); i++)
	{
		ret->components[i]->pos.x += ret->pos.x;
		ret->components[i]->pos.y += ret->pos.y;
	}

	if(_or)
		SDL_FreeSurface(_or);
}

void CMessage::drawBorder(int playerColor, SDL_Surface * ret, int w, int h, int x, int y)
{	
	//obwodka I-szego rzedu pozioma //border of 1st series, horizontal
	for (int i=0; i<w-piecesOfBox[playerColor][6]->w; i+=piecesOfBox[playerColor][6]->w)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][6],NULL,ret,&genRect(piecesOfBox[playerColor][6]->h,piecesOfBox[playerColor][6]->w,x+i,y+0));
		SDL_BlitSurface
			(piecesOfBox[playerColor][7],NULL,ret,&genRect(piecesOfBox[playerColor][7]->h,piecesOfBox[playerColor][7]->w,x+i,y+h-piecesOfBox[playerColor][7]->h+1));
	}
	//obwodka I-szego rzedu pionowa  //border of 1st series, vertical
	for (int i=0; i<h-piecesOfBox[playerColor][4]->h; i+=piecesOfBox[playerColor][4]->h)
	{
		SDL_BlitSurface
			(piecesOfBox[playerColor][4],NULL,ret,&genRect(piecesOfBox[playerColor][4]->h,piecesOfBox[playerColor][4]->w,x+0,y+i));
		SDL_BlitSurface
			(piecesOfBox[playerColor][5],NULL,ret,&genRect(piecesOfBox[playerColor][5]->h,piecesOfBox[playerColor][5]->w,x+w-piecesOfBox[playerColor][5]->w,y+i));
	}
	//corners
	SDL_BlitSurface
		(piecesOfBox[playerColor][0],NULL,ret,&genRect(piecesOfBox[playerColor][0]->h,piecesOfBox[playerColor][0]->w,x+0,y+0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][1],NULL,ret,&genRect(piecesOfBox[playerColor][1]->h,piecesOfBox[playerColor][1]->w,x+w-piecesOfBox[playerColor][1]->w,y+0));
	SDL_BlitSurface
		(piecesOfBox[playerColor][2],NULL,ret,&genRect(piecesOfBox[playerColor][2]->h,piecesOfBox[playerColor][2]->w,x+0,y+h-piecesOfBox[playerColor][2]->h+1));
	SDL_BlitSurface
		(piecesOfBox[playerColor][3],NULL,ret,&genRect(piecesOfBox[playerColor][3]->h,piecesOfBox[playerColor][3]->w,x+w-piecesOfBox[playerColor][3]->w,y+h-piecesOfBox[playerColor][3]->h+1));
}

ComponentResolved::ComponentResolved()
{
	comp = NULL;
	img = NULL;
	txt = NULL;
	txtFontHeight = 0;
}

ComponentResolved::ComponentResolved( SComponent *Comp )
{
	comp = Comp;
	img = comp->getImg();
	std::vector<std::string> brtext = CMessage::breakText(comp->subtitle,13); //text 
	txt = CMessage::drawText(&brtext,txtFontHeight,FONT_MEDIUM);

	//calculate dimensions
	std::pair<int,int> textSize = CMessage::getMaxSizes(txt, txtFontHeight);
	comp->pos.w = std::max(textSize.first, img->w); //bigger of: subtitle width and image width
	comp->pos.h = img->h + COMPONENT_TO_SUBTITLE + textSize.second;
}

ComponentResolved::~ComponentResolved()
{
	for(size_t i = 0; i < txt->size(); i++)
		for(size_t j = 0; j < (*txt)[i].size(); j++)
			if((*txt)[i][j])
				SDL_FreeSurface((*txt)[i][j]);
	delete txt;
}

ComponentsToBlit::~ComponentsToBlit()
{
	for(size_t i=0; i<comps.size(); i++)
		for(size_t j = 0; j < comps[i].size(); j++)
			delete comps[i][j];

}

ComponentsToBlit::ComponentsToBlit(std::vector<SComponent*> & SComps, int maxw, SDL_Surface* _or)
{
	w = h = 0;
	if(!SComps.size())
		return;

	comps.resize(1);
	int curw = 0;
	int curr = 0; //current row

	for(size_t i=0;i<SComps.size();i++)
	{
		ComponentResolved *cur = new ComponentResolved(SComps[i]);

		int toadd = (cur->comp->pos.w + 12 + (_or ? _or->w : 0));
		if (curw + toadd > maxw)
		{
			curr++;
			amax(w,curw);
			curw = cur->comp->pos.w;
			comps.resize(curr+1);
		}
		else
		{
			curw += toadd;
			amax(w,curw);
		}

		comps[curr].push_back(cur);
	}

	for(size_t i=0;i<comps.size();i++)
	{
		int maxh = 0;
		for(size_t j=0;j<comps[i].size();j++)
			amax(maxh,comps[i][j]->comp->pos.h);
		h += maxh + BETWEEN_COMPS_ROWS;
	}
}

void ComponentsToBlit::blitCompsOnSur( SDL_Surface * _or, int inter, int &curh, SDL_Surface *ret )
{
	for (size_t i=0;i<comps.size();i++)
	{
		int totalw=0, maxh=0;
		for(size_t j=0;j<(comps)[i].size();j++)
		{
			ComponentResolved *cur = (comps)[i][j];
			totalw += cur->comp->pos.w;
			amax(maxh,cur->comp->pos.h+BETWEEN_COMPS_ROWS);
		}
		if(_or)
		{
			totalw += (inter*2+_or->w) * ((comps)[i].size() - 1);
		}
		else
		{
			totalw += (inter) * ((comps)[i].size() - 1);
		}

		curh+=maxh/2;
		int curw = (ret->w/2)-(totalw/2);
		for(size_t j=0;j<(comps)[i].size();j++)
		{
			ComponentResolved *cur = (comps)[i][j];

			//blit img
			int hlp = curh-(cur->comp->pos.h)/2;
			blitAt(cur->img, curw + (cur->comp->pos.w - cur->comp->getImg()->w)/2, hlp, ret);
			cur->comp->pos.x = curw;
			cur->comp->pos.y = hlp;

			//blit subtitle
			hlp += cur->img->h + COMPONENT_TO_SUBTITLE;
			CMessage::blitTextOnSur(cur->txt, cur->txtFontHeight, hlp, ret, cur->comp->pos.x + cur->comp->pos.w/2 );

			//if there is subsequent component blit "or"
			curw += cur->comp->pos.w;
			if(j<((comps)[i].size()-1))
			{
				if(_or)
				{
					curw+=inter;
					blitAt(_or,curw,curh-(_or->h/2),ret);
					curw+=_or->w;
				}
				curw+=inter;
			}
		}
		curh+=maxh/2;
	}
}
