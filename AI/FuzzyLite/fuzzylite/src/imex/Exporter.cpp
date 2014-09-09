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

#include "fl/imex/Exporter.h"
#include "fl/Exception.h"

#include <fstream>

namespace fl {

    Exporter::Exporter() {

    }

    Exporter::~Exporter() {

    }

    void Exporter::toFile(const std::string& path, const Engine* engine) const {
        std::ofstream writer(path.c_str());
        if (not writer.is_open()) {
            throw fl::Exception("[file error] file <" + path + "> could not be created", FL_AT);
        }
        writer << toString(engine) << std::endl;
        writer.close();
    }

}
