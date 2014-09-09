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

#ifndef FL_PISHAPE_H
#define FL_PISHAPE_H

#include "fl/term/Term.h"

namespace fl {

    class FL_API PiShape : public Term {
    protected:
        scalar _bottomLeft;
        scalar _topLeft;
        scalar _topRight;
        scalar _bottomRight;

    public:
        PiShape(const std::string& name = "",
                scalar bottomLeft = fl::nan,
                scalar topLeft = fl::nan,
                scalar topRight = fl::nan,
                scalar bottomRight = fl::nan,
                scalar height = 1.0);
        virtual ~PiShape() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(PiShape)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual void setBottomLeft(scalar a);
        virtual scalar getBottomLeft() const;

        virtual void setTopLeft(scalar b);
        virtual scalar getTopLeft() const;

        virtual void setTopRight(scalar d);
        virtual scalar getTopRight() const;

        virtual void setBottomRight(scalar c);
        virtual scalar getBottomRight() const;

        virtual PiShape* clone() const FL_IOVERRIDE;

        static Term* constructor();
    };
}

#endif  /* FL_PISHAPE_H */

