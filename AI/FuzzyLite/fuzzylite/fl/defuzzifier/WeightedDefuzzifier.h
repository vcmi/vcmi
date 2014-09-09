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

#ifndef FL_WEIGHTEDDEFUZZIFIER_H
#define FL_WEIGHTEDDEFUZZIFIER_H

#include "fl/defuzzifier/Defuzzifier.h"

namespace fl {
    class Activated;

    class FL_API WeightedDefuzzifier : public Defuzzifier {
    public:

        enum Type {
            Automatic, TakagiSugeno, Tsukamoto
        };
        static std::string typeName(Type);

        WeightedDefuzzifier(Type type = Automatic);
        WeightedDefuzzifier(const std::string& type);
        virtual ~WeightedDefuzzifier() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(WeightedDefuzzifier)

        virtual void setType(Type type);
        virtual Type getType() const;
        virtual std::string getTypeName() const;
        virtual Type inferType(const Term* term) const;
        virtual bool isMonotonic(const Term* term) const;

        virtual scalar tsukamoto(const Term* monotonic, scalar activationDegree,
                scalar minimum, scalar maximum) const;

    protected:
        Type _type;

    };

}

#endif  /* FL_WEIGHTEDDEFUZZIFIER_H */

