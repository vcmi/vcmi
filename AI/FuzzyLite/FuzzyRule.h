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
 * File:   FuzzyRule.h
 * Author: jcrada
 *
 * Created on November 5, 2009, 11:11 AM
 */

#ifndef FL_FUZZYRULE_H
#define	FL_FUZZYRULE_H

#include "FuzzyOperator.h"
#include "FuzzyAntecedent.h"
#include "FuzzyConsequent.h"
#include <string>
#include <vector>
#include "flScalar.h"
#include "FuzzyExceptions.h"

namespace fl {
    class FuzzyRule {
    public:
        static const std::string FR_IF;
        static const std::string FR_IS;
        static const std::string FR_THEN;
        static const std::string FR_AND;
        static const std::string FR_OR;
        static const std::string FR_WITH;

    private:
        const FuzzyOperator* _fuzzy_operator;
        FuzzyAntecedent* _antecedent;
        std::vector<FuzzyConsequent*> _consequents;

    public:
        FuzzyRule();
        FuzzyRule(const FuzzyOperator& fuzzy_op);
        virtual ~FuzzyRule();

        virtual void setFuzzyOperator(const FuzzyOperator& fuzzy_op);
        virtual const FuzzyOperator& fuzzyOperator() const;

        virtual FuzzyAntecedent* antecedent() const;
        virtual void setAntecedent(FuzzyAntecedent* antecedent);

        virtual FuzzyConsequent* consequent(int index) const;
        virtual void addConsequent(FuzzyConsequent* consequent);
        virtual int numberOfConsequents() const;

        virtual bool isValid() const;

        virtual flScalar firingStrength() const;

        virtual void fire(flScalar strength);

        virtual std::string toString() const;

        virtual void parse(const std::string& rule, const FuzzyEngine& engine) = 0;

        static void ExtractFromRule(const std::string& rule, std::string& antecedent, std::string& consequent);

    };
}

#endif	/* _FUZZYRULE_H */

