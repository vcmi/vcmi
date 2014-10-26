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

#ifndef FL_CLONINGFACTORY_H
#define FL_CLONINGFACTORY_H

#include "fl/fuzzylite.h"

#include <map>
#include <string>
#include <vector>

namespace fl {

    template <typename T>
    class FL_API CloningFactory {
    protected:
        std::string _name;
        std::map<std::string, T> _objects;

    public:
        CloningFactory(const std::string& name = "");
        CloningFactory(const CloningFactory& other);
        CloningFactory& operator=(const CloningFactory& other);
        virtual ~CloningFactory();
        FL_DEFAULT_MOVE(CloningFactory)

        virtual std::string name() const;

        virtual void registerObject(const std::string& key, T object);
        virtual void deregisterObject(const std::string& key);
        virtual bool hasObject(const std::string& key) const;
        virtual T getObject(const std::string& key) const;
        virtual T cloneObject(const std::string& key) const;
        virtual std::vector<std::string> available() const;

    };
}

#endif  /* FL_CLONINGFACTORY_H */

