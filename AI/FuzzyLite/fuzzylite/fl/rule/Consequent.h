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

#ifndef FL_CONSEQUENT_H
#define FL_CONSEQUENT_H

#include "fl/fuzzylite.h"

#include <string>
#include <vector>

namespace fl {
    class Engine;
    class Rule;
    class Proposition;
    class TNorm;

    class FL_API Consequent {
    protected:
        std::string _text;
        std::vector<Proposition*> _conclusions;

    public:
        Consequent();
        virtual ~Consequent();

        virtual void setText(const std::string& text);
        virtual std::string getText() const;

        virtual const std::vector<Proposition*>& conclusions() const;

        virtual bool isLoaded();
        virtual void unload();
        virtual void load(Rule* rule, const Engine* engine);
        virtual void load(const std::string& consequent, Rule* rule, const Engine* engine);

        virtual void modify(scalar activationDegree, const TNorm* activation);

        virtual std::string toString() const;

    private:
        FL_DISABLE_COPY(Consequent)
    };

}

#endif /* FL_CONSEQUENT_H */
