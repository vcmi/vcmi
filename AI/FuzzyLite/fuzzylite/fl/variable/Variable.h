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

#ifndef FL_VARIABLE_H
#define FL_VARIABLE_H

#include "fl/fuzzylite.h"

#include <string>
#include <vector>

namespace fl {

    class Term;

    class FL_API Variable {
    private:
        void copyFrom(const Variable& source);

    protected:
        std::string _name;
        std::vector<Term*> _terms;
        scalar _minimum, _maximum;
        bool _enabled;

    public:
        Variable(const std::string& name = "",
                scalar minimum = -fl::inf,
                scalar maximum = fl::inf);
        Variable(const Variable& other);
        Variable& operator=(const Variable& other);
        virtual ~Variable();
        FL_DEFAULT_MOVE(Variable)

        virtual void setName(const std::string& name);
        virtual std::string getName() const;

        virtual void setRange(scalar minimum, scalar maximum);
        virtual scalar range() const;

        virtual void setMinimum(scalar minimum);
        virtual scalar getMinimum() const;

        virtual void setMaximum(scalar maximum);
        virtual scalar getMaximum() const;

        virtual void setEnabled(bool enabled);
        virtual bool isEnabled() const;

        virtual std::string fuzzify(scalar x) const;
        virtual Term* highestMembership(scalar x, scalar* yhighest = fl::null) const;

        virtual std::string toString() const;

        /**
         * Operations for iterable datatype _terms
         */
        virtual void sort();
        virtual void addTerm(Term* term);
        virtual void insertTerm(Term* term, int index);
        virtual Term* getTerm(int index) const;
        virtual Term* getTerm(const std::string& name) const;
        virtual bool hasTerm(const std::string& name) const;
        virtual Term* removeTerm(int index);
        virtual int numberOfTerms() const;
        virtual void setTerms(const std::vector<Term*>& terms);
        virtual const std::vector<Term*>& terms() const;
        virtual std::vector<Term*>& terms();

    };

}

#endif /* FL_VARIABLE_H */
