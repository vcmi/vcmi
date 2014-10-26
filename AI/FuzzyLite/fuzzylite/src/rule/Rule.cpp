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

#include "fl/rule/Rule.h"

#include "fl/Exception.h"
#include "fl/hedge/Hedge.h"
#include "fl/imex/FllExporter.h"
#include "fl/norm/Norm.h"
#include "fl/rule/Antecedent.h"
#include "fl/rule/Consequent.h"

#include <sstream>
#include <vector>

namespace fl {

    Rule::Rule(const std::string& text, scalar weight)
    : _text(text), _weight(weight), _antecedent(new Antecedent), _consequent(new Consequent) {
    }

    Rule::Rule(const Rule& other) : _text(other._text), _weight(other._weight),
    _antecedent(new Antecedent), _consequent(new Consequent) {
    }

    Rule& Rule::operator=(const Rule& other) {
        if (this != &other) {
            unload();

            _text = other._text;
            _weight = other._weight;
            _antecedent.reset(new Antecedent);
            _consequent.reset(new Consequent);
        }
        return *this;
    }

    Rule::~Rule() {
        unload();
    }

    void Rule::setText(const std::string& text) {
        this->_text = text;
    }

    std::string Rule::getText() const {
        return this->_text;
    }

    void Rule::setWeight(scalar weight) {
        this->_weight = weight;
    }

    scalar Rule::getWeight() const {
        return this->_weight;
    }

    void Rule::setAntecedent(Antecedent* antecedent) {
        this->_antecedent.reset(antecedent);
    }

    Antecedent* Rule::getAntecedent() const {
        return this->_antecedent.get();
    }

    void Rule::setConsequent(Consequent* consequent) {
        this->_consequent.reset(consequent);
    }

    Consequent* Rule::getConsequent() const {
        return this->_consequent.get();
    }

    /**
     * Operations for std::vector _hedges
     */
    void Rule::addHedge(Hedge* hedge) {
        this->_hedges[hedge->name()] = hedge;
    }

    Hedge* Rule::getHedge(const std::string& name) const {
        std::map<std::string, Hedge*>::const_iterator it = this->_hedges.find(name);
        if (it != this->_hedges.end()) {
            if (it->second) return it->second;
        }
        return fl::null;
    }

    Hedge* Rule::removeHedge(const std::string& name) {
        Hedge* result = fl::null;
        std::map<std::string, Hedge*>::iterator it = this->_hedges.find(name);
        if (it != this->_hedges.end()) {
            result = it->second;
            this->_hedges.erase(it);
        }
        return result;
    }

    bool Rule::hasHedge(const std::string& name) const {
        std::map<std::string, Hedge*>::const_iterator it = this->_hedges.find(name);
        return (it != this->_hedges.end());
    }

    int Rule::numberOfHedges() const {
        return this->_hedges.size();
    }

    void Rule::setHedges(const std::map<std::string, Hedge*>& hedges) {
        this->_hedges = hedges;
    }

    const std::map<std::string, Hedge*>& Rule::hedges() const {
        return this->_hedges;
    }

    std::map<std::string, Hedge*>& Rule::hedges() {
        return this->_hedges;
    }

    scalar Rule::activationDegree(const TNorm* conjunction, const SNorm* disjunction) const {
        if (not isLoaded()) {
            throw fl::Exception("[rule error] the following rule is not loaded: " + _text, FL_AT);
        }
        return _weight * getAntecedent()->activationDegree(conjunction, disjunction);
    }

    void Rule::activate(scalar degree, const TNorm* activation) const {
        if (not isLoaded()) {
            throw fl::Exception("[rule error] the following rule is not loaded: " + _text, FL_AT);
        }
        getConsequent()->modify(degree, activation);
    }

    bool Rule::isLoaded() const {
        return _antecedent->isLoaded() and _consequent->isLoaded();
    }

    void Rule::unload() {
        _antecedent->unload();
        _consequent->unload();

        for (std::map<std::string, Hedge*>::const_iterator it = _hedges.begin();
                it != _hedges.end(); ++it) {
            delete it->second;
        }
        _hedges.clear();
    }

    void Rule::load(const Engine* engine) {
        load(_text, engine);
    }

    void Rule::load(const std::string& rule, const Engine* engine) {
        this->_text = rule;
        std::istringstream tokenizer(rule.substr(0, rule.find_first_of('#')));
        std::string token;
        std::ostringstream ossAntecedent, ossConsequent;
        scalar weight = 1.0;

        enum FSM {
            S_NONE, S_IF, S_THEN, S_WITH, S_END
        };
        FSM state = S_NONE;
        try {
            while (tokenizer >> token) {

                switch (state) {
                    case S_NONE:
                        if (token == Rule::ifKeyword()) state = S_IF;
                        else {
                            std::ostringstream ex;
                            ex << "[syntax error] expected keyword <" << Rule::ifKeyword() <<
                                    ">, but found <" << token << "> in rule: " << rule;
                            throw fl::Exception(ex.str(), FL_AT);
                        }
                        break;
                    case S_IF:
                        if (token == Rule::thenKeyword()) state = S_THEN;
                        else ossAntecedent << token << " ";
                        break;
                    case S_THEN:
                        if (token == Rule::withKeyword()) state = S_WITH;
                        else ossConsequent << token << " ";
                        break;
                    case S_WITH:
                        try {
                            weight = fl::Op::toScalar(token);
                            state = S_END;
                        } catch (fl::Exception& e) {
                            std::ostringstream ex;
                            ex << "[syntax error] expected a numeric value as the weight of the rule: "
                                    << rule;
                            e.append(ex.str(), FL_AT);
                            throw e;
                        }
                        break;
                    case S_END:
                        std::ostringstream ex;
                        ex << "[syntax error] unexpected token <" << token << "> at the end of rule";
                        throw fl::Exception(ex.str(), FL_AT);
                }
            }
            if (state == S_NONE) {
                std::ostringstream ex;
                ex << "[syntax error] " << (rule.empty() ? "empty rule" : "ignored rule: " + rule);
                throw fl::Exception(ex.str(), FL_AT);
            } else if (state == S_IF) {
                std::ostringstream ex;
                ex << "[syntax error] keyword <" << Rule::thenKeyword() << "> not found in rule: " << rule;
                throw fl::Exception(ex.str(), FL_AT);
            } else if (state == S_WITH) {
                std::ostringstream ex;
                ex << "[syntax error] expected a numeric value as the weight of the rule: " << rule;
                throw fl::Exception(ex.str(), FL_AT);
            }

            _antecedent->load(ossAntecedent.str(), this, engine);
            _consequent->load(ossConsequent.str(), this, engine);
            _weight = weight;

        } catch (...) {
            unload();
            throw;
        }
    }

    std::string Rule::toString() const {
        return FllExporter().toString(this);
    }

    Rule* Rule::parse(const std::string& rule, const Engine* engine) {
        FL_unique_ptr<Rule> result(new Rule);
        result->load(rule, engine);
        return result.release();
    }

}
