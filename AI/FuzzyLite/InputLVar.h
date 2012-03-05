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
 * File:   InputLVar.h
 * Author: jcrada
 *
 * Created on November 1, 2009, 2:25 PM
 */

#ifndef FL_INPUTLVAR_H
#define	FL_INPUTLVAR_H

#include "LinguisticVariable.h"

namespace fl {

    class InputLVar : public LinguisticVariable {
    private:
        flScalar _input;
    public:
        InputLVar();
        InputLVar(const std::string& name);
        virtual ~InputLVar();

        virtual void setInput(flScalar input);
        virtual flScalar input() const;

    };
}

#endif	/* FL_INPUTLVAR_H */

