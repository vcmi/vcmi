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

#ifndef FL_FUNCTIONFACTORY_H
#define FL_FUNCTIONFACTORY_H

#include "fl/factory/CloningFactory.h"

#include "fl/term/Function.h"

namespace fl {

    class FunctionFactory : public CloningFactory<Function::Element*> {
    public:
        FunctionFactory();
        virtual ~FunctionFactory() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FunctionFactory)

        virtual std::vector<std::string> availableOperators() const;
        virtual std::vector<std::string> availableFunctions() const;

    };

}

#endif  /* FL_FUNCTIONFACTORY_H */

