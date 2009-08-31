//#define BOOST_SPIRIT_DEBUG
#include "CConfigHandler.h"
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/spirit.hpp>
#include <fstream>
using namespace config;
using namespace boost::spirit;
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
GUIOptions *current = NULL;
std::pair<int,int> curRes;
ButtonInfo *currentButton;
int gnb=-1;

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
struct SetCurButton
{
	template<typename IteratorT>
	void operator()(IteratorT t1, IteratorT t2) const
	{
		std::string str(t1,t2);
		if(str=="KingdomOv")
			currentButton = &current->ac.kingOverview;
		else if(str=="Underground")
			currentButton = &current->ac.underground;
		else if(str=="QuestLog")
			currentButton = &current->ac.questlog;
		else if(str=="SleepWake")
			currentButton = &current->ac.sleepWake;
		else if(str=="MoveHero")
			currentButton = &current->ac.moveHero;
		else if(str=="Spellbook")
			currentButton = &current->ac.spellbook;
		else if(str=="AdvOptions")
			currentButton = &current->ac.advOptions;
		else if(str=="SysOptions")
			currentButton = &current->ac.sysOptions;
		else if(str=="NextHero")
			currentButton = &current->ac.nextHero;
		else if(str=="EndTurn")
			currentButton = &current->ac.endTurn;
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

struct dummy 
{
	boost::function<void()> func;
	dummy(const boost::function<void()>  & F)
		:func(F){}
	template<typename IteratorT>
	void operator()(IteratorT t1, IteratorT t2) const
	{
		func();
	}
};

template<typename T>struct SetButtonProp
{
	T point;
	SetButtonProp(T p)
		:point(p){}
	template <typename Z>
	void operator()(const Z & val) const
	{
		currentButton->*point = val;
	}
};
template<typename T> SetButtonProp<T> SetButtonProp_a(T p)
{
	return SetButtonProp<T>(p);
}
struct SetButtonStr
{
	std::string ButtonInfo::*point;
	SetButtonStr(std::string ButtonInfo::* p)
		:point(p){}
	template <typename Z>
	void operator()(const Z first, const Z last) const
	{
		std::string str(first,last);
		currentButton->*point = str;
	}
};
template<typename T>struct SetAdventureProp
{
	T point;
	SetAdventureProp(T p)
		:point(p){}
	template <typename Z>
	void operator()(const Z & val) const
	{
		current->ac.*point = val;
	}
};
template<typename T> SetAdventureProp<T> SetAdventureProp_a(T p)
{
	return SetAdventureProp<T>(p);
}
struct SetAdventureStr
{
	std::string AdventureMapConfig::*point;
	SetAdventureStr(std::string AdventureMapConfig::* p)
		:point(p){}
	template <typename Z>
	void operator()(const Z first, const Z last) const
	{
		std::string str(first,last);
		current->ac.*point = str;
	}
};
struct AddDefForButton
{
	template <typename Z>
	void operator()(const Z first, const Z last) const
	{
		std::string str(first,last);
		currentButton->additionalDefs.push_back(str);
	}
};
static void addGRes()
{
	if(current)
		conf.guiOptions[curRes] = *current; //we'll use by default settings from previous resolution
	current = &conf.guiOptions[curRes];
}
static void setGem(int x, int val)
{
	if(x)	
		current->ac.gemX[gnb] = val;
	else
		current->ac.gemY[gnb] = val;
}
struct AddGemName
{
	template <typename Z>
	void operator()(const Z first, const Z last) const
	{
		current->ac.gemG.push_back(std::string(first,last));
	}
};
struct SettingsGrammar : public grammar<SettingsGrammar>
{
	template <typename ScannerT>
	struct definition
	{
		rule<ScannerT>  r, clientOption, clientOptionsSequence, ClientSettings;
		rule<ScannerT>  GUISettings, GUIOption, GUIOptionsSequence, AdvMapOptionsSequence, AdvMapOption;
		rule<ScannerT> GUIResolution, fname;
		definition(SettingsGrammar const& self)  
		{ 
			fname = lexeme_d[+(alnum_p | '.')];
			clientOption 
				= str_p("resolution=") >> (uint_p[assign_a(conf.cc.resx)] >> 'x' >> uint_p[assign_a(conf.cc.resy)] | eps_p[lerror("Wrong resolution!")])
				| str_p("port=") >> (uint_p[assign_a(conf.cc.port)] | eps_p[lerror("Wrong port!")])
				| str_p("bpp=") >> (uint_p[assign_a(conf.cc.bpp)] | eps_p[lerror("Wrong bpp!")])
				| str_p("localInformation=") >> (uint_p[assign_a(conf.cc.localInformation)] | eps_p[lerror("Wrong localInformation!")])
				| str_p("fullscreen=") >> (uint_p[assign_a(conf.cc.fullscreen)] | eps_p[lerror("Wrong fullscreen!")])
				| str_p("server=") >> ( ( +digit_p >> *('.' >> +digit_p) )[assign_a(conf.cc.server)]   | eps_p[lerror("Wrong server!")])
				| str_p("defaultAI=") >> ((+(anychar_p - ';'))[assign_a(conf.cc.defaultAI)] | eps_p[lerror("Wrong defaultAI!")])
				| (+(anychar_p - '}'))[lerror2("Unrecognized client option: ")]
				;
			clientOptionsSequence = *(clientOption >> (';' | eps_p[lerror("Semicolon lacking after client option!")]));
			ClientSettings = '{' >>  clientOptionsSequence >> '}';

			AdvMapOption 
				=	str_p("Buttons") >> ((ch_p('{') >> '}') | eps_p[lerror("Wrong Buttons!")])
				|	str_p("Minimap: ") >> 
						*(	
							"width=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::minimapW)]//[assign_a(current->ac.minimapW)] 
							  |	"height=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::minimapH)]
							  |	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::minimapX)]
							  |	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::minimapY)]
						 )
				| str_p("Statusbar:") >>
						*(	
							(	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::statusbarX)]
							  |	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::statusbarY)]
							  |	"graphic=" >> fname[SetAdventureStr(&AdventureMapConfig::statusbarG)]
							) 
						 )
				| str_p("ResDataBar:") >>
						*(	
							(	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resdatabarX)]
							|	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resdatabarY)]
							|	"offsetX=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resOffsetX)]
							|	"offsetY=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resOffsetY)]
							|	"resSpace=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resDist)]
							|	"resDateSpace=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::resDateDist)]
							  |	"graphic=" >> fname[SetAdventureStr(&AdventureMapConfig::resdatabarG)]
							) 
						 )
				| str_p("InfoBox:") >>
						*(	
							(	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::infoboxX)]
							  |	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::infoboxY)]
							) 
						 )
				| str_p("AdvMap:") >>
						*(	
							(   "x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::advmapX)]
							  | "y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::advmapY)]
							  | "width=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::advmapW)]
							  | "height=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::advmapH)]
							  | "smoothMove=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::smoothMove)]
							  | "puzzleSepia=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::puzzleSepia)]
							) 
						 )
				| str_p("background=") >> fname[SetAdventureStr(&AdventureMapConfig::mainGraphic)]
				| str_p("Button") >> (+(anychar_p-':'))[SetCurButton()] >> ':' >>
						*(	
							(	"x=" >> uint_p[SetButtonProp_a(&ButtonInfo::x)]
							  |	"y=" >> uint_p[SetButtonProp_a(&ButtonInfo::y)]
							  |	"playerColoured=" >> uint_p[SetButtonProp_a(&ButtonInfo::playerColoured)]
							  |	"graphic=" >> fname[SetButtonStr(&ButtonInfo::defName)]
							  | "additionalDefs=" >> ch_p('(') >> fname[AddDefForButton()] 
									>> *(',' >> fname[AddDefForButton()]) >> ')'
							) 
						 )
				 | str_p("HeroList:") >> 
						*(	
							(	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::hlistX)]
							  |	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::hlistY)]
							  |	"size=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::hlistSize)]
							  |	"movePoints=" >> fname[SetAdventureStr(&AdventureMapConfig::hlistMB)]
							  |	"manaPoints=" >> fname[SetAdventureStr(&AdventureMapConfig::hlistMN)]
							  |	"arrowUp=" >> fname[SetAdventureStr(&AdventureMapConfig::hlistAU)]
							  |	"arrowDown=" >> fname[SetAdventureStr(&AdventureMapConfig::hlistAD)]
							) 
						 )
				 | str_p("TownList:") >> 
						*(	
							(	"x=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::tlistX)]
							  |	"y=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::tlistY)]
							  |	"size=" >> uint_p[SetAdventureProp_a(&AdventureMapConfig::tlistSize)]
							  |	"arrowUp=" >> fname[SetAdventureStr(&AdventureMapConfig::tlistAU)]
							  |	"arrowDown=" >> fname[SetAdventureStr(&AdventureMapConfig::tlistAD)]
							) 
						 )
				 | str_p("gem") >> uint_p[var(gnb) = arg1] >> ':' >>
						*(	
							(	"x=" >> uint_p[bind(&setGem,1,_1)]
							  | "y=" >> uint_p[bind(&setGem,0,_1)]
							  | "graphic=" >> fname[AddGemName()]
							) 
						 )
				;
			AdvMapOptionsSequence = *(AdvMapOption >> (';' | eps_p[lerror("Semicolon lacking in advmapopt!")]));
			GUIResolution = (uint_p[assign_a(curRes.first)] >> 'x' >> uint_p[assign_a(curRes.second)])
								[dummy(&addGRes)];
			GUIOption = str_p("AdventureMap") >> ('{' >> AdvMapOptionsSequence >> '}' | eps_p[lerror("Wrong AdventureMap!")]);
			GUIOptionsSequence = *(GUIOption >> (';' | eps_p[lerror("Semicolon after GUIOption lacking!")]));
			GUISettings	= +(GUIResolution >> '{' >> GUIOptionsSequence >> '}');


			r	
				=	str_p("clientSettings") >> (ClientSettings | eps_p[lerror("Wrong clientSettings!")])
				>>	str_p("GUISettings") >> ('{' >> GUISettings >> '}' | eps_p[lerror("Wrong GUISettings!")]);
#ifdef BOOST_SPIRIT_DEBUG
			BOOST_SPIRIT_DEBUG_RULE(clientOption);
			BOOST_SPIRIT_DEBUG_RULE(clientOptionsSequence);
			BOOST_SPIRIT_DEBUG_RULE(ClientSettings);
			BOOST_SPIRIT_DEBUG_RULE(AdvMapOption);
			BOOST_SPIRIT_DEBUG_RULE(AdvMapOptionsSequence);
			BOOST_SPIRIT_DEBUG_RULE(GUIOption);
			BOOST_SPIRIT_DEBUG_RULE(GUIOptionsSequence);
			BOOST_SPIRIT_DEBUG_RULE(GUISettings);
			BOOST_SPIRIT_DEBUG_RULE(GUIResolution);
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

CConfigHandler::CConfigHandler(void)
{
}

CConfigHandler::~CConfigHandler(void)
{
}

void config::CConfigHandler::init()
{
	std::vector<char> settings;
	std::ifstream ifs("config/settings.txt");
	if(!ifs)
	{
		tlog1 << "Cannot open config/settings.txt !\n";
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
}

GUIOptions * config::CConfigHandler::go()
{
	return &guiOptions[std::pair<int,int>(cc.resx,cc.resy)];
}
