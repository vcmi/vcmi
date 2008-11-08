//#define BOOST_SPIRIT_DEBUG
#include "CConfigHandler.h"
#include <boost/bind.hpp>
#include <boost/spirit.hpp>
#include <fstream>
using namespace config;
using namespace boost::spirit;

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
//template <typename T, typename U>
//struct AssignInAll
//{
//	std::vector<T> &items;
//	U T::*pointer;
//	AssignInAll(std::vector<T> &Items, U T::*Pointer)
//		:items(Items),pointer(Pointer)
//	{}
//	void operator()(const U &as)
//	{
//		for(int i=0; i<items.size(); i++)
//			items[i].*pointer = U;
//	}
//};
struct SettingsGrammar : public grammar<SettingsGrammar>
{
	template <typename ScannerT>
	struct definition
	{
		rule<ScannerT>  r, clientOption, clientOptionsSequence;
		rule<ScannerT>  GUIOption, GUIOptionsSequence, AdvMapOptionsSequence, AdvMapOption;
		definition(SettingsGrammar const& self)  
		{ 
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
			clientOptionsSequence = *(clientOption >> (';' | eps_p[lerror("Semicolon lacking!")]));

			AdvMapOption 
				=	str_p("Buttons") >> ((ch_p('{') >> '}') | eps_p[lerror("Wrong Buttons!")])
				|	str_p("Minimap : ") >> 
						*(	
							(	"width=" >> uint_p[assign_a(conf.gc.ac.minimap.w)] 
							  |	"height=" >> uint_p[assign_a(conf.gc.ac.minimap.h)]
							  |	"x=" >> uint_p[assign_a(conf.gc.ac.minimap.x)]
							  |	"y=" >> uint_p[assign_a(conf.gc.ac.minimap.y)]
							) 
							>> *ch_p(',')
						 );
			AdvMapOptionsSequence = *(AdvMapOption >> (';' | eps_p[lerror("Semicolon lacking!")]));
			
			GUIOption = str_p("AdventureMap") >> (('{' >> AdvMapOptionsSequence >> '}') | eps_p[lerror("Wrong AdventureMap!")]);
			GUIOptionsSequence = *(GUIOption >> (';' | eps_p[lerror("Semicolon after GUIOption lacking!")]));
			r	= str_p("clientSettings") >> (('{' >> clientOptionsSequence >> '}') | eps_p[lerror("Wrong clientSettings!")])
				>> str_p("GUISettings") >> (('{' >> GUIOptionsSequence >> '}') | eps_p[lerror("Wrong GUISettings!")]);
			#ifdef BOOST_SPIRIT_DEBUG
				BOOST_SPIRIT_DEBUG_RULE(clientOption);
				BOOST_SPIRIT_DEBUG_RULE(clientOptionsSequence);
				BOOST_SPIRIT_DEBUG_RULE(AdvMapOption);
				BOOST_SPIRIT_DEBUG_RULE(AdvMapOptionsSequence);
				BOOST_SPIRIT_DEBUG_RULE(GUIOption);
				BOOST_SPIRIT_DEBUG_RULE(GUIOptionsSequence);
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