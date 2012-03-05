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
 * TrapezoidalTerm.h
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#ifndef FL_TRAPEZOIDALTERM_H_
#define FL_TRAPEZOIDALTERM_H_

#include "LinguisticTerm.h"

namespace fl {

    class TrapezoidalTerm : public LinguisticTerm {
    private:
        flScalar _b;
        flScalar _c;
    public:

        TrapezoidalTerm();
        TrapezoidalTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        TrapezoidalTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~TrapezoidalTerm();

        virtual void setA(flScalar a);
        virtual flScalar a() const;

        virtual void setB(flScalar b);
        virtual flScalar b() const;

        virtual void setC(flScalar c);
        virtual flScalar c() const;

        virtual void setD(flScalar d);
        virtual flScalar d() const;

        virtual TrapezoidalTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;

        virtual eMembershipFunction type() const;

        virtual std::string toString() const;
    };

} // namespace fuzzy_lite
#endif /* FL_TRAPEZOIDALTERM_H_ */
