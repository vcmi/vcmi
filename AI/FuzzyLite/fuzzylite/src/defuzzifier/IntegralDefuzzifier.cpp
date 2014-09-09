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

#include "fl/defuzzifier/IntegralDefuzzifier.h"

namespace fl {

    int IntegralDefuzzifier::_defaultResolution = 200;

    void IntegralDefuzzifier::setDefaultResolution(int defaultResolution) {
        _defaultResolution = defaultResolution;
    }

    int IntegralDefuzzifier::defaultResolution() {
        return _defaultResolution;
    }

    IntegralDefuzzifier::IntegralDefuzzifier(int resolution)
    : Defuzzifier(), _resolution(resolution) {
    }

    IntegralDefuzzifier::~IntegralDefuzzifier() {
    }

    void IntegralDefuzzifier::setResolution(int resolution) {
        this->_resolution = resolution;
    }

    int IntegralDefuzzifier::getResolution() const {
        return this->_resolution;
    }
}
