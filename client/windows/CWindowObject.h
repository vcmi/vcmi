/*
 * CWindowObject.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

/// Basic class for windows
class CWindowObject : public CIntObject
{
	std::shared_ptr<CPicture> createBg(std::string imageName, bool playerColored);
	int getUsedEvents(int options);

	std::shared_ptr<CIntObject> shadow;
	void setShadow(bool on);

	int options;

protected:
	std::shared_ptr<CPicture> background;

	//Simple function with call to GH.popInt
	void close();
	//Used only if RCLICK_POPUP was set
	void clickRight(tribool down, bool previousState) override;
	//To display border
	void updateShadow();
	void setBackground(std::string filename);
public:
	enum EOptions
	{
		PLAYER_COLORED=1, //background will be player-colored
		RCLICK_POPUP=2, // window will behave as right-click popup
		BORDERED=4, // window will have border if current resolution is bigger than size of window
		SHADOW_DISABLED=8 //this window won't display any shadow
	};

	/*
	 * options - EOpions enum
	 * imageName - name for background image, can be empty
	 * centerAt - position of window center. Default - center of the screen
	*/
	CWindowObject(int options, std::string imageName, Point centerAt);
	CWindowObject(int options, std::string imageName = "");
	~CWindowObject();

	void showAll(SDL_Surface * to) override;
};
