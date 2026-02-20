/*
 * CreditsScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CreditsScreen.h"

#include "CMainMenu.h"

#include "../GameEngine.h"
#include "../widgets/TextControls.h"
#include "../widgets/ObjectLists.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include "../../AUTHORS.h"

CreditsScreen::CreditsScreen(Rect rect)
	: CIntObject(LCLICK), timePassed(0)
{
	pos.w = rect.w;
	pos.h = rect.h;
	setRedrawParent(true);
	OBJECT_CONSTRUCTION;

	addUsedEvents(TIME);

	std::string contributorsText = "";
	std::string contributorsTask = "";
	for (auto & element : contributors) 
	{
		if(element[0] != contributorsTask)
			contributorsText += "\r\n\r\n{" + LIBRARY->generaltexth->translate("vcmi.credits." + boost::to_lower_copy(element[0])) + ":}\r\n";
		contributorsText += (element[1] != "" ? element[1] : element[2]) + "\r\n";
		contributorsTask = element[0];
	}

	auto textFile = CResourceHandler::get()->load(ResourcePath("DATA/CREDITS.TXT"))->readAll();
	std::string text((char *)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"') + 1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote);

	const auto & translateCredits = [&text](const std::map<std::string, std::string> & replacements){
		for(const auto & item : replacements)
			boost::replace_first(text, "{" + item.second + ":}", "{" + LIBRARY->generaltexth->translate(item.first) + ":}");
	};
	translateCredits({
		{ "core.credits.createdBy",           "Created By"           },
		{ "core.credits.executiveProducer",   "Executive Producer"   },
		{ "core.credits.producer",            "Producer"             },
		{ "core.credits.director",            "Director"             },
		{ "core.credits.designers",           "Designers"            },
		{ "core.credits.leadProgrammers",     "Lead Programmers"     },
		{ "core.credits.programmers",         "Programmers"          },
		{ "core.credits.installerProgrammer", "Installer Programmer" },
		{ "core.credits.leadArtists",         "Lead Artists"         },
		{ "core.credits.artists",             "Artists"              },
		{ "core.credits.assetCoordinator",    "Asset Coordinator"    },
		{ "core.credits.levelDesigners",      "Level Designers"      },
		{ "core.credits.musicProducer",       "Music Producer"       },
		{ "core.credits.townThemes",          "Town Themes"          },
		{ "core.credits.music",               "Music"                },
		{ "core.credits.soundDesign",         "Sound Design"         },
		{ "core.credits.voiceProduction",     "Voice Production"     },
		{ "core.credits.voiceTalent",         "Voice Talent"         },
		{ "core.credits.leadTester",          "Lead Tester"          },
		{ "core.credits.seniorTester",        "Senior Tester"        },
		{ "core.credits.testers",             "Testers"              },
		{ "core.credits.specialThanks",       "Special Thanks"       },
		{ "core.credits.visitUsOnTheWeb",     "Visit us on the Web"  }
	});

	text = "{- " + LIBRARY->generaltexth->translate("vcmi.credits.vcmi") + " -}\r\n" + contributorsText + "\r\n\r\n{" + LIBRARY->generaltexth->translate("vcmi.credits.website") + ":}\r\nhttps://vcmi.eu\r\n\r\n\r\n\r\n\r\n{- " + LIBRARY->generaltexth->translate("vcmi.credits.heroes") + " -}\r\n\r\n\r\n" + text;
	credits = std::make_shared<CMultiLineLabel>(Rect(pos.w - 350, 0, 350, 600), FONT_CREDITS, ETextAlignment::CENTER, Colors::WHITE, text);
	credits->scrollTextTo(-600); // move all text below the screen
}

void CreditsScreen::tick(uint32_t msPassed)
{
	static const int timeToScrollByOnePx = 20;
	timePassed += msPassed;
	int scrollPosition = timePassed / timeToScrollByOnePx - 600;
	credits->scrollTextTo(scrollPosition);

	//end of credits, close this screen
	if(credits->textSize.y < scrollPosition)
		clickPressed(ENGINE->getCursorPosition());
}

void CreditsScreen::clickPressed(const Point & cursorPosition)
{
	CTabbedInt * menu = dynamic_cast<CTabbedInt *>(parent);
	assert(menu);
	menu->setActive(0);
}
