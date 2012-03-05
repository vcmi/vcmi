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
 * File:   FuzzyAntecedent.h
 * Author: jcrada
 *
 * Created on March 3, 2010, 12:28 AM
 */

#ifndef FL_FUZZYANTECEDENT_H
#define	FL_FUZZYANTECEDENT_H

#include "flScalar.h"
#include <string>
#include "FuzzyExceptions.h"
#include "FuzzyOperator.h"
#include "InputLVar.h"
#include "FuzzyEngine.h"

namespace fl {
    class FuzzyAntecedent {
    private:
        const FuzzyOperator* _fuzzy_operator;
        const InputLVar* _input_lvar; //e.g. power
    public:
        FuzzyAntecedent();
        FuzzyAntecedent(const FuzzyOperator& fuzzy_op);
        virtual ~FuzzyAntecedent();

        virtual void setFuzzyOperator(const FuzzyOperator& fuzzy_op);
        virtual const FuzzyOperator& fuzzyOperator() const;

        virtual void setInputLVar(const InputLVar* input_lvar);
        virtual const InputLVar* inputLVar() const;

        virtual flScalar degreeOfTruth() const = 0;
        virtual std::string toString() const = 0;

        virtual void parse(const std::string& antecedent,
                const FuzzyEngine& engine) throw (ParsingException) = 0;
    };
}

#endif	/* FL_FUZZYANTECEDENT_H */

