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

#ifndef FL_LINEAR_H
#define FL_LINEAR_H

#include "fl/term/Term.h"

namespace fl {
    class Engine;

    class FL_API Linear : public Term {
    protected:
        std::vector<scalar> _coefficients;
        const Engine* _engine;
    public:
        Linear(const std::string& name = "",
                const std::vector<scalar>& coefficients = std::vector<scalar>(),
                const Engine* engine = fl::null);
        virtual ~Linear() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Linear)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual void set(const std::vector<scalar>& coeffs, const Engine* engine);

        virtual void setCoefficients(const std::vector<scalar>& coeffs);
        virtual const std::vector<scalar>& coefficients() const;
        virtual std::vector<scalar>& coefficients();

        virtual void setEngine(const Engine* engine);
        virtual const Engine* getEngine() const;

        virtual Linear* clone() const FL_IOVERRIDE;

        static Term* constructor();

        //Warning: this method is unsafe, make sure you use it correctly.
        template <typename T>
        static Linear* create(const std::string& name, const Engine* engine,
                T firstCoefficient, ...); // throw (fl::Exception);
    };

}

#endif  /* FL_LINEAR_H */

