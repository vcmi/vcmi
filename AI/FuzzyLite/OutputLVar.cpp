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
#include "OutputLVar.h"

namespace fl {

    OutputLVar::OutputLVar() : LinguisticVariable(),
    _output(new CompoundTerm("output")){

    }
    OutputLVar::OutputLVar(const std::string& name) : LinguisticVariable(name),
    _output(new CompoundTerm("output")) {

    }

    OutputLVar::~OutputLVar() {
        delete _output;
    }

    CompoundTerm& OutputLVar::output() const {
        return *this->_output;
    }

}
