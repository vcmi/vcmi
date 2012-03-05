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
 * File:   TakagiSugenoTerm.h
 * Author: jcrada
 *
 * Created on March 10, 2010, 7:28 PM
 */

#ifndef FL_TAKAGISUGENOTERM_H
#define	FL_TAKAGISUGENOTERM_H

#include "SingletonTerm.h"

namespace fl{
    class TakagiSugenoTerm : public SingletonTerm{
    private:
        flScalar _weight;
    public:
        TakagiSugenoTerm();
        TakagiSugenoTerm(const std::string& name, flScalar value = NAN, flScalar weight = 1.0);
        TakagiSugenoTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar value = NAN, flScalar weight = 1.0);
        virtual ~TakagiSugenoTerm();

        virtual void setWeight(flScalar weight);
        virtual flScalar weight() const;

        virtual TakagiSugenoTerm* clone() const;
        virtual eMembershipFunction type() const;
        virtual std::string toString() const;
    };
}

#endif	/* FL_TAKAGISUGENOTERM_H */

