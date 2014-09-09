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

#ifndef FL_EXPRESSION_H
#define FL_EXPRESSION_H

#include "fl/fuzzylite.h"

#include <string>
#include <vector>



namespace fl {
    class Variable;
    class Hedge;
    class Term;

    class FL_API Expression {
    public:

        Expression();
        virtual ~Expression();

        virtual std::string toString() const = 0;

    private:
        FL_DISABLE_COPY(Expression)
    };

    class FL_API Proposition : public Expression {
    public:
        Variable* variable;
        std::vector<Hedge*> hedges;
        Term* term;

        Proposition();
        virtual ~Proposition() FL_IOVERRIDE;

        virtual std::string toString() const FL_IOVERRIDE;

    private:
        FL_DISABLE_COPY(Proposition)
    };

    class FL_API Operator : public Expression {
    public:
        std::string name;
        Expression* left;
        Expression* right;

        Operator();
        virtual ~Operator() FL_IOVERRIDE;

        virtual std::string toString() const FL_IOVERRIDE;

    private:
        FL_DISABLE_COPY(Operator)

    };

}
#endif /* FL_FUZZYEXPRESSION_H */
