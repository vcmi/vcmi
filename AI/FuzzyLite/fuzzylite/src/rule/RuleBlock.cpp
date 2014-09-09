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

#include "fl/rule/RuleBlock.h"

#include "fl/imex/FllExporter.h"
#include "fl/norm/TNorm.h"
#include "fl/norm/SNorm.h"
#include "fl/rule/Rule.h"

#include <sstream>

namespace fl {

    RuleBlock::RuleBlock(const std::string& name)
    : _name(name), _enabled(true) {
    }

    RuleBlock::RuleBlock(const RuleBlock& other) : _name(other._name),
    _enabled(true) {
        copyFrom(other);
    }

    RuleBlock& RuleBlock::operator=(const RuleBlock& other) {
        if (this != &other) {
            for (std::size_t i = 0; i < _rules.size(); ++i) {
                delete _rules.at(i);
            }
            _rules.clear();
            _conjunction.reset(fl::null);
            _disjunction.reset(fl::null);
            _activation.reset(fl::null);

            copyFrom(other);
        }
        return *this;
    }

    void RuleBlock::copyFrom(const RuleBlock& source) {
        _name = source._name;
        _enabled = source._enabled;
        if (source._activation.get()) _activation.reset(source._activation->clone());
        if (source._conjunction.get()) _conjunction.reset(source._conjunction->clone());
        if (source._disjunction.get()) _disjunction.reset(source._disjunction->clone());
        for (std::size_t i = 0; i < source._rules.size(); ++i) {
            _rules.push_back(new Rule(*source._rules.at(i)));
        }
    }

    RuleBlock::~RuleBlock() {
        for (std::size_t i = 0; i < _rules.size(); ++i) {
            delete _rules.at(i);
        }
        _rules.clear();
    }

    void RuleBlock::activate() {
        FL_DBG("===================");
        FL_DBG("ACTIVATING RULEBLOCK " << _name);
        for (std::size_t i = 0; i < _rules.size(); ++i) {
            Rule* rule = _rules.at(i);
            if (rule->isLoaded()) {
                scalar activationDegree = rule->activationDegree(_conjunction.get(), _disjunction.get());
                FL_DBG(rule->toString() << " [activationDegree=" << activationDegree << "]");
                if (Op::isGt(activationDegree, 0.0)) {
                    rule->activate(activationDegree, _activation.get());
                }
            } else {
                FL_DBG("Rule not loaded: " << rule->toString());
            }
        }
    }

    void RuleBlock::unloadRules() const {
        for (std::size_t i = 0; i < _rules.size(); ++i) {
            _rules.at(i)->unload();
        }
    }

    void RuleBlock::loadRules(const Engine* engine) {
        std::ostringstream exceptions;
        bool throwException = false;
        for (std::size_t i = 0; i < _rules.size(); ++i) {
            Rule* rule = _rules.at(i);
            if (rule->isLoaded()) {
                rule->unload();
            }
            try {
                rule->load(engine);
            } catch (std::exception& ex) {
                throwException = true;
                exceptions << ex.what() << "\n";
            }
        }
        if (throwException) {
            fl::Exception exception("[ruleblock error] the following "
                    "rules could not be loaded:\n" + exceptions.str(), FL_AT);
            throw exception;
        }
    }

    void RuleBlock::reloadRules(const Engine* engine) {
        unloadRules();
        loadRules(engine);
    }

    void RuleBlock::setName(std::string name) {
        this->_name = name;
    }

    std::string RuleBlock::getName() const {
        return this->_name;
    }

    void RuleBlock::setConjunction(TNorm* tnorm) {
        this->_conjunction.reset(tnorm);
    }

    TNorm* RuleBlock::getConjunction() const {
        return this->_conjunction.get();
    }

    void RuleBlock::setDisjunction(SNorm* snorm) {
        this->_disjunction.reset(snorm);
    }

    SNorm* RuleBlock::getDisjunction() const {
        return this->_disjunction.get();
    }

    void RuleBlock::setActivation(TNorm* activation) {
        this->_activation.reset(activation);
    }

    TNorm* RuleBlock::getActivation() const {
        return this->_activation.get();
    }

    void RuleBlock::setEnabled(bool enabled) {
        this->_enabled = enabled;
    }

    bool RuleBlock::isEnabled() const {
        return this->_enabled;
    }

    std::string RuleBlock::toString() const {
        return FllExporter().toString(this);
    }

    /**
     * Operations for std::vector _rules
     */
    void RuleBlock::addRule(Rule* rule) {
        this->_rules.push_back(rule);
    }

    void RuleBlock::insertRule(Rule* rule, int index) {
        this->_rules.insert(this->_rules.begin() + index, rule);
    }

    Rule* RuleBlock::getRule(int index) const {
        return this->_rules.at(index);
    }

    Rule* RuleBlock::removeRule(int index) {
        Rule* result = this->_rules.at(index);
        this->_rules.erase(this->_rules.begin() + index);
        return result;
    }

    int RuleBlock::numberOfRules() const {
        return this->_rules.size();
    }

    const std::vector<Rule*>& RuleBlock::rules() const {
        return this->_rules;
    }

    void RuleBlock::setRules(const std::vector<Rule*>& rules) {
        this->_rules = rules;
    }

    std::vector<Rule*>& RuleBlock::rules() {
        return this->_rules;
    }


}
