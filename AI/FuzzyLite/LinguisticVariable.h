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
 * File:   LinguisticVariable.h
 * Author: jcrada
 *
 * Created on November 1, 2009, 1:56 PM
 */

#ifndef FL_LINGUISTICVARIABLE_H
#define	FL_LINGUISTICVARIABLE_H

#include <string>
#include <vector>

#include "LinguisticTerm.h"
#include "FuzzyExceptions.h"
#include "defs.h"
#include "CompoundTerm.h"

namespace fl {

    class LinguisticVariable {
    private:
        const FuzzyOperator* _fuzzy_operator;
        std::string _name;
        std::vector<LinguisticTerm*> _terms;
    protected:
        virtual int positionFor(const LinguisticTerm* term);
    public:
        LinguisticVariable();
        LinguisticVariable(const std::string& name);
        LinguisticVariable(const FuzzyOperator& fuzzy_op, const std::string& name);
        virtual ~LinguisticVariable();

        virtual std::string name() const;
        virtual void setName(const std::string& name);

        virtual void setFuzzyOperator(const FuzzyOperator& fuzzy_op);
        virtual const FuzzyOperator& fuzzyOperator() const;

        virtual void addTerm(LinguisticTerm* term);
        virtual LinguisticTerm* term(int index) const;
        virtual LinguisticTerm* term(const std::string& name) const;
        virtual int indexOf(const std::string& name) const;
        virtual LinguisticTerm* removeTerm(int index);
        virtual LinguisticTerm* removeTerm(const std::string& name);
        virtual LinguisticTerm* firstTerm() const;
        virtual LinguisticTerm* lastTerm() const;
        virtual bool isEmpty() const;
        virtual int numberOfTerms() const;
        virtual void createTerms(int number_of_terms, LinguisticTerm::eMembershipFunction mf,
                flScalar min, flScalar max, 
                const std::vector<std::string>& labels = std::vector<std::string>());

        virtual flScalar minimum() const;
        virtual flScalar maximum() const;

        virtual CompoundTerm compound() const;

        virtual std::string fuzzify(flScalar crisp) const;

        virtual LinguisticTerm* bestFuzzyApproximation(flScalar crisp);

        virtual std::string toString() const;

    };
}

#endif	/* FL_LINGUISTICVARIABLE_H */

