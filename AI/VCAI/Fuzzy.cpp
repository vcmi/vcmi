#include "StdInc.h"
#include "Fuzzy.h"
#include "../../lib/CObjectHandler.h"
#include "AreaCentroidAlgorithm.cpp"
#include "CompoundTerm.cpp"
#include "DescriptiveAntecedent.cpp"
#include "FuzzyEngine.cpp"
#include "FuzzyAnd.cpp"
#include "FuzzyOr.cpp"
#include "InputLVar.cpp"
#include "OutputLVar.cpp"
#include "FuzzyAntecedent.cpp"
#include "FuzzyConsequent.cpp"
#include "FuzzyDefuzzifier.cpp"
#include "FuzzyModulation.cpp"
#include "FuzzyOperator.cpp"
#include "FuzzyOperation.cpp"
#include "FuzzyException.cpp"
#include "FuzzyExceptions.cpp"
#include "FuzzyRule.cpp"
#include "HedgeSet.cpp"
#include "Hedge.cpp"
#include "SingletonTerm.cpp"
#include "TrapezoidalTerm.cpp"
#include "TriangularTerm.cpp"
#include "LinguisticTerm.cpp"
#include "LinguisticVariable.cpp"
#include "RuleBlock.cpp"
#include "ShoulderTerm.cpp"
#include "StrOp.cpp"
#include "MamdaniRule.cpp"
#include "MamdaniConsequent.cpp"

/*
 * Fuzzy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
*/

class BankConfig;
class FuzzyEngine;
class InputLVar;

FuzzyHelper fh;

ui64 evaluateBankConfig (BankConfig * bc)
{
	ui64 danger = 0;
	BOOST_FOREACH (auto opt, bc->guards)
	{
		danger += VLC->creh->creatures[opt.first]->fightValue * opt.second;
	}
	return danger;
}

FuzzyHelper::FuzzyHelper()
{
	try
	{
		bankInput = new fl::InputLVar("BankInput");
		bankDanger = new fl::OutputLVar("BankDanger");
		bankInput->addTerm(new fl::SingletonTerm ("SET"));

		engine.addRuleBlock (&ruleBlock); //have to be added before the rules are parsed
		engine.addInputLVar (bankInput);
		engine.addOutputLVar (bankDanger);
		for (int i = 0; i < 4; ++i)
		{
			bankDanger->addTerm(new fl::TriangularTerm ("Bank" + boost::lexical_cast<std::string>(i), 0, 1));
			ruleBlock.addRule(new fl::MamdaniRule("if BankInput is SET then BankDanger is Bank" + boost::lexical_cast<std::string>(i), engine));
		}
	}
	catch (fl::FuzzyException fe)
	{
		tlog1 << fe.name() << ": " << fe.message() << '\n';
	}
}

ui64 FuzzyHelper::estimateBankDanger (int ID)
{
	std::vector <ConstTransitivePtr<BankConfig>> & configs = VLC->objh->banksInfo[ID];
	ui64 val = INFINITY;;
	switch (configs.size())
	{
		case 4:
			try
			{
				int bankVal;
				for (int i = 0; i < 4; ++i)
				{
					bankVal = evaluateBankConfig (VLC->objh->banksInfo[ID][i]);
					bankDanger->term("Bank" + boost::lexical_cast<std::string>(i))->setMinimum(bankVal * 0.5f);
					bankDanger->term("Bank" + boost::lexical_cast<std::string>(i))->setMaximum(bankVal * 1.5f);
				}
				//comparison purposes
				//int averageValue = (evaluateBankConfig (VLC->objh->banksInfo[ID][0]) + evaluateBankConfig (VLC->objh->banksInfo[ID][3])) * 0.5;
				dynamic_cast<fl::SingletonTerm*>(bankInput->term("SET"))->setValue(0.5);
				bankInput->setInput (0.5);
				engine.process();
				val = bankDanger->output().defuzzify(); //some expected value of this bank
			}
			catch (fl::FuzzyException fe)
			{
				tlog1 << fe.name() << ": " << fe.message() << '\n';
			}
			break;
		case 1: //rare case - Pyramid
			val = evaluateBankConfig (VLC->objh->banksInfo[ID][0]);
			break;
		default:
			tlog3 << ("Uhnandled bank config!\n");
	}
	return val;

}