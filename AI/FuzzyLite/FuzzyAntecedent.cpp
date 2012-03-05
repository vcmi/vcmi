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



#include "FuzzyAntecedent.h"

namespace fl {

    FuzzyAntecedent::FuzzyAntecedent() : _input_lvar(NULL) {
        setFuzzyOperator(FuzzyOperator::DefaultFuzzyOperator());
    }

    FuzzyAntecedent::FuzzyAntecedent(const FuzzyOperator& fuzzy_op)
    : _fuzzy_operator(&fuzzy_op), _input_lvar(NULL) {

    }

    FuzzyAntecedent::~FuzzyAntecedent() {

    }

    void FuzzyAntecedent::setFuzzyOperator(const FuzzyOperator& fuzzy_op) {
        this->_fuzzy_operator = &fuzzy_op;
    }

    const FuzzyOperator& FuzzyAntecedent::fuzzyOperator() const {
        return *this->_fuzzy_operator;
    }

    void FuzzyAntecedent::setInputLVar(const InputLVar* input_lvar) {
        this->_input_lvar = input_lvar;
    }

    const InputLVar* FuzzyAntecedent::inputLVar() const {
        return this->_input_lvar;
    }


}
