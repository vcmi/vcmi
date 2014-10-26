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

#include "fl/factory/TNormFactory.h"

#include "fl/norm/t/AlgebraicProduct.h"
#include "fl/norm/t/BoundedDifference.h"
#include "fl/norm/t/DrasticProduct.h"
#include "fl/norm/t/EinsteinProduct.h"
#include "fl/norm/t/HamacherProduct.h"
#include "fl/norm/t/Minimum.h"
#include "fl/norm/t/NilpotentMinimum.h"

namespace fl {

    TNormFactory::TNormFactory() : ConstructionFactory<TNorm*>("TNorm") {
        registerConstructor("", fl::null);
        registerConstructor(AlgebraicProduct().className(), &(AlgebraicProduct::constructor));
        registerConstructor(BoundedDifference().className(), &(BoundedDifference::constructor));
        registerConstructor(DrasticProduct().className(), &(DrasticProduct::constructor));
        registerConstructor(EinsteinProduct().className(), &(EinsteinProduct::constructor));
        registerConstructor(HamacherProduct().className(), &(HamacherProduct::constructor));
        registerConstructor(Minimum().className(), &(Minimum::constructor));
        registerConstructor(NilpotentMinimum().className(), &(NilpotentMinimum::constructor));
    }

    TNormFactory::~TNormFactory() {

    }


}
