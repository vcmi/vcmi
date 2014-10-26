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

#include "fl/term/Trapezoid.h"

namespace fl {

    Trapezoid::Trapezoid(const std::string& name,
            scalar vertexA, scalar vertexB, scalar vertexC, scalar vertexD, scalar height)
    : Term(name, height), _vertexA(vertexA), _vertexB(vertexB), _vertexC(vertexC), _vertexD(vertexD) {
    }

    Trapezoid::~Trapezoid() {
    }

    std::string Trapezoid::className() const {
        return "Trapezoid";
    }

    scalar Trapezoid::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;

        if (Op::isLt(x, _vertexA) or Op::isGt(x, _vertexD))
            return _height * 0.0;

        if (Op::isLt(x, _vertexB))
            return _height * Op::min(scalar(1.0), (x - _vertexA) / (_vertexB - _vertexA));

        if (Op::isLE(x, _vertexC))
            return _height * 1.0;

        if (Op::isLt(x, _vertexD))
            return _height * (_vertexD - x) / (_vertexD - _vertexC);

        return _height * 0.0;
    }

    std::string Trapezoid::parameters() const {
        return Op::join(4, " ", _vertexA, _vertexB, _vertexC, _vertexD)+
                (not Op::isEq(_height, 1.0) ? " " + Op::str(_height) : "");
    }

    void Trapezoid::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> values = Op::split(parameters, " ");
        std::size_t required = 4;
        if (values.size() < required) {
            std::ostringstream ex;
            ex << "[configuration error] term <" << className() << ">"
                    << " requires <" << required << "> parameters";
            throw fl::Exception(ex.str(), FL_AT);
        }
        setVertexA(Op::toScalar(values.at(0)));
        setVertexB(Op::toScalar(values.at(1)));
        setVertexC(Op::toScalar(values.at(2)));
        setVertexD(Op::toScalar(values.at(3)));
        if (values.size() > required)
            setHeight(Op::toScalar(values.at(required)));
    }

    void Trapezoid::setVertexA(scalar a) {
        this->_vertexA = a;
    }

    scalar Trapezoid::getVertexA() const {
        return this->_vertexA;
    }

    void Trapezoid::setVertexB(scalar b) {
        this->_vertexB = b;
    }

    scalar Trapezoid::getVertexB() const {
        return this->_vertexB;
    }

    void Trapezoid::setVertexC(scalar c) {
        this->_vertexC = c;
    }

    scalar Trapezoid::getVertexC() const {
        return this->_vertexC;
    }

    void Trapezoid::setVertexD(scalar d) {
        this->_vertexD = d;
    }

    scalar Trapezoid::getVertexD() const {
        return this->_vertexD;
    }

    Trapezoid* Trapezoid::clone() const {
        return new Trapezoid(*this);
    }

    Term* Trapezoid::constructor() {
        return new Trapezoid;
    }


}
