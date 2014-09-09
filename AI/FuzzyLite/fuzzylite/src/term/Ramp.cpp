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

#include "fl/term/Ramp.h"

namespace fl {

    Ramp::Ramp(const std::string& name, scalar start, scalar end, scalar height)
    : Term(name, height), _start(start), _end(end) {
    }

    Ramp::~Ramp() {
    }

    std::string Ramp::className() const {
        return "Ramp";
    }

    scalar Ramp::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;

        if (Op::isEq(_start, _end)) return _height * 0.0;

        if (Op::isLt(_start, _end)) {
            if (Op::isLE(x, _start)) return _height * 0.0;
            if (Op::isGE(x, _end)) return _height * 1.0;
            return _height * (x - _start) / (_end - _start);
        } else {
            if (Op::isGE(x, _start)) return _height * 0.0;
            if (Op::isLE(x, _end)) return _height * 1.0;
            return _height * (_start - x) / (_start - _end);
        }
    }

    std::string Ramp::parameters() const {
        return Op::join(2, " ", _start, _end) +
                (not Op::isEq(_height, 1.0) ? " " + Op::str(_height) : "");
    }

    void Ramp::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> values = Op::split(parameters, " ");
        std::size_t required = 2;
        if (values.size() < required) {
            std::ostringstream ex;
            ex << "[configuration error] term <" << className() << ">"
                    << " requires <" << required << "> parameters";
            throw fl::Exception(ex.str(), FL_AT);
        }
        setStart(Op::toScalar(values.at(0)));
        setEnd(Op::toScalar(values.at(1)));
        if (values.size() > required)
            setHeight(Op::toScalar(values.at(required)));
    }

    void Ramp::setStart(scalar start) {
        this->_start = start;
    }

    scalar Ramp::getStart() const {
        return this->_start;
    }

    void Ramp::setEnd(scalar end) {
        this->_end = end;
    }

    scalar Ramp::getEnd() const {
        return this->_end;
    }

    Ramp::Direction Ramp::direction() const {
        scalar range = this->_end - this->_start;
        if (not fl::Op::isFinite(range) or fl::Op::isEq(range, 0.0)) return ZERO;

        if (fl::Op::isGt(range, 0.0)) return POSITIVE;

        return NEGATIVE;
    }

    Ramp* Ramp::clone() const {
        return new Ramp(*this);
    }

    Term* Ramp::constructor() {
        return new Ramp;
    }

}
