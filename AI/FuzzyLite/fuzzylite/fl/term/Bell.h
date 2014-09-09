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

#ifndef FL_BELL_H
#define FL_BELL_H

#include "fl/term/Term.h"

namespace fl {

    class FL_API Bell : public Term {
    protected:
        scalar _center;
        scalar _width;
        scalar _slope;
    public:
        Bell(const std::string& name = "",
                scalar center = fl::nan,
                scalar width = fl::nan,
                scalar slope = fl::nan,
                scalar height = 1.0);
        virtual ~Bell() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Bell)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual void setCenter(scalar center);
        virtual scalar getCenter() const;

        virtual void setWidth(scalar width);
        virtual scalar getWidth() const;

        virtual void setSlope(scalar slope);
        virtual scalar getSlope() const;

        virtual Bell* clone() const FL_IOVERRIDE;

        static Term* constructor();

    };

}
#endif /* FL_BELL_H */
