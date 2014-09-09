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

#ifndef FL_BISECTOR_H
#define FL_BISECTOR_H

#include "fl/defuzzifier/IntegralDefuzzifier.h"

namespace fl {

    class FL_API Bisector : public IntegralDefuzzifier {
    public:
        Bisector(int resolution = defaultResolution());
        virtual ~Bisector() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Bisector)

        virtual std::string className() const FL_IOVERRIDE;
        virtual scalar defuzzify(const Term* term,
                scalar minimum, scalar maximum) const FL_IOVERRIDE;
        virtual Bisector* clone() const FL_IOVERRIDE;

        static Defuzzifier* constructor();
    };

}

#endif  /* FL_BISECTOR_H */

