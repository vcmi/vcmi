/*   Copyright 2010 Juan Rada-Vilela

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
/* 
 * File:   FuzzyEngine.h
 * Author: jcrada
 *
 * Created on November 1, 2009, 4:51 PM
 */

#ifndef FL_FUZZYENGINE_H
#define	FL_FUZZYENGINE_H

#include "FuzzyOperator.h"
#include "InputLVar.h"
#include "OutputLVar.h"
#include "Hedge.h"
#include "RuleBlock.h"

#include "HedgeSet.h"

namespace fl {

    class FuzzyEngine {
    private:
        std::string _name;
        FuzzyOperator* _fuzzy_op;
        HedgeSet* _hedge_set;
        std::vector<InputLVar*> _input_lvars;
        std::vector<OutputLVar*> _output_lvars;
        std::vector<RuleBlock*> _rule_blocks;
    public:
        FuzzyEngine();
        FuzzyEngine(const std::string& name);
        FuzzyEngine(const std::string& name, FuzzyOperator& fuzzy_operator);
        virtual ~FuzzyEngine();

        virtual void setName(const std::string& name);
        virtual std::string name() const;

        virtual void process(bool clear = true);
        virtual void process(int ruleblock, bool clear = true);

        virtual void reset();

        virtual void addInputLVar(InputLVar* lvar);
        virtual InputLVar* inputLVar(int index) const ;
        virtual InputLVar* inputLVar(const std::string& name) const;
        virtual InputLVar* removeInputLVar(int index) ;
        virtual InputLVar* removeInputLVar(const std::string& name);
        virtual int indexOfInputLVar(const std::string& name) const;
        virtual int numberOfInputLVars() const;

        virtual void addOutputLVar(OutputLVar* lvar);
        virtual OutputLVar* outputLVar(int index) const ;
        virtual OutputLVar* outputLVar(const std::string& name) const;
        virtual OutputLVar* removeOutputLVar(int index) ;
        virtual OutputLVar* removeOutputLVar(const std::string& name);
        virtual int indexOfOutputLVar(const std::string& name) const;
        virtual int numberOfOutputLVars() const;

        virtual HedgeSet& hedgeSet() const;

        virtual void addRuleBlock(RuleBlock* ruleblock);
        virtual RuleBlock* removeRuleBlock(int index);
        virtual RuleBlock* ruleBlock(int index) const;
        virtual RuleBlock* ruleBlock(const std::string& name) const;
        virtual int numberOfRuleBlocks() const;

        virtual void setFuzzyOperator(FuzzyOperator& fuzzy_operator);
        virtual FuzzyOperator& fuzzyOperator() const;

        virtual void setInput(const std::string& input_lvar, flScalar value);
        virtual flScalar output(const std::string& output_lvar) const;

        virtual std::string fuzzyOutput(const std::string& output_lvar) const;

        virtual std::string toString() const;


    };
}

#endif	/* FL_FUZZYENGINE_H */

