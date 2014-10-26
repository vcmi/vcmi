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

#ifndef FL_ANTECEDENT_H
#define FL_ANTECEDENT_H

#include "fl/fuzzylite.h"

#include <string>

namespace fl {
    class Engine;
    class Rule;
    class TNorm;
    class SNorm;
    class Expression;

    class FL_API Antecedent {
    protected:
        std::string _text;
        Expression* _expression;

    public:
        Antecedent();
        virtual ~Antecedent();

        virtual void setText(const std::string& text);
        virtual std::string getText() const;

        virtual Expression* getExpression() const;

        virtual bool isLoaded() const;

        virtual void unload();
        virtual void load(Rule* rule, const Engine* engine);
        virtual void load(const std::string& antecedent, Rule* rule, const Engine* engine);

        virtual scalar activationDegree(const TNorm* conjunction, const SNorm* disjunction,
                const Expression* node) const;

        virtual scalar activationDegree(const TNorm* conjunction, const SNorm* disjunction) const;

        virtual std::string toString() const;

        virtual std::string toPrefix(const Expression* node = fl::null) const;
        virtual std::string toInfix(const Expression* node = fl::null) const;
        virtual std::string toPostfix(const Expression* node = fl::null) const;


    private:
        FL_DISABLE_COPY(Antecedent)
    };

}

#endif /* FL_ANTECEDENT_H */
