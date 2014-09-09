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

#include "fl/factory/DefuzzifierFactory.h"

#include "fl/defuzzifier/Bisector.h"
#include "fl/defuzzifier/Centroid.h"
#include "fl/defuzzifier/SmallestOfMaximum.h"
#include "fl/defuzzifier/LargestOfMaximum.h"
#include "fl/defuzzifier/MeanOfMaximum.h"
#include "fl/defuzzifier/WeightedAverage.h"
#include "fl/defuzzifier/WeightedSum.h"

namespace fl {

    DefuzzifierFactory::DefuzzifierFactory() : ConstructionFactory<Defuzzifier*>("Defuzzifier") {
        registerConstructor("", fl::null);
        registerConstructor(Bisector().className(), &(Bisector::constructor));
        registerConstructor(Centroid().className(), &(Centroid::constructor));
        registerConstructor(LargestOfMaximum().className(), &(LargestOfMaximum::constructor));
        registerConstructor(MeanOfMaximum().className(), &(MeanOfMaximum::constructor));
        registerConstructor(SmallestOfMaximum().className(), &(SmallestOfMaximum::constructor));
        registerConstructor(WeightedAverage().className(), &(WeightedAverage::constructor));
        registerConstructor(WeightedSum().className(), &(WeightedSum::constructor));
    }

    DefuzzifierFactory::~DefuzzifierFactory() {

    }

    Defuzzifier* DefuzzifierFactory::constructDefuzzifier(const std::string& key,
            int resolution, WeightedDefuzzifier::Type type) const {
        Defuzzifier* result = constructObject(key);
        if (IntegralDefuzzifier * integralDefuzzifier = dynamic_cast<IntegralDefuzzifier*> (result)) {
            integralDefuzzifier->setResolution(resolution);
        } else if (WeightedDefuzzifier * weightedDefuzzifier = dynamic_cast<WeightedDefuzzifier*> (result)) {
            weightedDefuzzifier->setType(type);
        }
        return result;
    }

    Defuzzifier* DefuzzifierFactory::constructDefuzzifier(const std::string& key, int resolution) const {
        Defuzzifier* result = constructObject(key);
        if (IntegralDefuzzifier * integralDefuzzifier = dynamic_cast<IntegralDefuzzifier*> (result)) {
            integralDefuzzifier->setResolution(resolution);
        }
        return result;
    }

    Defuzzifier* DefuzzifierFactory::constructDefuzzifier(const std::string& key, WeightedDefuzzifier::Type type) {
        Defuzzifier* result = constructObject(key);
        if (WeightedDefuzzifier * weightedDefuzzifier = dynamic_cast<WeightedDefuzzifier*> (result)) {
            weightedDefuzzifier->setType(type);
        }
        return result;
    }
}
