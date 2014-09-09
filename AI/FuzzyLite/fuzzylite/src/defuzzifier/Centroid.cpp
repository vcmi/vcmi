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

#include "fl/defuzzifier/Centroid.h"

#include "fl/term/Accumulated.h"
#include "fl/term/Term.h"


namespace fl {

    Centroid::Centroid(int divisions)
    : IntegralDefuzzifier(divisions) {
    }

    Centroid::~Centroid() {

    }

    std::string Centroid::className() const {
        return "Centroid";
    }

    scalar Centroid::defuzzify(const Term* term, scalar minimum, scalar maximum) const {
        if (not fl::Op::isFinite(minimum + maximum)) {
            return fl::nan;
        }
        if (maximum - minimum > _resolution) {
            FL_DBG("[accuracy warning] the resolution <" << _resolution << "> "
                    "is smaller than the range <" << minimum << ", " << maximum << ">. In order to "
                    "improve the accuracy, the resolution should be at least equal to the range.");
        }
        scalar dx = (maximum - minimum) / _resolution;
        scalar x, y;
        scalar area = 0, xcentroid = 0, ycentroid = 0;
        for (int i = 0; i < _resolution; ++i) {
            x = minimum + (i + 0.5) * dx;
            y = term->membership(x);

            xcentroid += y * x;
            ycentroid += y * y;
            area += y;
        }
        xcentroid /= area;
        ycentroid /= 2 * area;
        area *= dx; //total area... unused, but for future reference.
        return xcentroid;
    }

    Centroid* Centroid::clone() const {
        return new Centroid(*this);
    }

    Defuzzifier* Centroid::constructor() {
        return new Centroid;
    }


}
