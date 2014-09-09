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

#include "fl/term/PiShape.h"

namespace fl {

    PiShape::PiShape(const std::string& name, scalar bottomLeft, scalar topLeft,
            scalar topRight, scalar bottomRight, scalar height)
    : Term(name, height), _bottomLeft(bottomLeft), _topLeft(topLeft),
    _topRight(topRight), _bottomRight(bottomRight) {
    }

    PiShape::~PiShape() {
    }

    std::string PiShape::className() const {
        return "PiShape";
    }

    scalar PiShape::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;
        //from Octave smf.m
        scalar a_b_ave = (_bottomLeft + _topLeft) / 2.0;
        scalar b_minus_a = _topLeft - _bottomLeft;
        scalar c_d_ave = (_topRight + _bottomRight) / 2.0;
        scalar d_minus_c = _bottomRight - _topRight;

        if (Op::isLE(x, _bottomLeft)) return _height * 0.0;

        if (Op::isLE(x, a_b_ave))
            return _height * (2.0 * std::pow((x - _bottomLeft) / b_minus_a, 2));

        if (Op::isLt(x, _topLeft))
            return _height * (1.0 - 2.0 * std::pow((x - _topLeft) / b_minus_a, 2));

        if (Op::isLE(x, _topRight))
            return _height * 1.0;

        if (Op::isLE(x, c_d_ave))
            return _height * (1.0 - 2.0 * std::pow((x - _topRight) / d_minus_c, 2));

        if (Op::isLt(x, _bottomRight))
            return _height * (2.0 * std::pow((x - _bottomRight) / d_minus_c, 2));

        return _height * 0.0;
    }

    std::string PiShape::parameters() const {
        return Op::join(4, " ", _bottomLeft, _topLeft, _topRight, _bottomRight) +
                (not Op::isEq(_height, 1.0) ? " " + Op::str(_height) : "");
    }

    void PiShape::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> values = Op::split(parameters, " ");
        std::size_t required = 4;
        if (values.size() < required) {
            std::ostringstream ex;
            ex << "[configuration error] term <" << className() << ">"
                    << " requires <" << required << "> parameters";
            throw fl::Exception(ex.str(), FL_AT);
        }
        setBottomLeft(Op::toScalar(values.at(0)));
        setTopLeft(Op::toScalar(values.at(1)));
        setTopRight(Op::toScalar(values.at(2)));
        setBottomRight(Op::toScalar(values.at(3)));
        if (values.size() > required)
            setHeight(Op::toScalar(values.at(required)));
    }

    void PiShape::setBottomLeft(scalar a) {
        this->_bottomLeft = a;
    }

    scalar PiShape::getBottomLeft() const {
        return this->_bottomLeft;
    }

    void PiShape::setTopLeft(scalar b) {
        this->_topLeft = b;
    }

    scalar PiShape::getTopLeft() const {
        return this->_topLeft;
    }

    void PiShape::setTopRight(scalar d) {
        this->_topRight = d;
    }

    scalar PiShape::getTopRight() const {
        return this->_topRight;
    }

    void PiShape::setBottomRight(scalar c) {
        this->_bottomRight = c;
    }

    scalar PiShape::getBottomRight() const {
        return this->_bottomRight;
    }

    PiShape* PiShape::clone() const {
        return new PiShape(*this);
    }

    Term* PiShape::constructor() {
        return new PiShape;
    }

}
