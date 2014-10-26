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

#ifndef FL_FLLIMPORTER_H
#define FL_FLLIMPORTER_H

#include "fl/imex/Importer.h"

#include <utility>

namespace fl {
    class TNorm;
    class SNorm;
    class Term;
    class Defuzzifier;

    class FL_API FllImporter : public Importer {
    protected:
        std::string _separator;
    public:
        FllImporter(const std::string& separator = "\n");
        virtual ~FllImporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FllImporter)

        virtual void setSeparator(const std::string& separator);
        virtual std::string getSeparator() const;

        virtual std::string name() const FL_IOVERRIDE;
        virtual Engine* fromString(const std::string& fll) const FL_IOVERRIDE;

        virtual FllImporter* clone() const FL_IOVERRIDE;

    protected:
        virtual void process(const std::string& tag, const std::string& block, Engine* engine) const;
        virtual void processInputVariable(const std::string& block, Engine* engine) const;
        virtual void processOutputVariable(const std::string& block, Engine* engine) const;
        virtual void processRuleBlock(const std::string& block, Engine* engine) const;

        virtual TNorm* parseTNorm(const std::string& name) const;
        virtual SNorm* parseSNorm(const std::string& name) const;

        virtual Term* parseTerm(const std::string& text, Engine* engine) const;

        virtual Defuzzifier* parseDefuzzifier(const std::string& line) const;
        virtual std::pair<scalar, scalar> parseRange(const std::string& line) const;
        virtual bool parseBoolean(const std::string& boolean) const;

        virtual std::pair<std::string, std::string> parseKeyValue(const std::string& text,
                char separator = ':') const;
        virtual std::string clean(const std::string& line) const;

    };
}

#endif  /* FL_FLLIMPORTER_H */

