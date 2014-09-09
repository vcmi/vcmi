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

#include "fl/term/Term.h"

#include "fl/imex/FllExporter.h"
#include "fl/term/Linear.h"
#include "fl/term/Function.h"

namespace fl {

    Term::Term(const std::string& name, scalar height) : _name(name), _height(height) {

    }

    Term::~Term() {

    }

    void Term::setName(const std::string& name) {
        this->_name = name;
    }

    std::string Term::getName() const {
        return this->_name;
    }

    void Term::setHeight(scalar height) {
        this->_height = height;
    }

    scalar Term::getHeight() const {
        return this->_height;
    }

    std::string Term::toString() const {
        return FllExporter().toString(this);
    }

    void Term::updateReference(Term* term, const Engine* engine) {
        if (Linear * linear = dynamic_cast<Linear*> (term)) {
            linear->setEngine(engine);
        } else if (Function * function = dynamic_cast<Function*> (term)) {
            function->setEngine(engine);
            try {
                function->load();
            } catch (...) {
                //ignore
            }
        }
    }

}
