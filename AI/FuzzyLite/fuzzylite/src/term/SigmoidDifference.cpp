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

#include "fl/term/SigmoidDifference.h"

namespace fl {

    SigmoidDifference::SigmoidDifference(const std::string& name,
            scalar left, scalar rising,
            scalar falling, scalar right, scalar height)
    : Term(name, height), _left(left), _rising(rising), _falling(falling), _right(right) {
    }

    SigmoidDifference::~SigmoidDifference() {
    }

    std::string SigmoidDifference::className() const {
        return "SigmoidDifference";
    }

    scalar SigmoidDifference::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;

        scalar a = 1.0 / (1 + std::exp(-_rising * (x - _left)));
        scalar b = 1.0 / (1 + std::exp(-_falling * (x - _right)));
        return _height * std::abs(a - b);
    }

    std::string SigmoidDifference::parameters() const {
        return Op::join(4, " ", _left, _rising, _falling, _right) +
                (not Op::isEq(_height, 1.0) ? " " + Op::str(_height) : "");
    }

    void SigmoidDifference::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> values = Op::split(parameters, " ");
        std::size_t required = 4;
        if (values.size() < required) {
            std::ostringstream ex;
            ex << "[configuration error] term <" << className() << ">"
                    << " requires <" << required << "> parameters";
            throw fl::Exception(ex.str(), FL_AT);
        }
        setLeft(Op::toScalar(values.at(0)));
        setRising(Op::toScalar(values.at(1)));
        setFalling(Op::toScalar(values.at(2)));
        setRight(Op::toScalar(values.at(3)));
        if (values.size() > required)
            setHeight(Op::toScalar(values.at(required)));
    }

    void SigmoidDifference::setLeft(scalar leftInflection) {
        this->_left = leftInflection;
    }

    scalar SigmoidDifference::getLeft() const {
        return this->_left;
    }

    void SigmoidDifference::setRising(scalar risingSlope) {
        this->_rising = risingSlope;
    }

    scalar SigmoidDifference::getRising() const {
        return this->_rising;
    }

    void SigmoidDifference::setFalling(scalar fallingSlope) {
        this->_falling = fallingSlope;
    }

    scalar SigmoidDifference::getFalling() const {
        return this->_falling;
    }

    void SigmoidDifference::setRight(scalar rightInflection) {
        this->_right = rightInflection;
    }

    scalar SigmoidDifference::getRight() const {
        return this->_right;
    }

    SigmoidDifference* SigmoidDifference::clone() const {
        return new SigmoidDifference(*this);
    }

    Term* SigmoidDifference::constructor() {
        return new SigmoidDifference;
    }


}
