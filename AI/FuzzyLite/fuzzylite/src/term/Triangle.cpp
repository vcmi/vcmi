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

#include "fl/term/Triangle.h"

namespace fl {

    Triangle::Triangle(const std::string& name, scalar vertexA, scalar vertexB, scalar vertexC, scalar height)
    : Term(name, height), _vertexA(vertexA), _vertexB(vertexB), _vertexC(vertexC) {
        if (fl::Op::isNaN(vertexC)) {
            this->_vertexC = vertexB;
            this->_vertexB = (vertexA + vertexB) / 2.0;
        }
    }

    Triangle::~Triangle() {
    }

    std::string Triangle::className() const {
        return "Triangle";
    }

    scalar Triangle::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;

        if (Op::isLt(x, _vertexA) or Op::isGt(x, _vertexC))
            return _height * 0.0;

        if (Op::isEq(x, _vertexB))
            return _height * 1.0;

        if (Op::isLt(x, _vertexB))
            return _height * (x - _vertexA) / (_vertexB - _vertexA);

        return _height * (_vertexC - x) / (_vertexC - _vertexB);
    }

    std::string Triangle::parameters() const {
        return Op::join(3, " ", _vertexA, _vertexB, _vertexC) +
                (not Op::isEq(_height, 1.0) ? " " + Op::str(_height) : "");
    }

    void Triangle::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> values = Op::split(parameters, " ");
        std::size_t required = 3;
        if (values.size() < required) {
            std::ostringstream ex;
            ex << "[configuration error] term <" << className() << ">"
                    << " requires <" << required << "> parameters";
            throw fl::Exception(ex.str(), FL_AT);
        }
        setVertexA(Op::toScalar(values.at(0)));
        setVertexB(Op::toScalar(values.at(1)));
        setVertexC(Op::toScalar(values.at(2)));
        if (values.size() > required)
            setHeight(Op::toScalar(values.at(required)));
    }

    void Triangle::setVertexA(scalar a) {
        this->_vertexA = a;
    }

    scalar Triangle::getVertexA() const {
        return this->_vertexA;
    }

    void Triangle::setVertexB(scalar b) {
        this->_vertexB = b;
    }

    scalar Triangle::getVertexB() const {
        return this->_vertexB;
    }

    void Triangle::setVertexC(scalar c) {
        this->_vertexC = c;
    }

    scalar Triangle::getVertexC() const {
        return this->_vertexC;
    }

    Triangle* Triangle::clone() const {
        return new Triangle(*this);
    }

    Term* Triangle::constructor() {
        return new Triangle;
    }


}
