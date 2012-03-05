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
 * CompoundTerm.h
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */


#ifndef FL_COMPOUNDTERM_H_
#define FL_COMPOUNDTERM_H_

#include "LinguisticTerm.h"

#include <vector>
namespace fl {

    class CompoundTerm : public LinguisticTerm {
    private:
        std::vector<LinguisticTerm*> _terms;

    public:
        CompoundTerm();
        CompoundTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        CompoundTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~CompoundTerm();

        virtual void addTerm(const LinguisticTerm& term);
        virtual const LinguisticTerm& term(int index) const;
        virtual void removeTerm(int index);
        virtual int numberOfTerms() const;

        virtual void clear();

        virtual CompoundTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;
        virtual eMembershipFunction type() const;

        virtual std::string toString() const;

    };

} // namespace fuzzy_lite
#endif /* FL_COMPOUNDTERM_H_ */
