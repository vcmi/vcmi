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

#include "fl/factory/FactoryManager.h"

#include "fl/factory/DefuzzifierFactory.h"
#include "fl/factory/FunctionFactory.h"
#include "fl/factory/HedgeFactory.h"
#include "fl/factory/SNormFactory.h"
#include "fl/factory/TermFactory.h"
#include "fl/factory/TNormFactory.h"

namespace fl {

    FactoryManager FactoryManager::_instance;

    FactoryManager* FactoryManager::instance() {
        return &_instance;
    }

    FactoryManager::FactoryManager() :
    _tnorm(new TNormFactory), _snorm(new SNormFactory), _defuzzifier(new DefuzzifierFactory),
    _term(new TermFactory), _hedge(new HedgeFactory), _function(new FunctionFactory) {
    }

    FactoryManager::FactoryManager(TNormFactory* tnorm, SNormFactory* snorm,
            DefuzzifierFactory* defuzzifier, TermFactory* term,
            HedgeFactory* hedge, FunctionFactory* function) :
    _tnorm(tnorm), _snorm(snorm), _defuzzifier(defuzzifier), _term(term), _hedge(hedge),
    _function(function) {
    }

    FactoryManager::FactoryManager(const FactoryManager& other)
    : _tnorm(fl::null), _snorm(fl::null), _defuzzifier(fl::null), _term(fl::null), _hedge(fl::null), _function(fl::null) {
        if (other._tnorm.get()) this->_tnorm.reset(new TNormFactory(*other._tnorm.get()));
        if (other._snorm.get()) this->_snorm.reset(new SNormFactory(*other._snorm.get()));
        if (other._defuzzifier.get()) this->_defuzzifier.reset(new DefuzzifierFactory(*other._defuzzifier.get()));
        if (other._term.get()) this->_term.reset(new TermFactory(*other._term.get()));
        if (other._hedge.get()) this->_hedge.reset(new HedgeFactory(*other._hedge.get()));
        if (other._function.get()) this->_function.reset(new FunctionFactory(*other._function.get()));
    }

    FactoryManager& FactoryManager::operator=(const FactoryManager& other) {
        if (this != &other) {
            if (other._tnorm.get()) this->_tnorm.reset(new TNormFactory(*other._tnorm.get()));
            if (other._snorm.get()) this->_snorm.reset(new SNormFactory(*other._snorm.get()));
            if (other._defuzzifier.get()) this->_defuzzifier.reset(new DefuzzifierFactory(*other._defuzzifier.get()));
            if (other._term.get()) this->_term.reset(new TermFactory(*other._term.get()));
            if (other._hedge.get()) this->_hedge.reset(new HedgeFactory(*other._hedge.get()));
            if (other._function.get()) this->_function.reset(new FunctionFactory(*other._function.get()));
        }
        return *this;
    }

    FactoryManager::~FactoryManager() {
    }

    void FactoryManager::setTnorm(TNormFactory* tnorm) {
        this->_tnorm.reset(tnorm);
    }

    TNormFactory* FactoryManager::tnorm() const {
        return this->_tnorm.get();
    }

    void FactoryManager::setSnorm(SNormFactory* snorm) {
        this->_snorm.reset(snorm);
    }

    SNormFactory* FactoryManager::snorm() const {
        return this->_snorm.get();
    }

    void FactoryManager::setDefuzzifier(DefuzzifierFactory* defuzzifier) {
        this->_defuzzifier.reset(defuzzifier);
    }

    DefuzzifierFactory* FactoryManager::defuzzifier() const {
        return this->_defuzzifier.get();
    }

    void FactoryManager::setTerm(TermFactory* term) {
        this->_term.reset(term);
    }

    TermFactory* FactoryManager::term() const {
        return this->_term.get();
    }

    void FactoryManager::setHedge(HedgeFactory* hedge) {
        this->_hedge.reset(hedge);
    }

    HedgeFactory* FactoryManager::hedge() const {
        return this->_hedge.get();
    }

    void FactoryManager::setFunction(FunctionFactory* function) {
        this->_function.reset(function);
    }

    FunctionFactory* FactoryManager::function() const {
        return this->_function.get();
    }

}
