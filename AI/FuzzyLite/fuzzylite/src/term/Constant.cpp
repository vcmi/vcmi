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

#include "fl/term/Constant.h"

namespace fl {

    Constant::Constant(const std::string& name, scalar value)
    : Term(name), _value(value) {
    }

    Constant::~Constant() {
    }

    std::string Constant::className() const {
        return "Constant";
    }

    scalar Constant::membership(scalar x) const {
        (void) x;
        return this->_value;
    }

    std::string Constant::parameters() const {
        return Op::str(_value);
    }

    void Constant::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        setValue(Op::toScalar(parameters));
    }

    void Constant::setValue(scalar value) {
        this->_value = value;
    }

    scalar Constant::getValue() const {
        return this->_value;
    }

    Constant* Constant::clone() const {
        return new Constant(*this);
    }

    Term* Constant::constructor() {
        return new Constant;
    }

}
