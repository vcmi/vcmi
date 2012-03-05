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
 * TrapezoidalTerm.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#include "TrapezoidalTerm.h"
namespace fl {

    TrapezoidalTerm::TrapezoidalTerm() :
    LinguisticTerm(), _b(0), _c(0) {

    }

    TrapezoidalTerm::TrapezoidalTerm(const std::string& name, flScalar minimum,
            flScalar maximum) :
    LinguisticTerm(name, minimum, maximum), _b(0), _c(0) {
        setB(minimum + (maximum - minimum) * 1 / 5);
        setC(minimum + (maximum - minimum) * 4 / 5);
    }

    TrapezoidalTerm::TrapezoidalTerm(const FuzzyOperator& fuzzy_op,
            const std::string& name, flScalar minimum, flScalar maximum) :
    LinguisticTerm(fuzzy_op, name, minimum, maximum), _b(0), _c(0) {
        setB(minimum + (maximum - minimum) * 1 / 5);
        setC(minimum + (maximum - minimum) * 4 / 5);
    }

    TrapezoidalTerm::~TrapezoidalTerm() {

    }

    void TrapezoidalTerm::setA(flScalar a) {
        setMinimum(a);
    }

    flScalar TrapezoidalTerm::a() const {
        return minimum();
    }

    void TrapezoidalTerm::setB(flScalar b) {
        this->_b = b;
    }

    flScalar TrapezoidalTerm::b() const {
        return this->_b;
    }

    void TrapezoidalTerm::setC(flScalar c) {
        this->_c = c;
    }

    flScalar TrapezoidalTerm::c() const {
        return this->_c;
    }

    void TrapezoidalTerm::setD(flScalar d) {
        setMaximum(d);
    }

    flScalar TrapezoidalTerm::d() const {
        return maximum();
    }

    TrapezoidalTerm* TrapezoidalTerm::clone() const {
        return new TrapezoidalTerm(*this);
    }

    flScalar TrapezoidalTerm::membership(flScalar crisp) const {
        if (crisp < minimum() || crisp > maximum()) {
            return flScalar(0);
        }
        if (crisp < b()) {
            return fuzzyOperator().modulation().execute(modulation(),
                    FuzzyOperation::Scale(a(), b(), crisp, 0, 1));
        }
        if (crisp < c()) {
            return fuzzyOperator().modulation().execute(modulation(), flScalar(1.0));
        }
        if (crisp < d()) {
            return fuzzyOperator().modulation().execute(modulation(),
                    FuzzyOperation::Scale(c(), d(), crisp, 1, 0));
        }
        return flScalar(0.0);
    }

    LinguisticTerm::eMembershipFunction TrapezoidalTerm::type() const {
        return MF_TRAPEZOIDAL;
    }

    std::string TrapezoidalTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Trapezoidal (" << a() << " " << b() << " " << c() << " " << d() << ")";
        return ss.str();
    }

} // namespace fuzzy_lite
