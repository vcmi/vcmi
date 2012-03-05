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
#include "RuleBlock.h"
#include "FuzzyRule.h"
#include <iosfwd> 

namespace fl {

    RuleBlock::RuleBlock(const std::string& name)
    : _name(name) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    RuleBlock::RuleBlock(const std::string& name, const FuzzyOperator& fuzzy_operator)
    : _name(name), _fuzzy_operator(&fuzzy_operator) {

    }

    RuleBlock::~RuleBlock() {
        reset();
    }

    void RuleBlock::setName(const std::string& name) {
        this->_name = name;
    }

    std::string RuleBlock::name() const {
        return this->_name;
    }

    FuzzyRule* RuleBlock::rule(int index) const {
        return this->_rules[index];
    }

    void RuleBlock::addRule(FuzzyRule* rule) {
        this->_rules.push_back(rule);
    }

    FuzzyRule* RuleBlock::removeRule(int index) {
        FuzzyRule* result = rule(index);
        this->_rules.erase(this->_rules.begin() + index);
        return result;
    }

    int RuleBlock::numberOfRules() const {
        return this->_rules.size();
    }

    const FuzzyOperator& RuleBlock::fuzzyOperator() const {
        return *this->_fuzzy_operator;
    }

    void RuleBlock::setFuzzyOperator(const FuzzyOperator& fuzzy_op) {
        this->_fuzzy_operator = &fuzzy_op;
        for (int i = 0; i < numberOfRules(); ++i) {
            rule(i)->setFuzzyOperator(fuzzy_op);
        }
    }

    void RuleBlock::fireRules() {
        for (int i = 0; i < numberOfRules(); ++i) {
            rule(i)->fire(rule(i)->firingStrength());
        }
    }

    void RuleBlock::reset() {
        for (int i = numberOfRules() - 1; i >= 0; --i) {
            delete removeRule(i);
        }
    }

    std::string RuleBlock::toString() const{
        std::stringstream ss;
        ss <<  "RULEBLOCK " << name() << "\n";
        for (int i = 0 ; i < numberOfRules() ; ++i){
            ss << "  RULE " << i + 1 << ": " << rule(i)->toString() << ";\n";
        }
        ss << "END_RULEBLOCK";
        return ss.str();
    }

}
