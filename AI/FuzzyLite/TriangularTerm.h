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
 * TriangularTerm.h
 *
 *  Created on: Dec 13, 2009
 *      Author: jcrada
 */

#ifndef FL_TRIANGULARTERM_H_
#define FL_TRIANGULARTERM_H_

#include "LinguisticTerm.h"

namespace fl {

    class TriangularTerm : public LinguisticTerm {
    private:
        flScalar _b;
    public:
        TriangularTerm();
        TriangularTerm(const std::string& name, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        TriangularTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar minimum = -INFINITY, flScalar maximum = INFINITY);
        virtual ~TriangularTerm();

        virtual void setA(flScalar a);
        virtual flScalar a() const;

        virtual void setB(flScalar b);
        virtual flScalar b() const;

        virtual void setC(flScalar c);
        virtual flScalar c() const;

        virtual TriangularTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;

        virtual eMembershipFunction type() const;

        virtual std::string toString() const;

    };

} // namespace fuzzy_lite

#endif /* FL_TRIANGULARTERM_H_ */
