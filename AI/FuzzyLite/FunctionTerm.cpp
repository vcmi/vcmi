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
#include "FunctionTerm.h"

namespace fl {

    FunctionTerm::FunctionTerm() : LinguisticTerm() {
        _ip.loadMathOperators();
    }

    FunctionTerm::FunctionTerm(const std::string& name, const std::string& f_infix,
            flScalar minimum, flScalar maximum)
    : LinguisticTerm(name, minimum, maximum) {
        _ip.loadMathOperators();
        setInfixFunction(f_infix);
    }

    FunctionTerm::FunctionTerm(const FuzzyOperator& fuzzy_op, const std::string& name,
            const std::string& f_infix, flScalar minimum, flScalar maximum)
    : LinguisticTerm(fuzzy_op, name, minimum, maximum) {
        _ip.loadMathOperators();
        setInfixFunction(f_infix);
    }

    FunctionTerm::~FunctionTerm() {

    }

    void FunctionTerm::setInfixFunction(const std::string& infix) {
        this->_infix_function = infix;
        setPostfixFunction(_ip.transform(infix));
    }

    std::string FunctionTerm::infixFunction() const {
        return this->_infix_function;
    }

    void FunctionTerm::setPostfixFunction(const std::string& postfix) {
        this->_postfix_function = postfix;
    }

    std::string FunctionTerm::postfixFunction() const {
        return this->_postfix_function;
    }

    bool FunctionTerm::isValid() const {
        std::map<std::string, flScalar> x;
        x["x"] = 0;
        x["min"] = minimum();
        x["max"] = maximum();
        try {
            _ip.evaluate(postfixFunction(), &x);
            return true;
        } catch (std::exception ex) {
            return false;
        }
    }

    flScalar FunctionTerm::membership(flScalar crisp) const {
        std::map<std::string, flScalar> m;
        m["x"] = crisp;
        m["min"] = minimum();
        m["max"] = maximum();
        return _ip.evaluate(postfixFunction(), &m);
    }

    FunctionTerm* FunctionTerm::clone() const {
        return new FunctionTerm(*this);
    }

    LinguisticTerm::eMembershipFunction FunctionTerm::type() const {
        return MF_FUNCTION;
    }

    std::string FunctionTerm::toString() const {
        std::stringstream ss;
        ss << LinguisticTerm::toString();
        ss << "Function f = " << postfixFunction() << " | x=[" << minimum() << " " << maximum() << "]";
        return ss.str();
    }

}
