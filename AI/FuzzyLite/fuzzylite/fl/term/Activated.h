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

#ifndef FL_ACTIVATED_H
#define FL_ACTIVATED_H

#include "fl/term/Term.h"

namespace fl {
    class TNorm;

    class FL_API Activated : public Term {
    protected:
        const Term* _term;
        scalar _degree;
        const TNorm* _activation;

    public:
        Activated(const Term* term = fl::null, scalar degree = 1.0,
                const TNorm* activationOperator = fl::null);
        virtual ~Activated() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Activated)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual scalar membership(scalar x) const FL_IOVERRIDE;
        virtual std::string toString() const FL_IOVERRIDE;

        virtual void setTerm(const Term* term);
        virtual const Term* getTerm() const;

        virtual void setDegree(scalar degree);
        virtual scalar getDegree() const;

        virtual void setActivation(const TNorm* activation);
        virtual const TNorm* getActivation() const;

        virtual Activated* clone() const FL_IOVERRIDE;
    };

}
#endif /* FL_ACTIVATED_H */
