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
 * File:   InfixToPostfix.h
 * Author: jcrada
 *
 * Created on March 6, 2010, 1:14 AM
 */

#ifndef _INFIXTOPOSTFIX_H
#define	_INFIXTOPOSTFIX_H

#include <vector>
#include <string>
#include <map>
#include "flScalar.h"

namespace fl {

    class InfixToPostfix {
    private:
        std::vector<std::string> _operators;
    protected:
        virtual std::string preprocess(const std::string& infix) const;
    public:
        InfixToPostfix();
        virtual ~InfixToPostfix();

        virtual void addOperator(const std::string& op, int precedence = 0);
        virtual std::string removeOperator(int index);
        virtual std::string getOperator(int index) const;
        virtual int operatorPrecedence(const std::string& op) const;
        virtual int numberOfOperators() const;

        virtual bool isOperator(const std::string& op) const;
        virtual bool isUnaryOperator(const std::string op) const;
        virtual bool isOperand(const std::string& op) const;
        virtual bool isNumeric(const std::string& op) const;

        virtual void loadLogicalOperators();
        virtual void loadMathOperators();

        virtual std::string transform(const std::string& infix) const;
        virtual flScalar evaluate(const std::string postfix,
                const std::map<std::string, flScalar>* variables = NULL) const;

        virtual flScalar compute(const std::string& op, flScalar a, flScalar b) const;

        static void main(int args, char** argv);

    };
}

#endif	/* _INFIXTOPOSTFIX_H */

