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
 * File:   DescriptiveAntecedent.h
 * Author: jcrada
 *
 * Created on March 3, 2010, 12:32 AM
 */

#ifndef FL_DESCRIPTIVEANTECEDENT_H
#define	FL_DESCRIPTIVEANTECEDENT_H

#include "FuzzyAntecedent.h"
#include "FuzzyOperator.h"
#include "InputLVar.h"
#include "Hedge.h"
#include "LinguisticTerm.h"
#include <vector>
#include <string>
#include "FuzzyEngine.h"
#include "FuzzyExceptions.h"

namespace fl {

    class DescriptiveAntecedent : public FuzzyAntecedent {
    public:

        enum OPERATION {
            O_NONE = ' ', O_AND = '&', O_OR = '*',
        };
    private:
        DescriptiveAntecedent* _left;
        DescriptiveAntecedent* _right;
        OPERATION _operation;
        std::vector<Hedge*> _hedges; //very
        const LinguisticTerm* _term; //high

    public:
        DescriptiveAntecedent();
        DescriptiveAntecedent(const FuzzyOperator& fuzzy_op);
        virtual ~DescriptiveAntecedent();

        virtual flScalar degreeOfTruth() const;
        
        virtual std::string toString() const;

        virtual void parse(const std::string& antecedent,
                const FuzzyEngine& engine) throw (ParsingException);

        virtual void setLeft(DescriptiveAntecedent* l_antecedent);
        virtual DescriptiveAntecedent* left() const;

        virtual void setRight(DescriptiveAntecedent* r_antecedent);
        virtual DescriptiveAntecedent* right() const;

        virtual void setOperation(OPERATION operation);
        virtual OPERATION operation() const;

        virtual void addHedge(Hedge* hedge);
        virtual int numberOfHedges() const;
        virtual Hedge* hedge(int index) const;

        virtual void setTerm(const LinguisticTerm* term);
        virtual const LinguisticTerm* term() const;
        
        virtual bool isTerminal() const;

//        static bool IsValid(const std::string& rule, const FuzzyEngine& engine);
        std::string Preprocess(const std::string& infix);
        std::string InfixToPostfix(const std::string& infix);

    };
}

#endif	/* FL_DESCRIPTIVEANTECEDENT_H */

