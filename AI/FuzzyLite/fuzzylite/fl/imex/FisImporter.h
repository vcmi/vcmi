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

#ifndef FL_FISIMPORTER_H
#define FL_FISIMPORTER_H

#include "fl/imex/Importer.h"

#include <utility>
#include <vector>


namespace fl {
    class Norm;
    class TNorm;
    class SNorm;
    class Term;
    class Defuzzifier;
    class Variable;

    class FL_API FisImporter : public Importer {
    public:
        FisImporter();
        virtual ~FisImporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FisImporter)

        virtual std::string name() const FL_IOVERRIDE;

        virtual Engine* fromString(const std::string& fcl) const FL_IOVERRIDE;

        virtual FisImporter* clone() const FL_IOVERRIDE;

    protected:
        virtual void importSystem(const std::string& section, Engine* engine,
                std::string& andMethod, std::string& orMethod,
                std::string& impMethod, std::string& aggMethod,
                std::string& defuzzMethod) const;
        virtual void importInput(const std::string& section, Engine* engine) const;
        virtual void importOutput(const std::string& section, Engine* engine) const;
        virtual void importRules(const std::string& section, Engine* engine) const;
        virtual std::string translateProposition(scalar code, Variable* variable) const;

        virtual std::string extractTNorm(const std::string& tnorm) const;
        virtual std::string extractSNorm(const std::string& tnorm) const;
        virtual std::string extractDefuzzifier(const std::string& defuzzifier) const;

        virtual Term* parseTerm(const std::string& line, const Engine* engine) const;
        virtual Term* createInstance(const std::string& termClass, const std::string& name,
                const std::vector<std::string>& params, const Engine* engine) const;
        virtual std::pair<scalar, scalar> range(const std::string& range) const;

    };

}
#endif  /* FL_FISIMPORTER_H */

