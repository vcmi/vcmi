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

#include "fl/factory/SNormFactory.h"

#include "fl/norm/s/AlgebraicSum.h"
#include "fl/norm/s/BoundedSum.h"
#include "fl/norm/s/DrasticSum.h"
#include "fl/norm/s/EinsteinSum.h"
#include "fl/norm/s/HamacherSum.h"
#include "fl/norm/s/Maximum.h"
#include "fl/norm/s/NilpotentMaximum.h"
#include "fl/norm/s/NormalizedSum.h"

namespace fl {

    SNormFactory::SNormFactory() : ConstructionFactory<SNorm*>("SNorm") {
        registerConstructor("", fl::null);
        registerConstructor(AlgebraicSum().className(), &(AlgebraicSum::constructor));
        registerConstructor(BoundedSum().className(), &(BoundedSum::constructor));
        registerConstructor(DrasticSum().className(), &(DrasticSum::constructor));
        registerConstructor(EinsteinSum().className(), &(EinsteinSum::constructor));
        registerConstructor(HamacherSum().className(), &(HamacherSum::constructor));
        registerConstructor(Maximum().className(), &(Maximum::constructor));
        registerConstructor(NilpotentMaximum().className(), &(NilpotentMaximum::constructor));
        registerConstructor(NormalizedSum().className(), &(NormalizedSum::constructor));
    }

    SNormFactory::~SNormFactory() {

    }


}
