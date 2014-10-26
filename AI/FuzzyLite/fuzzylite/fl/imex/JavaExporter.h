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

#ifndef FL_JAVAEXPORTER_H
#define FL_JAVAEXPORTER_H

#include "fl/imex/Exporter.h"

namespace fl {

    class Engine;
    class InputVariable;
    class OutputVariable;
    class RuleBlock;
    class Term;
    class Defuzzifier;
    class SNorm;
    class TNorm;

    class FL_API JavaExporter : public Exporter {
    public:
        JavaExporter();
        virtual ~JavaExporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(JavaExporter)

        virtual std::string name() const FL_IOVERRIDE;

        virtual std::string toString(const Engine* engine) const FL_IOVERRIDE;
        virtual std::string toString(const InputVariable* inputVariable, const Engine* engine) const;
        virtual std::string toString(const OutputVariable* outputVariable, const Engine* engine) const;
        virtual std::string toString(const RuleBlock* ruleBlock, const Engine* engine) const;
        virtual std::string toString(const Term* term) const;
        virtual std::string toString(const Defuzzifier* defuzzifier) const;
        virtual std::string toString(const SNorm* norm) const;
        virtual std::string toString(const TNorm* norm) const;
        virtual std::string toString(scalar value) const;

        virtual JavaExporter* clone() const FL_IOVERRIDE;

    };

}

#endif  /* FL_JAVAEXPORTER_H */

