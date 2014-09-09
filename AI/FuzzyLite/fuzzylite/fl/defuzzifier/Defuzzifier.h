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

//TODO Check http://en.wikipedia.org/wiki/Defuzzification for other defuzzifiers.

#ifndef FL_DEFUZZIFIER_H
#define FL_DEFUZZIFIER_H

#include "fl/fuzzylite.h"
#include <string>

namespace fl {
    class Term;

    class FL_API Defuzzifier {
    public:

        Defuzzifier() {
        }

        virtual ~Defuzzifier() {
        }
        FL_DEFAULT_COPY_AND_MOVE(Defuzzifier)

        virtual std::string className() const = 0;
        virtual Defuzzifier* clone() const = 0;
        virtual scalar defuzzify(const Term* term, scalar minimum, scalar maximum) const = 0;

    };

}
#endif /* FL_DEFUZZIFIER_H */
