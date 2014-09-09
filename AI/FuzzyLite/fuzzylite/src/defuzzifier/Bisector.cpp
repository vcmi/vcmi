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

#include "fl/defuzzifier/Bisector.h"

#include "fl/term/Accumulated.h"
#include "fl/term/Term.h"

namespace fl {

    Bisector::Bisector(int resolution)
    : IntegralDefuzzifier(resolution) {
    }

    Bisector::~Bisector() {

    }

    std::string Bisector::className() const {
        return "Bisector";
    }

    scalar Bisector::defuzzify(const Term* term, scalar minimum, scalar maximum) const {
        if (not fl::Op::isFinite(minimum + maximum)) {
            return fl::nan;
        }
        if (maximum - minimum > _resolution) {
            FL_DBG("[accuracy warning] the resolution <" << _resolution << "> "
                    "is smaller than the range <" << minimum << ", " << maximum << ">. In order to "
                    "improve the accuracy, the resolution should be at least equal to the range.");
        }
        scalar dx = (maximum - minimum) / _resolution;

        int counter = _resolution;
        int left = 0, right = 0;
        scalar leftArea = 0, rightArea = 0;
        scalar xLeft = minimum, xRight = maximum;
        while (counter-- > 0) {
            if (fl::Op::isLE(leftArea, rightArea)) {
                xLeft = minimum + (left + 0.5) * dx;
                leftArea += term->membership(xLeft);
                left++;
            } else {
                xRight = maximum - (right + 0.5) * dx;
                rightArea += term->membership(xRight);
                right++;
            }
        }

        //Inverse weighted average to compensate
        scalar bisector = (leftArea * xRight + rightArea * xLeft) / (leftArea + rightArea);
        return bisector;
    }

    Bisector* Bisector::clone() const {
        return new Bisector(*this);
    }

    Defuzzifier* Bisector::constructor() {
        return new Bisector;
    }


}
