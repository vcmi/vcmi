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
 * File:   TakagiSugenoConsequent.h
 * Author: jcrada
 *
 * Created on March 4, 2010, 12:18 AM
 */

#ifndef FL_TAKAGISUGENOCONSEQUENT_H
#define	FL_TAKAGISUGENOCONSEQUENT_H

#include "FuzzyConsequent.h"

#include "InfixToPostfix.h"

namespace fl {

    class TakagiSugenoConsequent : public FuzzyConsequent {
    private:
        const FuzzyEngine* _fuzzy_engine;
        std::string _consequent;
        std::string _postfix_function;
        InfixToPostfix _infix2postfix;
    protected:
        const FuzzyEngine& fuzzyEngine() const;
        void setFuzzyEngine(const FuzzyEngine& fuzzy_engine);

        std::string consequent() const;
        void setConsequent(const std::string& consequent);

        std::string postfixFunction() const;
        void setPostfixFunction(const std::string& postfix);

    public:
        TakagiSugenoConsequent();
        virtual ~TakagiSugenoConsequent();

        virtual void parse(const std::string& consequent,
                const FuzzyEngine& engine) throw (ParsingException);

        virtual void execute(flScalar degree);

        virtual std::string toString() const;

        static void main(int args, char** argv);

    };
}

#endif	/* FL_TAKAGISUGENOCONSEQUENT_H */

