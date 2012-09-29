#include "StdInc.h"
#include "CMessage.h"

#include "SDL_ttf.h"
#include "CDefHandler.h"
#include "CAnimation.h"
#include "CGameInfo.h"
#include "UIFramework/SDL_Extensions.h"
#include "../lib/CGeneralTextHandler.h"
#include "Graphics.h"
#include "GUIClasses.h"
#include "../lib/CConfigHandler.h"
#include "CBitmapHandler.h"
#include "UIFramework/CIntObjectClasses.h"

/*
 * CMessage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//extern SDL_Surface * screen;

const int COMPONENT_TO_SUBTITLE = 17;
const int BETWEEN_COMPS_ROWS = 10;
const int BEFORE_COMPONENTS = 30;
const int BETWEEN_COMPS = 30;
const int SIDE_MARGIN = 30;

template <typename T, typename U> std::pair<T,U> max(const std::pair<T,U> &x, const std::pair<T,U> &y)
{
	std::pair<T,U> ret;
	ret.first = std::max(x.first,y.first);
	ret.second = std::max(x.second,y.second);
	return ret;
}

//One image component + subtitles below it
class ComponentResolved : public CIntObject
{
public:
	CComponent *comp;

	//blit component with image centered at this position
	void showAll(SDL_Surface * to);

	//ComponentResolved(); //c-tor
	ComponentResolved(CComponent *Comp); //c-tor
	~ComponentResolved(); //d-tor
};
// Full set of components for blitting on dialog box
struct ComponentsToBlit
{
	std::vector< std::vector<ComponentResolved*> > comps;
	int w, h;

	void blitCompsOnSur(SDL_Surface * _or, int inter, int &curh, SDL_Surface *ret);
	ComponentsToBlit(std::vector<CComponent*> & SComps, int maxw, SDL_Surface* _or); //c-tor
	~ComponentsToBlit(); //d-tor
};

namespace
{
	CDefHandler * ok, *cancel;
	std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	SDL_Surface * background = NULL;
}

void CMessage::init()
{
	{
		piecesOfBox.resize(GameConstants::PLAYER_LIMIT);
		for (int i=0;i<GameConstants::PLAYER_LIMIT;i++)
		{
			CDefHandler * bluePieces = CDefHandler::giveDef("DIALGBOX.DEF");
			if (i==1)
			{
				for (size_t j=0;j<bluePieces->ourImages.size();++j)
				{
					piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
					bluePieces->ourImages[j].bitmap->refcount++;
				}
			}
			for (size_t j=0;j<bluePieces->ourImages.size();++j)
			{
				graphics->blueToPlayersAdv(bluePieces->ourImages[j].bitmap,i);
				piecesOfBox[i].push_back(bluePieces->ourImages[j].bitmap);
				bluePieces->ourImages[j].bitmap->refcount++;
			}
			delete bluePieces;
		}
		background = BitmapHandler::loadBitmap("DIBOXBCK.BMP");
		SDL_SetColorKey(background,SDL_SRCCOLORKEY,SDL_MapRGB(background->format,0,255,255));
	}
	ok = CDefHandler::giveDef("IOKAY.DEF");
	cancel = CDefHandler::giveDef("ICANCEL.DEF");
}

void CMessage::dispose()
{
	for (int i=0;i<GameConstants::PLAYER_LIMIT;i++)
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

SDL_Surface * CMessage::drawDialogBox(int w, int h, int playerColor)
{
	//prepare surface
	SDL_Surface * ret = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, screen->format->BitsPerPixel, screen->format->Rmask, screen->format->Gmask, screen->format->Bmask, screen->format->Amask);
	for (int i=0; i<w; i+=background->w)//background
	{
		for (int j=0; j<h; j+=background->h)
		{
			Rect srcR(0,0,background->w, background->h);
			Rect dstR(i,j,w,h);
			CSDL_Ext::blitSurface(background, &srcR, ret, &dstR);
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

	boost::algorithm::trim_right_if(text,boost::algorithm::is_any_of(std::string(" ")));

	while (text.length())
	{
		ui32 lineLength = 0;	//in characters or given char metric
		ui32 z = 0; //our position in text
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
			 * suitable character.
			 * Note: Cyrillic symbols have indexes 220-255 so we need
			 * to use ui8 for comparison
			 */
			int pos = z-1;
			while(pos > 0 &&  ((ui8)text[pos]) > ' ' )
				pos --;

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
			boost::algorithm::trim_left_if(text,boost::algorithm::is_any_of(std::string(" "))); 

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

SDL_Surface * FNT_RenderText (EFonts font, std::string text, SDL_Color kolor= Colors::Cornsilk)
{
	if (graphics->fontsTrueType[font])
		return TTF_RenderText_Blended(graphics->fontsTrueType[font], text.c_str(), kolor);
	const Font *f = graphics->fonts[font];
	int w = f->getWidth(text.c_str()),
	    h = f->height;
	SDL_Surface * ret = CSDL_Ext::newSurface(w, h, screen);
	CSDL_Ext::fillRect (ret, NULL, SDL_MapRGB(ret->format,128,128,128));//if use default black - no shadowing
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
				(*txtg)[i].push_back(FNT_RenderText(font, (*brtext)[i].substr(0,z), Colors::Cornsilk));
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
				(*txtg)[i].push_back(FNT_RenderText(font, (*brtext)[i].substr(0,z), Colors::Jasmine));
			(*brtext)[i].erase(0,z);

			if ((*brtext)[i].length() && (*brtext)[i][0] == '}')
				/* Remove '}' */
				(*brtext)[i].erase(0,1);

		} //ends while((*brtext)[i].length())
	} //ends for(int i=0; i<brtext->size();i++)
	return txtg;
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
	SDL_Surface *ret = drawDialogBox(boxs.first,boxs.second,player);
	blitTextOnSur(txtg,fontHeight,curh,ret);
	curh += imgToBmp;
	blitAt(bitmap,(ret->w/2)-(bitmap->w/2),curh,ret);
	curh += bitmap->h + 5;
	CSDL_Ext::printAtMiddle(sub,ret->w/2,curh+10,FONT_SMALL,Colors::Cornsilk,ret);
	delete txtg;
	return ret;
}

void CMessage::drawIWindow(CInfoWindow * ret, std::string text, int player)
{
	SDL_Surface * _or = NULL;

	if(dynamic_cast<CSelWindow*>(ret)) //it's selection window, so we'll blit "or" between components
		_or = FNT_RenderText(FONT_MEDIUM,CGI->generaltexth->allTexts[4],Colors::Cornsilk);

	const int sizes[][2] = {{400, 125}, {500, 150}, {600, 200}, {480, 400}};

	for(int i = 0; 
		i < ARRAY_COUNT(sizes) 
			&& sizes[i][0] < screen->w - 150  
			&& sizes[i][1] < screen->h - 150
			&& ret->text->slider;
		i++)
	{
		ret->text->setBounds(sizes[i][0], sizes[i][1]);
	}

	if(ret->text->slider)
		ret->text->slider->addUsedEvents(CIntObject::WHEEL | CIntObject::KEYBOARD);

	std::pair<int,int> winSize(ret->text->pos.w, ret->text->pos.h); //start with text size

	ComponentsToBlit comps(ret->components,500,_or);
	if (ret->components.size())
		winSize.second += 10 + comps.h; //space to first component

	int bw = 0;
	if (ret->buttons.size())
	{
		// Compute total width of buttons
		bw = 20*(ret->buttons.size()-1); // space between all buttons
		for(size_t i=0; i<ret->buttons.size(); i++) //and add buttons width
			bw+=ret->buttons[i]->pos.w;
		winSize.second += 20 + //before button
		ok->ourImages[0].bitmap->h; //button	
	}

	// Clip window size
	vstd::amax(winSize.second, 50);
	vstd::amax(winSize.first, 80);
	vstd::amax(winSize.first, comps.w);
	vstd::amax(winSize.first, bw);

	vstd::amin(winSize.first, screen->w - 150);

	ret->bitmap = drawDialogBox (winSize.first + 2*SIDE_MARGIN, winSize.second + 2*SIDE_MARGIN, player);
	ret->pos.h=ret->bitmap->h;
	ret->pos.w=ret->bitmap->w;
	ret->center();

	int curh = SIDE_MARGIN;
	int xOffset = (ret->pos.w - ret->text->pos.w)/2;

	if(!ret->buttons.size() && !ret->components.size()) //improvement for very small text only popups -> center text vertically
	{
		if(ret->bitmap->h > ret->text->pos.h + 2*SIDE_MARGIN)
			curh = (ret->bitmap->h - ret->text->pos.h)/2;
	}

	ret->text->moveBy(Point(xOffset, curh));

	curh += ret->text->pos.h;

	if (ret->components.size())
	{
		curh += BEFORE_COMPONENTS;
		comps.blitCompsOnSur (_or, BETWEEN_COMPS, curh, ret->bitmap);
	}
	if(ret->buttons.size())
	{
		// Position the buttons at the bottom of the window
		bw = (ret->bitmap->w/2) - (bw/2);
		curh = ret->bitmap->h - SIDE_MARGIN - ret->buttons[0]->pos.h;

		for(size_t i=0; i<ret->buttons.size(); i++)
		{
			ret->buttons[i]->moveBy(Point(bw, curh));
			bw += ret->buttons[i]->pos.w + 20;
		}
	}
	for(size_t i=0; i<ret->components.size(); i++)
		ret->components[i]->moveBy(Point(ret->pos.x, ret->pos.y));

	if(_or)
		SDL_FreeSurface(_or);
}

void CMessage::drawBorder(int playerColor, SDL_Surface * ret, int w, int h, int x, int y)
{	
	std::vector<SDL_Surface *> &box = piecesOfBox[playerColor];

	// Note: this code assumes that the corner dimensions are all the same.

	// Horizontal borders
	int start_x = x + box[0]->w;
	const int stop_x = x + w - box[1]->w;
	const int bottom_y = y+h-box[7]->h+1;
	while (start_x < stop_x) {
		int cur_w = stop_x - start_x;
		if (cur_w > box[6]->w)
			cur_w = box[6]->w;

		// Top border
		Rect srcR(0, 0, cur_w, box[6]->h);
		Rect dstR(start_x, y, 0, 0);
		CSDL_Ext::blitSurface(box[6], &srcR, ret, &dstR);
		
		// Bottom border
		dstR.y = bottom_y;
		CSDL_Ext::blitSurface(box[7], &srcR, ret, &dstR);

		start_x += cur_w;
	}

	// Vertical borders
	int start_y = y + box[0]->h;
	const int stop_y = y + h - box[2]->h+1;
	const int right_x = x+w-box[5]->w;
	while (start_y < stop_y) {
		int cur_h = stop_y - start_y;
		if (cur_h > box[4]->h)
			cur_h = box[4]->h;

		// Left border
		Rect srcR(0, 0, box[4]->w, cur_h);
		Rect dstR(x, start_y, 0, 0);
		CSDL_Ext::blitSurface(box[4], &srcR, ret, &dstR);

		// Right border
		dstR.x = right_x;
		CSDL_Ext::blitSurface(box[5], &srcR, ret, &dstR);

		start_y += cur_h;
	}

	//corners
	Rect dstR(x, y, box[0]->w, box[0]->h);
	CSDL_Ext::blitSurface(box[0], NULL, ret, &dstR);

	dstR=Rect(x+w-box[1]->w, y,   box[1]->w, box[1]->h);
	CSDL_Ext::blitSurface(box[1], NULL, ret, &dstR);

	dstR=Rect(x, y+h-box[2]->h+1, box[2]->w, box[2]->h);
	CSDL_Ext::blitSurface(box[2], NULL, ret, &dstR);

	dstR=Rect(x+w-box[3]->w, y+h-box[3]->h+1, box[3]->w, box[3]->h);
	CSDL_Ext::blitSurface(box[3], NULL, ret, &dstR);
}

ComponentResolved::ComponentResolved( CComponent *Comp ):
	comp(Comp)
{
	//Temporary assign ownership on comp
	if (parent)
		parent->removeChild(this);
	if (comp->parent)
	{
		comp->parent->addChild(this);
		comp->parent->removeChild(comp);
	}

	addChild(comp);
	defActions = 255 - DISPOSE;
	pos.x = pos.y = 0;

	pos.w = comp->pos.w;
	pos.h = comp->pos.h;
}

ComponentResolved::~ComponentResolved()
{
	if (parent)
	{
		removeChild(comp);
		parent->addChild(comp);
	}
}

void ComponentResolved::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	comp->showAll(to);
}

ComponentsToBlit::~ComponentsToBlit()
{
	for(size_t i=0; i<comps.size(); i++)
		for(size_t j = 0; j < comps[i].size(); j++)
			delete comps[i][j];

}

ComponentsToBlit::ComponentsToBlit(std::vector<CComponent*> & SComps, int maxw, SDL_Surface* _or)
{
	w = h = 0;
	if(SComps.empty())
		return;

	comps.resize(1);
	int curw = 0;
	int curr = 0; //current row

	for(size_t i=0;i<SComps.size();i++)
	{
		ComponentResolved *cur = new ComponentResolved(SComps[i]);

		int toadd = (cur->pos.w + BETWEEN_COMPS + (_or ? _or->w : 0));
		if (curw + toadd > maxw)
		{
			curr++;
			vstd::amax(w,curw);
			curw = cur->pos.w;
			comps.resize(curr+1);
		}
		else
		{
			curw += toadd;
			vstd::amax(w,curw);
		}

		comps[curr].push_back(cur);
	}

	for(size_t i=0;i<comps.size();i++)
	{
		int maxHeight = 0;
		for(size_t j=0;j<comps[i].size();j++)
			vstd::amax(maxHeight, comps[i][j]->pos.h);

		h += maxHeight + BETWEEN_COMPS_ROWS;
	}
}

void ComponentsToBlit::blitCompsOnSur( SDL_Surface * _or, int inter, int &curh, SDL_Surface *ret )
{
	for (size_t i=0;i<comps.size();i++)//for each row
	{
		int totalw=0, maxHeight=0;
		for(size_t j=0;j<(comps)[i].size();j++)//find max height & total width in this row
		{
			ComponentResolved *cur = (comps)[i][j];
			totalw += cur->pos.w;
			vstd::amax(maxHeight, cur->pos.h);
		}

		//add space between comps in this row
		if(_or)
			totalw += (inter*2+_or->w) * ((comps)[i].size() - 1);
		else
			totalw += (inter) * ((comps)[i].size() - 1);

		int middleh = curh + maxHeight/2;//axis for image aligment
		int curw = ret->w/2 - totalw/2;

		for(size_t j=0;j<(comps)[i].size();j++)
		{
			ComponentResolved *cur = (comps)[i][j];
			cur->moveTo(Point(curw, curh));

			//blit component
			cur->showAll(ret);
			curw += cur->pos.w;

			//if there is subsequent component blit "or"
			if(j<((comps)[i].size()-1))
			{
				if(_or)
				{
					curw+=inter;
					blitAt(_or,curw,middleh-(_or->h/2),ret);
					curw+=_or->w;
				}
				curw+=inter;
			}
		}
		curh += maxHeight + BETWEEN_COMPS_ROWS;
	}
}
