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
 * TriangularTerm.cpp
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#include "TriangularTerm.h"
namespace fl {

    TriangularTerm::TriangularTerm() :
    LinguisticTerm(), _b(0) {

    }

    TriangularTerm::TriangularTerm(const std::string& name, flScalar minimum,
            flScalar maximum) :
    LinguisticTerm(name, minimum, maximum), _b((minimum + maximum) / 2) {

    }

    TriangularTerm::TriangularTerm(const FuzzyOperator& fuzzy_op,
            const std::string& name, flScalar minimum, flScalar maximum) :
    LinguisticTerm(fuzzy_op, name, minimum, maximum), _b((minimum + maximum) / 2) {

    }

    TriangularTerm::~TriangularTerm() {

    }

    void TriangularTerm::setA(flScalar a) {
        setMinimum(a);
    }

    flScalar TriangularTerm::a() const {
        return minimum();
    }

    void TriangularTerm::setB(flScalar b) {
        this->_b = b;
    }

    flScalar TriangularTerm::b() const {
        return this->_b;
    }

    void TriangularTerm::setC(flScalar c) {
        setMaximum(c);
    }

    flScalar TriangularTerm::c() const {
        return maximum();
    }

    TriangularTerm* TriangularTerm::clone() const {
        return new TriangularTerm(*this);
    }

    flScalar TriangularTerm::membership(flScalar crisp) const {
        if (crisp > maximum() || crisp < minimum()) {
            return flScalar(0.0);
        }
        flScalar result = crisp < b() ? FuzzyOperation::Scale(a(), b(), crisp, 0, 1)
                : FuzzyOperation::Scale(b(), c(), crisp, 1, 0);
        return fuzzyOperator().modulation().execute(result, modulation());
    }

    LinguisticTerm::eMembershipFunction TriangularTerm::type() const {
        return MF_TRIANGULAR;
    }

    std::string TriangularTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Triangular (" << a() << " " << b() << " " << c() << ")";
        return ss.str();
    }

} // namespace fuzzy_lite
