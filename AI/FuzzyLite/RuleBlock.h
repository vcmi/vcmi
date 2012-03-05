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
 * RuleBlock.h
 *
 *  Created on: Jan 2, 2010
 *      Author: jcrada
 */

#ifndef FL_RULEBLOCK_H_
#define FL_RULEBLOCK_H_

#include "FuzzyOperator.h"
#include <vector>
#include <string>
namespace fl {

    class FuzzyRule;
    class RuleBlock {
    private:
        std::string _name;
        const FuzzyOperator* _fuzzy_operator;
        std::vector<FuzzyRule*> _rules;
    public:
        RuleBlock(const std::string& name = "");
        RuleBlock(const std::string& name, const FuzzyOperator& fuzzy_operator);
        virtual ~RuleBlock();

        virtual void setName(const std::string& name);
        virtual std::string name() const;

        virtual FuzzyRule* rule(int index) const;
        virtual void addRule(FuzzyRule* rule);
        virtual FuzzyRule* removeRule(int index);
        virtual int numberOfRules() const;

        virtual const FuzzyOperator& fuzzyOperator() const;
        virtual void setFuzzyOperator(const FuzzyOperator& fuzzy_op);

        virtual void fireRules();
        virtual void reset();

        virtual std::string toString() const;
    };


} // namespace fl

#endif /* FL_RULEBLOCK_H_ */
