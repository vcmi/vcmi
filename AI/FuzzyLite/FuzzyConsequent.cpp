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

#include "FuzzyConsequent.h"


namespace fl {

    FuzzyConsequent::FuzzyConsequent() : _output_lvar(NULL), _weight(1.0) {

    }

    FuzzyConsequent::~FuzzyConsequent() {

    }

    void FuzzyConsequent::setOutputLVar(OutputLVar* output_lvar) {
        this->_output_lvar = output_lvar;
    }

    OutputLVar* FuzzyConsequent::outputLVar() const {
        return this->_output_lvar;
    }

    void FuzzyConsequent::setWeight(flScalar weight) {
        this->_weight = weight;
    }

    flScalar FuzzyConsequent::weight() const {
        return this->_weight;
    }


}
