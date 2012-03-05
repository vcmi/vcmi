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
 * SingletonTerm.h
 *
 *  Created on: Jan 7, 2010
 *      Author: jcrada
 */

#ifndef FL_SINGLETONTERM_H_
#define FL_SINGLETONTERM_H_

#include "LinguisticTerm.h"

namespace fl {

    class SingletonTerm : public LinguisticTerm {
    private:
        flScalar _value;
    public:

        SingletonTerm();
        SingletonTerm(const std::string& name, flScalar value = NAN);
        SingletonTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                flScalar value = NAN);
        virtual ~SingletonTerm();

        virtual void setValue(flScalar value);
        virtual flScalar value() const;

        virtual SingletonTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;

        virtual eMembershipFunction type() const;

        virtual std::string toString() const;

    };
} // namespace fl
#endif /* FL_SINGLETONTERM_H_ */
