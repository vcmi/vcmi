//#define BOOST_SPIRIT_DEBUG
#include "CConfigHandler.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/version.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include "../lib/JsonNode.h"

using namespace config;

#if BOOST_VERSION >= 103800
#include <boost/spirit/include/classic.hpp>
using namespace boost::spirit::classic;
#else
#include <boost/spirit.hpp>
using namespace boost::spirit;
#endif

using namespace phoenix;

/*
 * CConfigHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CConfigHandler conf;

struct lerror
{
	std::string txt;
	lerror(const std::string & TXT):txt(TXT){};
	void operator()() const
	{
		tlog1 << txt << std::endl;
	}
	template<typename IteratorT>
	void operator()(IteratorT t1, IteratorT t2) const
	{
		tlog1 << txt << std::endl;
	}
};

struct lerror2
{
	std::string txt;
	lerror2(const std::string & TXT):txt(TXT){};
	template<typename IteratorT>
	void operator()(IteratorT t1, IteratorT t2) const
	{
		std::string txt2(t1,t2);
		tlog1 << txt << txt2 << std::endl;
	}
};

struct SettingsGrammar : public grammar<SettingsGrammar>
{
	template <typename ScannerT>
	struct definition
	{
		rule<ScannerT>  r, clientOption, clientOptionsSequence, ClientSettings;

		definition(SettingsGrammar const& self)  
		{ 
			clientOption 
				= str_p("resolution=") >> (uint_p[assign_a(conf.cc.resx)] >> 'x' >> uint_p[assign_a(conf.cc.resy)] | eps_p[lerror("Wrong resolution!")])
				| str_p("pregameRes=") >> (uint_p[assign_a(conf.cc.pregameResx)] >> 'x' >> uint_p[assign_a(conf.cc.pregameResy)] | eps_p[lerror("Wrong pregame size!")])
				| str_p("screenSize=") >> (uint_p[assign_a(conf.cc.screenx)] >> 'x' >> uint_p[assign_a(conf.cc.screeny)] | eps_p[lerror("Wrong screen size!")])
				| str_p("port=") >> (uint_p[assign_a(conf.cc.port)] | eps_p[lerror("Wrong port!")])
				| str_p("bpp=") >> (uint_p[assign_a(conf.cc.bpp)] | eps_p[lerror("Wrong bpp!")])
				| str_p("localInformation=") >> (uint_p[assign_a(conf.cc.localInformation)] | eps_p[lerror("Wrong localInformation!")])
				| str_p("fullscreen=") >> (uint_p[assign_a(conf.cc.fullscreen)] | eps_p[lerror("Wrong fullscreen!")])
				| str_p("server=") >> ( ( +digit_p >> *('.' >> +digit_p) )[assign_a(conf.cc.server)]   | eps_p[lerror("Wrong server!")])
				| str_p("defaultPlayerAI=") >> ((+(anychar_p - ';'))[assign_a(conf.cc.defaultPlayerAI)] | eps_p[lerror("Wrong defaultAI!")])
				| str_p("neutralBattleAI=") >> ((+(anychar_p - ';'))[assign_a(conf.cc.defaultBattleAI)] | eps_p[lerror("Wrong defaultAI!")])
				| str_p("showFPS=") >> (uint_p[assign_a(conf.cc.showFPS)] | eps_p[lerror("Wrong showFPS!")])
				| str_p("classicCreatureWindow=") >> (uint_p[assign_a(conf.cc.classicCreatureWindow)] | eps_p[lerror("Wrong classicCreatureWindow!")])
				| (+(anychar_p - '}'))[lerror2("Unrecognized client option: ")]
				;
			clientOptionsSequence = *(clientOption >> (';' | eps_p[lerror("Semicolon lacking after client option!")]));
			ClientSettings = '{' >>  clientOptionsSequence >> '}';

			r =	str_p("clientSettings") >> (ClientSettings | eps_p[lerror("Wrong clientSettings!")]);

#ifdef BOOST_SPIRIT_DEBUG
			BOOST_SPIRIT_DEBUG_RULE(clientOption);
			BOOST_SPIRIT_DEBUG_RULE(clientOptionsSequence);
			BOOST_SPIRIT_DEBUG_RULE(ClientSettings);
			BOOST_SPIRIT_DEBUG_RULE(r);
#endif
		}    

		rule<ScannerT> const& start() const { return r; }
	};
};

struct CommentsGrammar : public grammar<CommentsGrammar>
{
	template <typename ScannerT>
	struct definition
	{
		rule<ScannerT>  comment;
		definition(CommentsGrammar const& self)  
		{ 
			comment = comment_p("//") | comment_p("/*","*/") | space_p;
			BOOST_SPIRIT_DEBUG_RULE(comment);
		}
		rule<ScannerT> const& start() const { return comment; }
	};
};

static void setButton(ButtonInfo &button, const JsonNode &g)
{
	button.x = g["x"].Float();
	button.y = g["y"].Float();
	button.playerColoured = g["playerColoured"].Float();
	button.defName = g["graphic"].String();

	if (!g["additionalDefs"].isNull()) {
		const JsonVector &defs_vec = g["additionalDefs"].Vector();

		BOOST_FOREACH(const JsonNode &def, defs_vec) {
			button.additionalDefs.push_back(def.String());
		}
	}
}

static void setGem(AdventureMapConfig &ac, const int gem, const JsonNode &g)
{
	ac.gemX[gem] = g["x"].Float();
	ac.gemY[gem] = g["y"].Float();
	ac.gemG.push_back(g["graphic"].String());
}

CConfigHandler::CConfigHandler(void): current(NULL)
{
}

CConfigHandler::~CConfigHandler(void)
{
}

void config::CConfigHandler::init()
{
	std::vector<char> settings;
	std::ifstream ifs(DATA_DIR "/config/settings.txt");
	if(!ifs)
	{
		tlog1 << "Cannot open " DATA_DIR "/config/settings.txt !" << std::endl;
		return;
	}
	ifs.unsetf(std::ios::skipws); //  Turn of white space skipping on the stream
	std::copy(std::istream_iterator<char>(ifs),std::istream_iterator<char>(),std::back_inserter(settings));
	std::vector<char>::const_iterator first = settings.begin(), last = settings.end();
	SettingsGrammar sg;
	BOOST_SPIRIT_DEBUG_NODE(sg);
	CommentsGrammar cg;    
	BOOST_SPIRIT_DEBUG_NODE(cg);


	parse_info<std::vector<char>::const_iterator> info = parse(first,last,sg,cg);
	if(!info.hit)
		tlog1 << "Cannot parse config/settings.txt file!\n";
	else if(!info.full)
		tlog2 << "Not entire config/settings.txt parsed!\n";

	/* Read resolutions. */
	const JsonNode config(DATA_DIR "/config/resolutions.json");
	const JsonVector &guisettings_vec = config["GUISettings"].Vector();

	BOOST_FOREACH(const JsonNode &g, guisettings_vec) {
		std::pair<int,int> curRes(g["resolution"]["x"].Float(), g["resolution"]["y"].Float());
		GUIOptions *current = &conf.guiOptions[curRes];
		
		current->ac.inputLineLength = g["InGameConsole"]["maxInputPerLine"].Float();
		current->ac.outputLineLength = g["InGameConsole"]["maxOutputPerLine"].Float();
		
		current->ac.advmapX = g["AdvMap"]["x"].Float();
		current->ac.advmapY = g["AdvMap"]["y"].Float();
		current->ac.advmapW = g["AdvMap"]["width"].Float();
		current->ac.advmapH = g["AdvMap"]["height"].Float();
		current->ac.smoothMove = g["AdvMap"]["smoothMove"].Float();
		current->ac.puzzleSepia = g["AdvMap"]["puzzleSepia"].Float();

		current->ac.infoboxX = g["InfoBox"]["x"].Float();
		current->ac.infoboxY = g["InfoBox"]["y"].Float();

		setGem(current->ac, 0, g["gem0"]);
		setGem(current->ac, 1, g["gem1"]);
		setGem(current->ac, 2, g["gem2"]);
		setGem(current->ac, 3, g["gem3"]);

		current->ac.mainGraphic = g["background"].String();

		current->ac.hlistX = g["HeroList"]["x"].Float();
		current->ac.hlistY = g["HeroList"]["y"].Float();
		current->ac.hlistSize = g["HeroList"]["size"].Float();
		current->ac.hlistMB = g["HeroList"]["movePoints"].String();
		current->ac.hlistMN = g["HeroList"]["manaPoints"].String();
		current->ac.hlistAU = g["HeroList"]["arrowUp"].String();
		current->ac.hlistAD = g["HeroList"]["arrowDown"].String();

		current->ac.tlistX = g["TownList"]["x"].Float();
		current->ac.tlistY = g["TownList"]["y"].Float();
		current->ac.tlistSize = g["TownList"]["size"].Float();
		current->ac.tlistAU = g["TownList"]["arrowUp"].String();
		current->ac.tlistAD = g["TownList"]["arrowDown"].String();

		current->ac.minimapW = g["Minimap"]["width"].Float();
		current->ac.minimapH = g["Minimap"]["height"].Float();
		current->ac.minimapX = g["Minimap"]["x"].Float();
		current->ac.minimapY = g["Minimap"]["y"].Float();

		current->ac.overviewPics = g["Overview"]["pics"].Float();
		current->ac.overviewSize = g["Overview"]["size"].Float();
		current->ac.overviewBg = g["Overview"]["graphic"].String();

		current->ac.statusbarX = g["Statusbar"]["x"].Float();
		current->ac.statusbarY = g["Statusbar"]["y"].Float();
		current->ac.statusbarG = g["Statusbar"]["graphic"].String();

		current->ac.resdatabarX = g["ResDataBar"]["x"].Float();
		current->ac.resdatabarY = g["ResDataBar"]["y"].Float();
		current->ac.resOffsetX = g["ResDataBar"]["offsetX"].Float();
		current->ac.resOffsetY = g["ResDataBar"]["offsetY"].Float();
		current->ac.resDist = g["ResDataBar"]["resSpace"].Float();
		current->ac.resDateDist = g["ResDataBar"]["resDateSpace"].Float();
		current->ac.resdatabarG = g["ResDataBar"]["graphic"].String();

		setButton(current->ac.kingOverview, g["ButtonKingdomOv"]);
		setButton(current->ac.underground, g["ButtonUnderground"]);
		setButton(current->ac.questlog, g["ButtonQuestLog"]);
		setButton(current->ac.sleepWake, g["ButtonSleepWake"]);
		setButton(current->ac.moveHero, g["ButtonMoveHero"]);
		setButton(current->ac.spellbook, g["ButtonSpellbook"]);
		setButton(current->ac.advOptions, g["ButtonAdvOptions"]);
		setButton(current->ac.sysOptions, g["ButtonSysOptions"]);
		setButton(current->ac.nextHero, g["ButtonNextHero"]);
		setButton(current->ac.endTurn, g["ButtonEndTurn"]);
	}

	//fixing screenx / screeny if set to 0x0
	if (cc.screenx == 0 && cc.screeny == 0)
	{
		cc.screenx = cc.resx;
		cc.screeny = cc.resy;
	}

	SetResolution(cc.resx, cc.resy);
}
