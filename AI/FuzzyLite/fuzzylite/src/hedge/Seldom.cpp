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

#include "fl/hedge/Seldom.h"

#include "fl/Operation.h"

namespace fl {

    std::string Seldom::name() const {
        return "seldom";
    }

    scalar Seldom::hedge(scalar x) const {
        return Op::isLE(x, 0.5)
                ? std::sqrt(x / 2.0)
                : 1.0 - std::sqrt((1.0 - x) / 2.0);
    }

    Seldom* Seldom::clone() const {
        return new Seldom(*this);
    }

    Hedge* Seldom::constructor() {
        return new Seldom;
    }


}
