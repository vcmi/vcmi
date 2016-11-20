/*
 Author: Juan Rada-Vilela, Ph.D.
 Copyright (C) 2010-2014 FuzzyLite Limited
 All rights reserved

 This file is part of fuzzylite.

 fuzzylite is free software: you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 fuzzylite is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with fuzzylite.  If not, see <http://www.gnu.org/licenses/>.

 fuzzylite™ is a trademark of FuzzyLite Limited.

 */

#include "fl/rule/Consequent.h"

#include "fl/Engine.h"
#include "fl/factory/HedgeFactory.h"
#include "fl/factory/FactoryManager.h"
#include "fl/hedge/Hedge.h"
#include "fl/hedge/Any.h"
#include "fl/norm/TNorm.h"
#include "fl/rule/Expression.h"
#include "fl/rule/Rule.h"
#include "fl/term/Accumulated.h"
#include "fl/term/Activated.h"
#include "fl/variable/OutputVariable.h"

#include <algorithm>

namespace fl {

    Consequent::Consequent() {
    }

    Consequent::~Consequent() {
        unload();
    }

    std::string Consequent::getText() const {
        return this->_text;
    }

    void Consequent::setText(const std::string& text) {
        this->_text = text;
    }

    const std::vector<Proposition*>& Consequent::conclusions() const {
        return this->_conclusions;
    }

    void Consequent::modify(scalar activationDegree, const TNorm* activation) {
        if (not isLoaded()) {
            throw fl::Exception("[consequent error] consequent <" + _text + "> is not loaded", FL_AT);
        }
        for (std::size_t i = 0; i < _conclusions.size(); ++i) {
            Proposition* proposition = _conclusions.at(i);
            if (proposition->variable->isEnabled()) {
                if (not proposition->hedges.empty()) {
                    for (std::vector<Hedge*>::const_reverse_iterator rit = proposition->hedges.rbegin();
                            rit != proposition->hedges.rend(); ++rit) {
                        activationDegree = (*rit)->hedge(activationDegree);
                    }
                }
                Activated* term = new Activated(_conclusions.at(i)->term, activationDegree, activation);
                OutputVariable* outputVariable = dynamic_cast<OutputVariable*> (proposition->variable);
                outputVariable->fuzzyOutput()->addTerm(term);
                FL_DBG("Accumulating " << term->toString());
            }
        }
    }

    bool Consequent::isLoaded() {
        return not _conclusions.empty();
    }

    void Consequent::unload() {
        for (std::size_t i = 0; i < _conclusions.size(); ++i) {
            delete _conclusions.at(i);
        }
        _conclusions.clear();
    }

    void Consequent::load(Rule* rule, const Engine* engine) {
        load(_text, rule, engine);
    }

    void Consequent::load(const std::string& consequent, Rule* rule, const Engine* engine) {
        unload();
        this->_text = consequent;

        if (fl::Op::trim(consequent).empty()) {
            throw fl::Exception("[syntax error] consequent is empty", FL_AT);
        }

        /**
         Extracts the list of propositions from the consequent
         The rules are:
         1) After a variable comes 'is' or '=',
         2) After 'is' comes a hedge or a term
         3) After a hedge comes a hedge or a term
         4) After a term comes operators 'and' or 'with'
         5) After operator 'and' comes a variable
         6) After operator 'with' comes a float
         */
        enum FSM {
            S_VARIABLE = 1, S_IS = 2, S_HEDGE = 4, S_TERM = 8,
            S_AND = 16, S_WITH = 32
        };
        int state = S_VARIABLE;

        Proposition* proposition = fl::null;

        std::stringstream tokenizer(consequent);
        std::string token;
        try {
            while (tokenizer >> token) {
                if (state bitand S_VARIABLE) {
                    if (engine->hasOutputVariable(token)) {
                        proposition = new Proposition;
                        proposition->variable = engine->getOutputVariable(token);
                        _conclusions.push_back(proposition);

                        state = S_IS;
                        continue;
                    }
                }

                if (state bitand S_IS) {
                    if (token == Rule::isKeyword()) {
                        state = S_HEDGE bitor S_TERM;
                        continue;
                    }
                }

                if (state bitand S_HEDGE) {
                    Hedge* hedge = rule->getHedge(token);
                    if (not hedge) {
                        std::vector<std::string> hedges = FactoryManager::instance()->hedge()->available();
                        if (std::find(hedges.begin(), hedges.end(), token) != hedges.end()) {
                            hedge = FactoryManager::instance()->hedge()->constructObject(token);
                            rule->addHedge(hedge);
                        }
                    }
                    if (hedge) {
                        proposition->hedges.push_back(hedge);
                        state = S_HEDGE bitor S_TERM;
                        continue;
                    }
                }

                if (state bitand S_TERM) {
                    if (proposition->variable->hasTerm(token)) {
                        proposition->term = proposition->variable->getTerm(token);
                        state = S_AND bitor S_WITH;
                        continue;
                    }
                }

                if (state bitand S_AND) {
                    if (token == Rule::andKeyword()) {
                        state = S_VARIABLE;
                        continue;
                    }
                }

                //if reached this point, there was an error:
                if (state bitand S_VARIABLE) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected output variable, but found <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }
                if (state bitand S_IS) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected keyword <" << Rule::isKeyword() << ">, "
                            "but found <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }

                if ((state bitand S_HEDGE) or (state bitand S_TERM)) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected hedge or term, but found <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }

                if ((state bitand S_AND) or (state bitand S_WITH)) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected operator <" << Rule::andKeyword() << "> "
                            << "or keyword <" << Rule::withKeyword() << ">, "
                            << "but found <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }

                std::ostringstream ex;
                ex << "[syntax error] unexpected token <" << token << "> in consequent";
                throw fl::Exception(ex.str(), FL_AT);
            }

            if (not ((state bitand S_AND) or (state bitand S_WITH))) { //only acceptable final state
                if (state bitand S_VARIABLE) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected output variable after <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }
                if (state bitand S_IS) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected keyword <" << Rule::isKeyword() << "> "
                            "after <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }
                if ((state bitand S_HEDGE) or (state bitand S_TERM)) {
                    std::ostringstream ex;
                    ex << "[syntax error] consequent expected hedge or term after <" << token << ">";
                    throw fl::Exception(ex.str(), FL_AT);
                }
            }
        } catch (...) {
            unload();
            throw;
        }
    }

    std::string Consequent::toString() const {
        std::stringstream ss;
        for (std::size_t i = 0; i < _conclusions.size(); ++i) {
            ss << _conclusions.at(i)->toString();
            if (i + 1 < _conclusions.size())
                ss << " " << Rule::andKeyword() << " ";
        }
        return ss.str();
    }

}
