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
 * File:   FuzzyConsequent.h
 * Author: jcrada
 *
 * Created on March 2, 2010, 7:05 PM
 */

#ifndef FL_FUZZYCONSEQUENT_H
#define	FL_FUZZYCONSEQUENT_H

#include "OutputLVar.h"
#include <string>
#include "FuzzyExceptions.h"
#include "FuzzyEngine.h"

namespace fl {

    class FuzzyConsequent {
    private:
        OutputLVar* _output_lvar;
        flScalar _weight;
    public:
        FuzzyConsequent();
        virtual ~FuzzyConsequent();

        virtual void setOutputLVar(OutputLVar* output_lvar);
        virtual OutputLVar* outputLVar() const;

        virtual void setWeight(flScalar weight);
        virtual flScalar weight() const;

        virtual void parse(const std::string& consequent,
                const FuzzyEngine& engine) throw (ParsingException) = 0;

        virtual void execute(flScalar degree) = 0;

        virtual std::string toString() const = 0;

        static void main(int argc, char** argv);

    };

}

#endif	/* FL_FUZZYCONSEQUENT_H */

