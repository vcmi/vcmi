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
#include "TakagiSugenoConsequent.h"
#include <stack>
#include <iosfwd>

#include "FunctionTerm.h"
#include "SingletonTerm.h"
#include "TakagiSugenoTerm.h"

namespace fl {

    TakagiSugenoConsequent::TakagiSugenoConsequent() {
        _infix2postfix.loadMathOperators();
    }

    TakagiSugenoConsequent::~TakagiSugenoConsequent() {

    }

    const FuzzyEngine& TakagiSugenoConsequent::fuzzyEngine() const {
        return *this->_fuzzy_engine;
    }

    void TakagiSugenoConsequent::setFuzzyEngine(const FuzzyEngine& fuzzy_engine) {
        this->_fuzzy_engine = &fuzzy_engine;
    }

    std::string TakagiSugenoConsequent::consequent() const {
        return this->_consequent;
    }

    void TakagiSugenoConsequent::setConsequent(const std::string& consequent) {
        this->_consequent = consequent;
    }

    std::string TakagiSugenoConsequent::postfixFunction() const {
        return this->_postfix_function;
    }

    void TakagiSugenoConsequent::setPostfixFunction(const std::string& postfix) {
        this->_postfix_function = postfix;
    }

    void TakagiSugenoConsequent::parse(const std::string& consequent,
            const FuzzyEngine& engine) throw (ParsingException) {
        setFuzzyEngine(engine);
        setConsequent(consequent);

        std::stringstream ss(consequent);
        std::string output_lvar;
        ss >> output_lvar;
        if (!fuzzyEngine().outputLVar(output_lvar)) {
            throw ParsingException(FL_AT, "Output variable <" + output_lvar +
                    "> not found in fuzzy engine");
        }
        setOutputLVar(fuzzyEngine().outputLVar(output_lvar));

        std::string equal;
        ss >> equal;
        if (equal != "=") {
            throw ParsingException(FL_AT, "<=> expected but found <" + equal + ">");
        }
        std::string right_side;
        std::string token;
        while (ss >> token) {
            right_side += token + " ";
        }
        setPostfixFunction(_infix2postfix.transform(right_side));
        std::map<std::string, flScalar> variables;
        for (int i = 0; i < fuzzyEngine().numberOfInputLVars(); ++i) {
            variables[fuzzyEngine().inputLVar(i)->name()] =
                    fuzzyEngine().inputLVar(i)->input();
        }
        for (int i = 0; i < fuzzyEngine().numberOfOutputLVars(); ++i) {
            variables[fuzzyEngine().outputLVar(i)->name()] =
                    fuzzyEngine().outputLVar(i)->output().defuzzify();
        }
        try {
            _infix2postfix.evaluate(postfixFunction(), &variables);
        } catch (FuzzyException& e) {
            throw ParsingException(FL_AT, e.message(), e);
        }
    }

    void TakagiSugenoConsequent::execute(flScalar degree) {
        std::map<std::string, flScalar> variables;
        for (int i = 0; i < fuzzyEngine().numberOfInputLVars(); ++i) {
            variables[fuzzyEngine().inputLVar(i)->name()] =
                    fuzzyEngine().inputLVar(i)->input();
        }
        for (int i = 0; i < fuzzyEngine().numberOfOutputLVars(); ++i) {
            variables[fuzzyEngine().outputLVar(i)->name()] =
                    fuzzyEngine().outputLVar(i)->output().defuzzify();
        }

        TakagiSugenoTerm result(fuzzyEngine().fuzzyOperator(), postfixFunction());
        result.setValue(_infix2postfix.evaluate(postfixFunction(), &variables));
        result.setWeight(degree);
        outputLVar()->output().addTerm(result);
    }

    std::string TakagiSugenoConsequent::toString() const {
        return consequent();
    }

    void TakagiSugenoConsequent::main(int args, char** argv) {
    }
}
