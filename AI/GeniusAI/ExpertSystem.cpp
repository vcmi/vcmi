#include "GeneralAI.h"
#include "../../CCallback.h"
#include "ExpertSystem.h"

/*
 * ExpertSystem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template <typename ruleType, typename facts> void ExpertSystemShell<ruleType, facts>::DataDrivenReasoning(runType type)
{
	switch (type)
	{
		case ANY_GOAL: //first produced decision ends reasoning
		{
			for (std::list<typename facts>::iterator it = factList.begin(); it != factList.end(); it++)
			{};
		}
		break;
	}
}