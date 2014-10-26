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

#ifndef FL_FLLEXPORTER_H
#define FL_FLLEXPORTER_H

#include "fl/imex/Exporter.h"

#include <vector>

namespace fl {
    class Variable;
    class InputVariable;
    class OutputVariable;
    class RuleBlock;
    class Rule;
    class Norm;
    class Defuzzifier;
    class Term;

    class FL_API FllExporter : public Exporter {
    protected:
        std::string _indent;
        std::string _separator;
    public:
        FllExporter(const std::string& indent = "  ", const std::string& separator = "\n");
        virtual ~FllExporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FllExporter)

        virtual std::string name() const FL_IOVERRIDE;

        virtual void setIndent(const std::string& indent);
        virtual std::string getIndent() const;

        virtual void setSeparator(const std::string& separator);
        virtual std::string getSeparator() const;

        virtual std::string toString(const Engine* engine) const FL_IOVERRIDE;

        virtual std::string toString(const std::vector<Variable*>& variables) const;
        virtual std::string toString(const std::vector<InputVariable*>& inputVariables) const;
        virtual std::string toString(const std::vector<OutputVariable*>& outputVariables) const;
        virtual std::string toString(const std::vector<RuleBlock*>& ruleBlocks) const;

        virtual std::string toString(const Variable* variable) const;
        virtual std::string toString(const InputVariable* inputVariable) const;
        virtual std::string toString(const OutputVariable* outputVariable) const;

        virtual std::string toString(const RuleBlock* ruleBlock) const;
        virtual std::string toString(const Rule* rule) const;

        virtual std::string toString(const Norm* norm) const;
        virtual std::string toString(const Defuzzifier* defuzzifier) const;
        virtual std::string toString(const Term* term) const;

        virtual FllExporter* clone() const FL_IOVERRIDE;
    };

}

#endif  /* FL_FLLEXPORTER_H */

