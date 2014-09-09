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

#include "fl/norm/t/NilpotentMinimum.h"

namespace fl {

    std::string NilpotentMinimum::className() const {
        return "NilpotentMinimum";
    }

    scalar NilpotentMinimum::compute(scalar a, scalar b) const {
        if (Op::isGt(a + b, 1.0)) {
            return Op::min(a, b);
        }
        return 0.0;
    }

    NilpotentMinimum* NilpotentMinimum::clone() const {
        return new NilpotentMinimum(*this);
    }

    TNorm* NilpotentMinimum::constructor() {
        return new NilpotentMinimum;
    }


}

