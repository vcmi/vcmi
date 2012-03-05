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
#include "DescriptiveAntecedent.h"
#include "StrOp.h"
#include <stack>
#include "FuzzyRule.h"

namespace fl {

    DescriptiveAntecedent::DescriptiveAntecedent() : FuzzyAntecedent(),
    _left(NULL), _right(NULL), _operation(O_NONE), _term(NULL) {

    }

    DescriptiveAntecedent::DescriptiveAntecedent(const FuzzyOperator& fuzzy_op) :
    FuzzyAntecedent(fuzzy_op), _left(NULL), _right(NULL), _operation(O_NONE),
    _term(NULL) {
    }

    DescriptiveAntecedent::~DescriptiveAntecedent() {
        if (_left) {
            delete _left;
        }
        if (_right) {
            delete _right;
        }
    }

    void DescriptiveAntecedent::setLeft(DescriptiveAntecedent* l_antecedent) {
        this->_left = l_antecedent;
    }

    DescriptiveAntecedent* DescriptiveAntecedent::left() const {
        return this->_left;
    }

    void DescriptiveAntecedent::setRight(DescriptiveAntecedent* r_antecedent) {
        this->_right = r_antecedent;
    }

    DescriptiveAntecedent* DescriptiveAntecedent::right() const {
        return this->_right;
    }

    void DescriptiveAntecedent::setOperation(OPERATION operation) {
        this->_operation = operation;
    }

    DescriptiveAntecedent::OPERATION DescriptiveAntecedent::operation() const {
        return this->_operation;
    }

    void DescriptiveAntecedent::addHedge(Hedge* hedge) {
        this->_hedges.push_back(hedge);
    }

    int DescriptiveAntecedent::numberOfHedges() const {
        return this->_hedges.size();
    }

    Hedge* DescriptiveAntecedent::hedge(int index) const {
        return this->_hedges[index];
    }

    void DescriptiveAntecedent::setTerm(const LinguisticTerm* term) {
        this->_term = term;
    }

    const LinguisticTerm* DescriptiveAntecedent::term() const {
        return this->_term;
    }

    bool DescriptiveAntecedent::isTerminal() const {
        return operation() == O_NONE;
    }

    flScalar DescriptiveAntecedent::degreeOfTruth() const {
        if (!isTerminal()) {
            if (left() == NULL || right() == NULL) {
                throw NullPointerException(FL_AT, toString());
            }
            switch (operation()) {
                case O_AND:
                    return fuzzyOperator().tnorm().execute(left()->degreeOfTruth(),
                            right()->degreeOfTruth());
                case O_OR:
                    return fuzzyOperator().snorm().execute(left()->degreeOfTruth(),
                            right()->degreeOfTruth());
                default:
                    throw InvalidArgumentException(FL_AT, "Operation " + StrOp::IntToString(operation()) + " not available");
            }
        }

        flScalar result = term()->membership(inputLVar()->input());
        for (int i = 0; i < numberOfHedges(); ++i) {
            result = hedge(i)->hedge(result);
        }
        return result;
    }

    std::string DescriptiveAntecedent::toString() const {
        std::stringstream ss;
        if (isTerminal()) {
            ss << inputLVar()->name() + " " + FuzzyRule::FR_IS + " ";
            for (int i = 0; i < numberOfHedges(); ++i) {
                ss << hedge(i)->name() << " ";
            }
            ss << term()->name();
        } else {
            ss << " ( " + (left() ? left()->toString() : "NULL");
            ss << " " + (operation() == O_AND ? FuzzyRule::FR_AND : FuzzyRule::FR_OR);
            ss << " " + (right() ? right()->toString() : "NULL") + " ) ";
        }
        return ss.str();
    }

    void DescriptiveAntecedent::parse(const std::string& antecedent,
            const FuzzyEngine& engine) throw (ParsingException) {
        enum e_state {
            //e.g. Posfix antecedent: Energy is LOW Distance is FAR_AWAY and
            S_LVAR = 1, S_IS, S_HEDGE, S_TERM, S_OPERATOR
        };
        e_state current_state = S_LVAR;
        std::string postfix_antecedent = InfixToPostfix(antecedent);
        std::stringstream ss(postfix_antecedent);
        std::string token;
        InputLVar* input = NULL;
        std::vector<Hedge*> hedges;
        Hedge* hedge = NULL;
        LinguisticTerm* term = NULL;
        std::stack<DescriptiveAntecedent*> f_antecedents;
        DescriptiveAntecedent* tmp_antecedent = NULL;
        // <editor-fold desc="State Machine">
        while (ss >> token) {
            switch (current_state) {
                    //e.g.Postfix: Energy is LOW Distance is FAR_AWAY and. After term follows Operator or LVar
                case S_LVAR:
                case S_OPERATOR:
                    input = engine.inputLVar(token);
                    if (input) {
                        current_state = S_IS;
                        break;
                    }
                    if (token != FuzzyRule::FR_AND && token != FuzzyRule::FR_OR) {
                        //if it is not and InputLVar and not an Operator then exception
                        throw ParsingException(FL_AT, "Input variable <" +
                                token + "> not registered in fuzzy engine");
                        //                        throw RuleParsingException(FL_AT,
                        //                                "Operator expected but found <" + token +
                        //                                ">. Antecedent: " + antecedent);
                    }
                    //A is a  B is b  and
                    if (isTerminal()) {
                        setLeft(f_antecedents.top());
                        f_antecedents.pop();
                        setRight(f_antecedents.top());
                        f_antecedents.pop();
                        setOperation(token == FuzzyRule::FR_AND ? DescriptiveAntecedent::O_AND :
                                DescriptiveAntecedent::O_OR); //I am not terminal anymore
                    } else {
                        setLeft(new DescriptiveAntecedent(*this));
                        setRight(f_antecedents.top());
                        f_antecedents.pop();
                        setOperation(token == FuzzyRule::FR_AND ? DescriptiveAntecedent::O_AND :
                                DescriptiveAntecedent::O_OR); //I am not terminal anymore
                    }
                    break;
                case S_IS:
                    if (token == FuzzyRule::FR_IS) {
                        current_state = S_HEDGE;
                    } else {
                        throw ParsingException(FL_AT, "<is> expected but found <" +
                                token + ">");
                    }
                    break;
                case S_HEDGE:
                    hedge = engine.hedgeSet().get(token);
                    if (hedge) {
                        hedges.push_back(hedge); //And check for more hedges
                        break;
                    }
                    //intentional fall-through in case there are no hedges
                case S_TERM:
                    term = input->term(token);
                    if (!term) {
                        throw ParsingException(FL_AT, "Term <" + token +
                                "> not found in input variable <" + input->name() +
                                ">");
                    }
                    current_state = S_OPERATOR;
                    tmp_antecedent = new DescriptiveAntecedent(engine.fuzzyOperator());
                    tmp_antecedent->setInputLVar(input);
                    tmp_antecedent->setTerm(term);
                    for (size_t i = 0; i < hedges.size(); ++i) {
                        tmp_antecedent->addHedge(hedges[i]);
                    }
                    f_antecedents.push(tmp_antecedent);
                    tmp_antecedent = NULL;
                    hedges.clear();
                    break;
            }
        }
        //</editor-fold>

        if (!f_antecedents.empty()) { //e.g. Rule: if A is a then X is x (one antecedent only)
            FL_ASSERT(f_antecedents.size() == 1);
            DescriptiveAntecedent* me = f_antecedents.top();
            setInputLVar(me->inputLVar());
            for (int i = 0; i < me->numberOfHedges(); ++i) {
                addHedge(me->hedge(i));
            }
            setTerm(me->term());
            delete me;
        }

    }

    std::string DescriptiveAntecedent::Preprocess(const std::string& infix) {
        std::string result;
        for (size_t i = 0; i < infix.size(); ++i) {
            if (infix[i] == '(') {
                result += " ( ";
            } else if (infix[i] == ')') {
                result += " ) ";
            } else {
                result += infix[i];
            }
        }
        return result;
    }

    std::string DescriptiveAntecedent::InfixToPostfix(const std::string& infix) {
        std::string p_infix = Preprocess(infix);
        std::stringstream ss(p_infix);
        std::string token;
        std::string tmp;
        std::string result;
        std::stack<std::string> stack;

        while (ss >> token) {
            if (token == "(") {
                stack.push(token);
            } else if (token == ")") {
                FL_ASSERT(!stack.empty());
                tmp = stack.top();
                stack.pop();
                while (tmp != "(") {
                    result += tmp + " ";
                    FL_ASSERT(!stack.empty());
                    tmp = stack.top();
                    stack.pop();
                }
            } else if (token == FuzzyRule::FR_AND || token == FuzzyRule::FR_OR) {
                if (stack.empty()) {
                    stack.push(token);
                } else {
                    FL_ASSERT(!stack.empty())
                    tmp = stack.top();
                    stack.pop();
                    while (tmp != FuzzyRule::FR_AND && tmp != FuzzyRule::FR_OR && tmp != "(") {
                        result += tmp + " ";
                        FL_ASSERT(!stack.empty())
                        tmp = stack.top();
                        stack.pop();
                    }
                    stack.push(tmp);
                    stack.push(token);
                }
            } else {
                result += token + " ";
            }
        }

        while (!stack.empty()) {
            FL_ASSERT(!stack.empty())
            token = stack.top();
            stack.pop();
            result += token + " ";
        }
        return result;
    }


}
