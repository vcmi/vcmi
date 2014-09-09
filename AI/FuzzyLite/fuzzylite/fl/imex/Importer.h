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

#ifndef FL_IMPORTER_H
#define FL_IMPORTER_H

#include "fl/fuzzylite.h"

#include <string>

namespace fl {
    class Engine;

    class FL_API Importer {
    public:

        Importer();
        virtual ~Importer();
        FL_DEFAULT_COPY_AND_MOVE(Importer)

        virtual Engine* fromString(const std::string& s) const = 0;
        virtual Engine* fromFile(const std::string& path) const;

        virtual std::string name() const = 0;
        virtual Importer* clone() const = 0;
    };

}

#endif  /* IMPORTER_H */

