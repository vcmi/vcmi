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

#ifndef FL_DEFUZZIFIERFACTORY_H
#define FL_DEFUZZIFIERFACTORY_H

#include "fl/factory/ConstructionFactory.h"

#include "fl/defuzzifier/Defuzzifier.h"
#include "fl/defuzzifier/IntegralDefuzzifier.h"
#include "fl/defuzzifier/WeightedDefuzzifier.h"

namespace fl {

    class FL_API DefuzzifierFactory : public ConstructionFactory<Defuzzifier*> {
    public:
        DefuzzifierFactory();
        virtual ~DefuzzifierFactory() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(DefuzzifierFactory)

        virtual Defuzzifier* constructDefuzzifier(const std::string& key,
                int resolution, WeightedDefuzzifier::Type) const;

        virtual Defuzzifier* constructDefuzzifier(const std::string& key, int resolution) const;

        virtual Defuzzifier* constructDefuzzifier(const std::string& key, WeightedDefuzzifier::Type type);
    };
}
#endif  /* DEFUZZIFIERFACTORY_H */

