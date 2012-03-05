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
#include "InfixToPostfix.h"
#include <stack>
#include "defs.h"
#include <sstream>
#include "StrOp.h"
#include <math.h>
#include <stdlib.h>
namespace fl {

    InfixToPostfix::InfixToPostfix() {

    }

    InfixToPostfix::~InfixToPostfix() {

    }

    void InfixToPostfix::addOperator(const std::string& op, int precedence) {
        this->_operators.insert(this->_operators.begin() + precedence, op);
    }

    std::string InfixToPostfix::removeOperator(int index) {
        std::string result = getOperator(index);
        this->_operators.erase(this->_operators.begin() + index);
        return result;
    }

    std::string InfixToPostfix::getOperator(int index) const {
        return this->_operators.at(index);
    }

    int InfixToPostfix::operatorPrecedence(const std::string& op) const {
        for (int i = 0; i < numberOfOperators(); ++i) {
            std::stringstream ss(getOperator(i));
            std::string token;
            while (ss >> token) {
                if (token == op) {
                    return i;
                }
            }
        }
        return -1;
    }

    int InfixToPostfix::numberOfOperators() const {
        return this->_operators.size();
    }

    bool InfixToPostfix::isOperator(const std::string& op) const {
        return operatorPrecedence(op) != -1;
    }

    bool InfixToPostfix::isUnaryOperator(const std::string op) const {
        return op == "!" || op == "sin" || op == "cos" || op == "tan" ||
                op == "sinh" || op == "cosh" || op == "tanh";
    }

    void InfixToPostfix::loadLogicalOperators() {
        addOperator("!");
        addOperator("or");
        addOperator("and");
    }

    void InfixToPostfix::loadMathOperators() {
        addOperator("^");
        addOperator("/ * %");
        addOperator("sin cos tan sinh cosh tanh");
        addOperator("+ -");

    }

    bool InfixToPostfix::isOperand(const std::string& op) const {
        return !isOperator(op) ;
    }

    bool InfixToPostfix::isNumeric(const std::string& x) const {
        return (int) x.find_first_not_of("0123456789.-+") == -1;
    }

    std::string InfixToPostfix::preprocess(const std::string& infix) const {
        std::string result = infix;
        StrOp::FindReplace(result,"(", " ( ");
        StrOp::FindReplace(result, ")", " ) ");
        return result;
    }

    //TODO: Be Careful: sin(x) / x => x x / sin. The expected is obtained: (sin x) / x => x sin x /
    std::string InfixToPostfix::transform(const std::string& infix) const {
        std::string p_infix = preprocess(infix);
        std::string postfix;
        std::stack<std::string> stack;
        std::stringstream ss(p_infix);
        std::string token;
        while (ss >> token) {
            if (token == "(") {
                stack.push(token);
            } else if (token == ")") {
                while (!stack.empty()) {
                    if (stack.top() == "(") {
                        stack.pop();
                        break;
                    }
                    postfix += stack.top() + " ";
                    stack.pop();
                }
            } else if (isOperand(token)) {
                postfix += token + " ";
            } else if (isOperator(token)) {
                while (!stack.empty() &&
                        operatorPrecedence(token) <= operatorPrecedence(stack.top())) {
                    postfix += stack.top() + " ";
                    stack.pop();
                }
                stack.push(token);
            }
        }
        while (!stack.empty()) {
            postfix += stack.top() + " ";
            stack.pop();
        }
        return postfix;
    }

    flScalar InfixToPostfix::evaluate(const std::string postfix,
            const std::map<std::string, flScalar>* variables) const {
        std::stack<flScalar> stack;
        std::stringstream ss(postfix);
        std::string token;
        while (ss >> token) {
            if (isOperand(token)) {
                if (isNumeric(token)) {
                    stack.push(atof(token.c_str()));
                } else {
                    if (!variables) {
                        throw FuzzyException(FL_AT, "Impossible to compute operand <" + token + "> because no map was supplied");
                    }
                    std::map<std::string, flScalar>::const_iterator it = variables->find(token);
                    if (it == variables->end()) {
                        throw FuzzyException(FL_AT, "Operand <" + token + "> not found in map");
                    }
                    stack.push(it->second);
                }
            } else if (isOperator(token)) {
                if (isUnaryOperator(token)) {
                    if (stack.empty()) {
                        throw FuzzyException(FL_AT, "Error evaluating postfix expression <" + postfix + ">");
                    }
                    flScalar a = stack.top();
                    stack.pop();
                    stack.push(compute(token, a, 0));
                } else {
                    if (stack.size() < 2) {
                        throw FuzzyException(FL_AT, "Error evaluating postfix expression <" + postfix + ">");
                    }
                    flScalar b = stack.top();
                    stack.pop();
                    flScalar a = stack.top();
                    stack.pop();
                    stack.push(compute(token, a, b));
                }
            } 
        }
        if (stack.size() == 1){
            return stack.top();
        }
        throw FuzzyException(FL_AT, "Error evaluating postfix expression <" + postfix + ">");
    }

    flScalar InfixToPostfix::compute(const std::string& op, flScalar a, flScalar b) const {
        if (op == "!") {
            return !(bool)a;
        }
        if (op == "and") {
            return (bool)a && (bool)b;
        }
        if (op == "or") {
            return (bool)a || (bool)b;
        }
        if (op == "+") {
            return a + b;
        }
        if (op == "-") {
            return a - b;
        }
        if (op == "*") {
            return a * b;
        }
        if (op == "/") {
            return a / b;
        }
        if (op == "%") {
            return (long) a % (long) b;
        }
        if (op == "^") {
            return pow(a, b);
        }
        if (op == "sin") {
            return sin(a);
        }
        if (op == "cos") {
            return cos(a);
        }
        if (op == "tan") {
            return tan(a);
        }
        if (op == "sinh") {
            return sinh(a);
        }
        if (op == "cosh") {
            return cosh(a);
        }
        if (op == "tanh") {
            return tanh(a);
        }
        throw FuzzyException(FL_AT, "Operator <" + op + "> not defined");
    }

    void InfixToPostfix::main(int args, char** argv) {
        std::map<std::string, flScalar> m;
        m["x"] = 4;
        m["y"] = 2;
        m["z"] = -4;
        std::string x = "(sin x) / x";
        FL_LOG(x);
        InfixToPostfix ip;
        ip.loadMathOperators();
        std::string postfix = ip.transform(x);
        FL_LOG(postfix);
        for (flScalar x = 0.0 ; x < 10.5 ; x+=0.5){
            m["x"] = x;
            FL_LOG(ip.evaluate(postfix, &m));
        }
        
        
    }
}
