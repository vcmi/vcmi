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
 * FuzzyRule.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#include "FuzzyRule.h"
#include <stack>
#include <iosfwd>
#include "StrOp.h"
namespace fl {

    const std::string FuzzyRule::FR_IF = "if";
    const std::string FuzzyRule::FR_IS = "is";
    const std::string FuzzyRule::FR_THEN = "then";
    const std::string FuzzyRule::FR_AND = "and";
    const std::string FuzzyRule::FR_OR = "or";
    const std::string FuzzyRule::FR_WITH = "with";

    FuzzyRule::FuzzyRule() : _antecedent(NULL) {
        setFuzzyOperator(fuzzyOperator().DefaultFuzzyOperator());
    }

    FuzzyRule::FuzzyRule(const FuzzyOperator& fuzzy_op)
    : _fuzzy_operator(&fuzzy_op), _antecedent(NULL) {

    }

    FuzzyRule::~FuzzyRule() {
        if (_antecedent) {
            delete _antecedent;
        }
        FuzzyConsequent* fconsequent = NULL;
        for (int i = numberOfConsequents() - 1; i >= 0; --i) {
            fconsequent = consequent(i);
            _consequents.pop_back();
            delete fconsequent;
        }
    }

    void FuzzyRule::setFuzzyOperator(const FuzzyOperator& fuzzy_op){
        this->_fuzzy_operator = &fuzzy_op;
        if (antecedent()){
            antecedent()->setFuzzyOperator(fuzzy_op);
        }
        //Todo: check if consequents must have a fuzzyoperator
        for (int i =0 ; i  < numberOfConsequents() ;++i){
        }
    }

    const FuzzyOperator& FuzzyRule::fuzzyOperator() const{
        return *this->_fuzzy_operator;
    }
    void FuzzyRule::setAntecedent(FuzzyAntecedent* antecedent) {
        this->_antecedent = antecedent;
    }

    FuzzyAntecedent* FuzzyRule::antecedent() const {
        return this->_antecedent;
    }

    FuzzyConsequent* FuzzyRule::consequent(int index) const {
        return this->_consequents[index];
    }

    void FuzzyRule::addConsequent(FuzzyConsequent* consequent) {
        this->_consequents.push_back(consequent);
    }

    int FuzzyRule::numberOfConsequents() const {
        return this->_consequents.size();
    }

    bool FuzzyRule::isValid() const {
        return this->_antecedent && this->_consequents.size() > 0;
    }

    flScalar FuzzyRule::firingStrength() const {
        if (antecedent()) {
            return antecedent()->degreeOfTruth();
        }
        throw InvalidArgumentException(FL_AT,"Antecedent was not parsed correctly");
    }

    void FuzzyRule::fire(flScalar strength) {
        if (!FuzzyOperation::IsEq(strength, flScalar(0.0))) {
//            FL_LOG("DoT: " << strength << " - Fired: " << toString());
            for (int i = 0; i < numberOfConsequents(); ++i) {
                consequent(i)->execute(strength);
            }
        }
    }

    std::string FuzzyRule::toString() const {
        std::stringstream ss;
        ss << FR_IF << " " << _antecedent->toString() << " " << FR_THEN;
        for (int i = 0; i < numberOfConsequents(); ++i) {
            ss << " " + consequent(i)->toString() + (i < numberOfConsequents() - 1 ? FR_AND : "");
        }
        return ss.str();
    }

    void FuzzyRule::ExtractFromRule(const std::string& rule, std::string& antecedent, std::string& consequent) {
        std::vector<std::string> parse = StrOp::SplitByWord(rule, FR_IF);
        if (parse.size() != 1) {
            antecedent = "invalid";
            consequent = "invalid";
            return;
        }
        parse = StrOp::SplitByWord(parse[0], FR_THEN);
        if (parse.size() != 2){
            antecedent = "invalid";
            consequent = "invalid";
            return;
        }
        antecedent = parse[0];
        consequent = parse[1];
    }

}// namespace fuzzy_lite
