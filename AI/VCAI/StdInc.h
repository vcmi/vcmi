#pragma  once
#include "../../Global.h"
#include <cassert>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/type_traits.hpp>
#include "../../lib/AI_Base.h"
#include "../../CCallback.h"
#include "../../lib/CObjectHandler.h"

#include <boost/foreach.hpp>
#include "../../lib/CThreadHelper.h"
#include <boost/thread/tss.hpp>

#include "../../lib/VCMI_Lib.h"
#include "../../lib/CBuildingHandler.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CTownHandler.h"
#include "../../lib/CSpellHandler.h"
#include "../../lib/CObjectHandler.h"
#include "../../lib/Connection.h"
#include "../../lib/CGameState.h"
#include "../../lib/map.h"
#include "../../lib/NetPacks.h"
#include "../../lib/CondSh.h"
#include "../../lib/CStopWatch.h"

#include "Fuzzy.h"

#include <fstream>
#include <queue>

using boost::format;
using boost::str;


extern CLogger &aiLogger;

#define INDENT AILogger::Tab ___dummy_ind
#define PNLOG(txt) {int i = logger.lvl; while(i--) aiLogger << "\t"; aiLogger << txt << "\n";}
#define BNLOG(txt, formattingEls) {int i = logger.lvl; while(i--) aiLogger << "\t"; aiLogger << (boost::format(txt) % formattingEls) << "\n";}
//#define LOG_ENTRY PNLOG("Entered " __FUNCTION__)
#define LOG_ENTRY

