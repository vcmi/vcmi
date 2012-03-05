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
 * RectangularTerm.cpp
 *
 *  Created on: Jan 7, 2010
 *      Author: jcrada
 */

#include "RectangularTerm.h"

namespace fl {

    RectangularTerm::RectangularTerm() :
    LinguisticTerm() {

    }

    RectangularTerm::RectangularTerm(const std::string& name, flScalar minimum,
            flScalar maximum) :
    LinguisticTerm(name, minimum, maximum) {

    }

    RectangularTerm::RectangularTerm(const FuzzyOperator& fuzzy_op,
            const std::string& name, flScalar minimum, flScalar maximum) :
    LinguisticTerm(fuzzy_op, name, minimum, maximum) {

    }

    RectangularTerm::~RectangularTerm() {

    }

    RectangularTerm* RectangularTerm::clone() const {
        return new RectangularTerm(*this);
    }

    flScalar RectangularTerm::membership(flScalar crisp) const {
        if (crisp > maximum() || crisp < minimum()) {
            return flScalar(0.0);
        }
        return fuzzyOperator().modulation().execute(1.0, modulation());
    }

    LinguisticTerm::eMembershipFunction RectangularTerm::type() const {
        return MF_RECTANGULAR;
    }

    std::string RectangularTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Rectangular (" << minimum() << " " << maximum() << ")";
        return ss.str();
    }

} // namespace fl
