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

#ifndef FL_GAUSSIANPRODUCT_H
#define FL_GAUSSIANPRODUCT_H

#include "fl/term/Term.h"

namespace fl {

    class FL_API GaussianProduct : public Term {
    protected:
        scalar _meanA;
        scalar _standardDeviationA;
        scalar _meanB;
        scalar _standardDeviationB;

    public:
        GaussianProduct(const std::string& name = "",
                scalar meanA = fl::nan,
                scalar standardDeviationA = fl::nan,
                scalar meanB = fl::nan,
                scalar standardDeviationB = fl::nan,
                scalar height = 1.0);
        virtual ~GaussianProduct() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(GaussianProduct)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual void setMeanA(scalar meanA);
        virtual scalar getMeanA() const;

        virtual void setStandardDeviationA(scalar sigmaA);
        virtual scalar getStandardDeviationA() const;

        virtual void setMeanB(scalar meanB);
        virtual scalar getMeanB() const;

        virtual void setStandardDeviationB(scalar sigmaB);
        virtual scalar getStandardDeviationB() const;

        virtual GaussianProduct* clone() const FL_IOVERRIDE;

        static Term* constructor();
    };
}
#endif  /* FL_GAUSSIANPRODUCT_H */

