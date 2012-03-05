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
#include "InputLVar.h"

namespace fl {

    InputLVar::InputLVar() : LinguisticVariable(), _input(flScalar(0.0)) {

    }

    InputLVar::InputLVar(const std::string& name) : LinguisticVariable(name),
    _input(flScalar(0.0)) {

    }

    InputLVar::~InputLVar() {
    }

    void InputLVar::setInput(flScalar input) {
        this->_input = input;
    }

    flScalar InputLVar::input() const {
        return this->_input;
    }

}
