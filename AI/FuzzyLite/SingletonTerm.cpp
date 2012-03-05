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
 * SingletonTerm.cpp
 *
 *  Created on: Jan 7, 2010
 *      Author: jcrada
 */

#include "SingletonTerm.h"

namespace fl {

    SingletonTerm::SingletonTerm() :
    LinguisticTerm() {

    }

    SingletonTerm::SingletonTerm(const std::string& name, flScalar value) :
    LinguisticTerm(name, value - 5 * FL_EPSILON, value + 5 * FL_EPSILON), _value(value) {

    }

    SingletonTerm::SingletonTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            flScalar value) :
    LinguisticTerm(fuzzy_op, name, value - 5 * FL_EPSILON, value + 5 * FL_EPSILON), _value(
    value) {

    }

    SingletonTerm::~SingletonTerm() {

    }

    void SingletonTerm::setValue(flScalar value) {
        this->_value = value;
        setMinimum(value - 5 * FL_EPSILON);
        setMaximum(value + 5 * FL_EPSILON);
    }

    flScalar SingletonTerm::value() const {
        return this->_value;
    }

    SingletonTerm* SingletonTerm::clone() const {
        return new SingletonTerm(*this);
    }

    flScalar SingletonTerm::membership(flScalar crisp) const {
        return fuzzyOperator().modulation().execute(modulation(), FuzzyOperation::IsEq(crisp, value())
                ? flScalar(1.0) : flScalar(0.0));
    }

    LinguisticTerm::eMembershipFunction SingletonTerm::type() const {
        return MF_SINGLETON;
    }

    std::string SingletonTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Singleton (" << value() << ")";
        return ss.str();
    }

} // namespace fl
