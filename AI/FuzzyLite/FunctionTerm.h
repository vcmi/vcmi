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
 * File:   FunctionTerm.h
 * Author: jcrada
 *
 * Created on March 10, 2010, 6:16 PM
 */

#ifndef FL_FUNCTIONTERM_H
#define	FL_FUNCTIONTERM_H

#include "LinguisticTerm.h"
#include "InfixToPostfix.h"

namespace fl {

    class FunctionTerm : public LinguisticTerm {
    private:
        InfixToPostfix _ip;
        std::string _postfix_function;
        std::string _infix_function;
    public:
        FunctionTerm();
        FunctionTerm(const std::string& name, const std::string& f_infix, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        FunctionTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
                const std::string& f_infix, flScalar minimum = -INFINITY,
                flScalar maximum = INFINITY);
        virtual ~FunctionTerm();

        virtual void setInfixFunction(const std::string& infix);
        virtual std::string infixFunction() const;
        virtual void setPostfixFunction(const std::string& postfix);
        virtual std::string postfixFunction() const;

        virtual bool isValid() const;

        virtual FunctionTerm* clone() const;
        virtual flScalar membership(flScalar crisp) const;
        virtual eMembershipFunction type() const;
        virtual std::string toString() const;

    };
}

#endif	/* FL_FUNCTIONTERM_H */

