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

#ifndef FL_FISEXPORTER_H
#define FL_FISEXPORTER_H

#include "fl/imex/Exporter.h"

#include <vector>

namespace fl {
    class TNorm;
    class SNorm;
    class Defuzzifier;
    class Term;
    class Rule;
    class Proposition;
    class Variable;

    class FL_API FisExporter : public Exporter {
    protected:

        virtual std::string translate(const std::vector<Proposition*>& propositions,
                const std::vector<Variable*> variables) const;

    public:
        FisExporter();
        virtual ~FisExporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FisExporter)

        virtual std::string name() const FL_IOVERRIDE;
        virtual std::string toString(const Engine* engine) const FL_IOVERRIDE;

        virtual std::string toString(const TNorm* tnorm) const;
        virtual std::string toString(const SNorm* snorm) const;
        virtual std::string toString(const Defuzzifier* defuzzifier) const;
        virtual std::string toString(const Term* term) const;

        virtual std::string exportSystem(const Engine* engine) const;
        virtual std::string exportInputs(const Engine* engine) const;
        virtual std::string exportOutputs(const Engine* engine) const;
        virtual std::string exportRules(const Engine* engine) const;
        virtual std::string exportRule(const Rule* rule, const Engine* engine) const;

        virtual FisExporter* clone() const FL_IOVERRIDE;
    };

}

#endif  /* FL_FISEXPORTER_H */

