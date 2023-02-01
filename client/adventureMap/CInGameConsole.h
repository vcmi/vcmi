/*
 * AdventureMapClasses.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ObjectLists.h"
#include "../../lib/FunctionList.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CGGarrison;
class CGObjectInstance;
class CGHeroInstance;
class CGTownInstance;
struct Component;
struct InfoAboutArmy;
struct InfoAboutHero;
struct InfoAboutTown;

VCMI_LIB_NAMESPACE_END

class CAnimation;
class CAnimImage;
class CShowableAnim;
class CFilledTexture;
class CButton;
class CComponent;
class CHeroTooltip;
class CTownTooltip;
class CTextBox;
class IImage;

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, uint32_t > > texts; //list<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously

	std::weak_ptr<IStatusBar> currentStatusBar;
public:
	std::string enteredText;
	void show(SDL_Surface * to) override;
	void print(const std::string &txt);
	void keyPressed (const SDL_KeyboardEvent & key) override; //call-in

	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;

	void startEnteringText();
	void endEnteringText(bool processEnteredText);
	void refreshEnteredText();

	CInGameConsole();
};
