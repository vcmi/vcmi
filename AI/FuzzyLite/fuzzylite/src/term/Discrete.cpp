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

#include "fl/term/Discrete.h"

#include <cstdarg>

namespace fl {

    Discrete::Discrete(const std::string& name, const std::vector<Pair>& xy, scalar height)
    : Term(name, height), _xy(xy) {
    }

    Discrete::~Discrete() {
    }

    std::string Discrete::className() const {
        return "Discrete";
    }

    scalar Discrete::membership(scalar _x_) const {
        if (fl::Op::isNaN(_x_)) return fl::nan;
        if (_xy.empty())
            throw fl::Exception("[discrete error] term is empty", FL_AT);

        /*                ______________________
         *               /                      \
         *              /                        \
         * ____________/                          \____________
         *            x[0]                      x[n-1]
         */


        if (fl::Op::isLE(_x_, _xy.front().first)) return _height * _xy.front().second;
        if (fl::Op::isGE(_x_, _xy.back().first)) return _height * _xy.back().second;

        int lower = -1, upper = -1;

        for (std::size_t i = 0; i < _xy.size(); ++i) {
            if (Op::isEq(_xy.at(i).first, _x_)) return _height * _xy.at(i).second;
            //approximate on the left
            if (Op::isLt(_xy.at(i).first, _x_)) {
                lower = i;
            }
            //get the immediate next one on the right
            if (Op::isGt(_xy.at(i).first, _x_)) {
                upper = i;
                break;
            }
        }
        if (upper < 0) upper = _xy.size() - 1;
        if (lower < 0) lower = 0;

        return _height * Op::scale(_x_, _xy.at(lower).first, _xy.at(upper).first,
                _xy.at(lower).second, _xy.at(upper).second);
    }

    std::string Discrete::parameters() const {
        std::ostringstream ss;
        for (std::size_t i = 0; i < _xy.size(); ++i) {
            ss << fl::Op::str(_xy.at(i).first) << " " << fl::Op::str(_xy.at(i).second);
            if (i + 1 < _xy.size()) ss << " ";
        }
        if (not Op::isEq(_height, 1.0)) ss << " " << Op::str(_height);
        return ss.str();
    }

    void Discrete::configure(const std::string& parameters) {
        if (parameters.empty()) return;
        std::vector<std::string> strValues = Op::split(parameters, " ");
        std::vector<scalar> values(strValues.size());
        for (std::size_t i = 0; i < strValues.size(); ++i) {
            values.at(i) = Op::toScalar(strValues.at(i));
        }
        if (values.size() % 2 == 0) {
            setHeight(1.0);
        } else {
            setHeight(values.back());
            values.pop_back();
        }
        this->_xy = toPairs(values);
    }

    template <typename T>
    Discrete* Discrete::create(const std::string& name, int argc,
            T x1, T y1, ...) { // throw (fl::Exception) {
        std::vector<scalar> xy(argc);
        xy.at(0) = x1;
        xy.at(1) = y1;
        va_list args;
        va_start(args, y1);
        for (int i = 2; i < argc; ++i) {
            xy.at(i) = (scalar) va_arg(args, T);
        }
        va_end(args);

        FL_unique_ptr<Discrete> result(new Discrete(name));
        if (xy.size() % 2 != 0) {
            result->setHeight(xy.back());
            xy.pop_back();
        }
        result->setXY(toPairs(xy));
        return result.release();
    }

    template FL_API Discrete* Discrete::create(const std::string& name, int argc,
            double x1, double y1, ...); // throw (fl::Exception);
    //double, not scalar because variadic promotes floats to double
    template FL_API Discrete* Discrete::create(const std::string& name, int argc,
            int x1, int y1, ...); // throw (fl::Exception);

    void Discrete::setXY(const std::vector<Pair>& pairs) {
        this->_xy = pairs;
    }

    const std::vector<Discrete::Pair>& Discrete::xy() const {
        return this->_xy;
    }

    std::vector<Discrete::Pair>& Discrete::xy() {
        return this->_xy;
    }

    const Discrete::Pair& Discrete::xy(int index) const {
        return this->_xy.at(index);
    }

    Discrete::Pair& Discrete::xy(int index) {
        return this->_xy.at(index);
    }

    std::vector<Discrete::Pair> Discrete::toPairs(const std::vector<scalar>& xy) {
        if (xy.size() % 2 != 0) {
            std::ostringstream os;
            os << "[discrete error] missing value in set of pairs (|xy|=" << xy.size() << ")";
            throw fl::Exception(os.str(), FL_AT);
        }

        std::vector<Pair> result((xy.size() + 1) / 2);
        for (std::size_t i = 0; i + 1 < xy.size(); i += 2) {
            result.at(i / 2).first = xy.at(i);
            result.at(i / 2).second = xy.at(i + 1);
        }
        return result;
    }

    std::vector<Discrete::Pair> Discrete::toPairs(const std::vector<scalar>& xy,
            scalar missingValue) FL_INOEXCEPT {
        std::vector<Pair> result((xy.size() + 1) / 2);
        for (std::size_t i = 0; i + 1 < xy.size(); i += 2) {
            result.at(i / 2).first = xy.at(i);
            result.at(i / 2).second = xy.at(i + 1);
        }
        if (xy.size() % 2 != 0) {
            result.back().first = xy.back();
            result.back().second = missingValue;
        }
        return result;
    }

    std::vector<scalar> Discrete::toVector(const std::vector<Pair>& xy) {
        std::vector<scalar> result(xy.size() * 2);
        for (std::size_t i = 0; i < xy.size(); ++i) {
            result.at(2 * i) = xy.at(i).first;
            result.at(2 * i + 1) = xy.at(i).second;
        }
        return result;
    }

    std::string Discrete::formatXY(const std::vector<Pair>& xy, const std::string& prefix, const std::string& innerSeparator, const std::string& postfix, const std::string& outerSeparator) {
        std::ostringstream os;
        for (std::size_t i = 0; i < xy.size(); ++i) {
            os << prefix << fl::Op::str(xy.at(i).first) << innerSeparator
                    << fl::Op::str(xy.at(i).second) << postfix;
            if (i + 1 < xy.size()) os << outerSeparator;
        }
        return os.str();
    }

    Discrete* Discrete::clone() const {
        return new Discrete(*this);
    }

    Term* Discrete::constructor() {
        return new Discrete;
    }


}
