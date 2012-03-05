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
#include "TakagiSugenoTerm.h"

namespace fl {

    TakagiSugenoTerm::TakagiSugenoTerm() : SingletonTerm() {

    }

    TakagiSugenoTerm::TakagiSugenoTerm(const std::string& name, flScalar value,
            flScalar weight)
    : SingletonTerm(name, value), _weight(weight) {

    }

    TakagiSugenoTerm::TakagiSugenoTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            flScalar value, flScalar weight)
    : SingletonTerm(fuzzy_op, name, value), _weight(weight) {

    }

    TakagiSugenoTerm::~TakagiSugenoTerm() {

    }

    void TakagiSugenoTerm::setWeight(flScalar weight) {
        this->_weight = weight;
    }

    flScalar TakagiSugenoTerm::weight() const {
        return this->_weight;
    }

    TakagiSugenoTerm* TakagiSugenoTerm::clone() const {
        return new TakagiSugenoTerm(*this);
    }

    LinguisticTerm::eMembershipFunction TakagiSugenoTerm::type() const {
        return MF_TAKAGI_SUGENO;
    }

    std::string TakagiSugenoTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "TakagiSugeno (" << value() << " " << weight() << ")";
        return ss.str();
    }
}
