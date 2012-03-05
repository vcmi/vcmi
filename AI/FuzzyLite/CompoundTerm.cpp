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
 * CompoundTerm.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#include "CompoundTerm.h"

namespace fl {

    CompoundTerm::CompoundTerm() :
    LinguisticTerm() {

    }

    CompoundTerm::CompoundTerm(const std::string& name, flScalar minimum,
            flScalar maximum) :
    LinguisticTerm(name, minimum, maximum) {

    }

    CompoundTerm::CompoundTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            flScalar minimum, flScalar maximum) :
    LinguisticTerm(fuzzy_op, name, minimum, maximum) {

    }

    CompoundTerm::~CompoundTerm() {
    	clear();
    }

    void CompoundTerm::addTerm(const LinguisticTerm& term) {
        _terms.push_back(term.clone());
        if (term.minimum() < minimum() || isinf(minimum())) {
            setMinimum(term.minimum());
        }
        if (term.maximum() > maximum() || isinf(maximum())) {
            setMaximum(term.maximum());
        }
    }

    const LinguisticTerm& CompoundTerm::term(int index) const {
        return *this->_terms[index];
    }

    void CompoundTerm::removeTerm(int index){
        LinguisticTerm* t = this->_terms[index];
        this->_terms.erase(this->_terms.begin() + index);
        delete t;
    }

    int CompoundTerm::numberOfTerms() const{
        return this->_terms.size();
    }

    void CompoundTerm::clear() {
        for (size_t i = 0; i < _terms.size(); ++i) {
            delete _terms[i];
        }
        _terms.clear();
        setMinimum(-INFINITY);
        setMaximum(INFINITY);
    }

    CompoundTerm* CompoundTerm::clone() const {
    	CompoundTerm* result = new CompoundTerm;
    	for (int i = 0 ; i < numberOfTerms(); ++i){
    		result->addTerm(term(i));
    	}
        return result;
    }

    flScalar CompoundTerm::membership(flScalar crisp) const {
        flScalar max = flScalar(0);
        for (size_t i = 0; i < _terms.size(); ++i) {
            max = fuzzyOperator().aggregation().execute(max, _terms[i]->membership(crisp));
        }
        return fuzzyOperator().modulation().execute(modulation(), max);
    }

    LinguisticTerm::eMembershipFunction CompoundTerm::type() const {
        return MF_COMPOUND;
    }

    std::string CompoundTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Compound (";
        for (size_t i = 0; i < _terms.size(); ++i) {
            ss << _terms[i]->toString();
            if (i < _terms.size() -1 ) ss << " + ";
        }
        ss << ")";
        return ss.str();
    }

} // namespace fuzzy_lite
