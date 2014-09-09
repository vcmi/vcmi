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

#ifndef FL_FCLIMPORTER_H
#define FL_FCLIMPORTER_H

#include "fl/imex/Importer.h"

#include <string>
#include <utility>
#include <vector>


namespace fl {
    class Norm;
    class TNorm;
    class SNorm;
    class Term;
    class Defuzzifier;

    class FL_API FclImporter : public Importer {
    public:
        FclImporter();
        virtual ~FclImporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FclImporter)

        virtual std::string name() const FL_IOVERRIDE;

        virtual Engine* fromString(const std::string& fcl) const FL_IOVERRIDE;

        virtual FclImporter* clone() const FL_IOVERRIDE;

    protected:
        virtual void processBlock(const std::string& tag, const std::string& block, Engine* engine) const;
        virtual void processVar(const std::string& var, const std::string& block, Engine* engine)const;
        virtual void processFuzzify(const std::string& block, Engine* engine)const;
        virtual void processDefuzzify(const std::string& block, Engine* engine)const;
        virtual void processRuleBlock(const std::string& block, Engine* engine)const;

        virtual TNorm* parseTNorm(const std::string& line) const;
        virtual SNorm* parseSNorm(const std::string& line) const;
        virtual Term* parseTerm(const std::string& line, const Engine* engine) const;

        virtual Defuzzifier* parseDefuzzifier(const std::string& line) const;
        virtual std::pair<scalar, bool> parseDefaultValue(const std::string& line) const;
        virtual std::pair<scalar, scalar> parseRange(const std::string& line) const;
        virtual std::pair<bool, bool> parseLocks(const std::string& line) const;
        virtual bool parseEnabled(const std::string& line) const;

    };

}
#endif /* FL_FCLIMPORTER_H */
