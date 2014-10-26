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

#include "fl/term/Activated.h"

#include "fl/imex/FllExporter.h"
#include "fl/norm/TNorm.h"

namespace fl {

    Activated::Activated(const Term* term, scalar degree, const TNorm* activation)
    : Term(""), _term(term), _degree(degree), _activation(activation) {
        if (term) this->_name = term->getName();
    }

    Activated::~Activated() {
    }

    std::string Activated::className() const {
        return "Activated";
    }

    scalar Activated::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;
        if (not _activation) throw fl::Exception("[activation error] "
                "activation operator needed to activate " + _term->toString(), FL_AT);
        return _activation->compute(this->_term->membership(x), _degree);
    }

    std::string Activated::parameters() const {
        FllExporter exporter;
        std::ostringstream ss;
        ss << Op::str(_degree) << " " << exporter.toString(_activation) << " "
                << exporter.toString(_term);
        return ss.str();
    }

    void Activated::configure(const std::string& parameters) {
        (void) parameters;
    }

    std::string Activated::toString() const {
        FllExporter exporter;
        std::ostringstream ss;
        ss << exporter.toString(_activation) << "("
                << Op::str(_degree) << ","
                << _term->getName() << ")";
        return ss.str();
    }

    void Activated::setTerm(const Term* term) {
        this->_term = term;
    }

    const Term* Activated::getTerm() const {
        return this->_term;
    }

    void Activated::setDegree(scalar degree) {
        this->_degree = degree;
    }

    scalar Activated::getDegree() const {
        return this->_degree;
    }

    void Activated::setActivation(const TNorm* activation) {
        this->_activation = activation;
    }

    const TNorm* Activated::getActivation() const {
        return this->_activation;
    }

    Activated* Activated::clone() const {
        return new Activated(*this);
    }

}
