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

#include "fl/factory/CloningFactory.h"

#include "fl/Exception.h"
#include "fl/term/Function.h"

namespace fl {

    template<typename T>
    CloningFactory<T>::CloningFactory(const std::string& name) : _name(name) {

    }

    template<typename T>
    CloningFactory<T>::CloningFactory(const CloningFactory& other) {
        typename std::map<std::string, T>::const_iterator it = other._objects.begin();
        while (it != other._objects.end()) {
            T clone = fl::null;
            if (it->second) clone = it->second->clone();
            this->_objects[it->first] = clone;
            ++it;
        }
    }

    template<typename T>
    CloningFactory<T>& CloningFactory<T>::operator=(const CloningFactory& other) {
        if (this != &other) {
            typename std::map<std::string, T>::const_iterator it = this->_objects.begin();
            while (it != this->_objects.end()) {
                if (it->second) delete it->second;
                ++it;
            }
            this->_objects.clear();

            it = other._objects.begin();
            while (it != other._objects.end()) {
                T clone = fl::null;
                if (it->second) clone = it->second->clone();
                this->_objects[it->first] = clone;
                ++it;
            }
        }
        return *this;
    }

    template<typename T>
    CloningFactory<T>::~CloningFactory() {
        typename std::map<std::string, T>::const_iterator it = this->_objects.begin();
        while (it != this->_objects.end()) {
            if (it->second) delete it->second;
            ++it;
        }
    }

    template<typename T>
    std::string CloningFactory<T>::name() const {
        return this->_name;
    }

    template<typename T>
    void CloningFactory<T>::registerObject(const std::string& key, T object) {
        this->_objects[key] = object;
    }

    template<typename T>
    void CloningFactory<T>::deregisterObject(const std::string& key) {
        typename std::map<std::string, T>::iterator it = this->_objects.find(key);
        if (it != this->_objects.end()) {
            this->_objects.erase(it);
            delete it->second;
        }
    }

    template<typename T>
    bool CloningFactory<T>::hasObject(const std::string& key) const {
        typename std::map<std::string, T>::const_iterator it = this->_objects.find(key);
        return (it != this->_objects.end());
    }

    template<typename T>
    T CloningFactory<T>::getObject(const std::string& key) const {
        typename std::map<std::string, T>::const_iterator it = this->_objects.find(key);
        if (it != this->_objects.end()) {
            if (it->second) return it->second;
        }
        return fl::null;
    }

    template<typename T>
    T CloningFactory<T>::cloneObject(const std::string& key) const {
        typename std::map<std::string, T>::const_iterator it = this->_objects.find(key);
        if (it != this->_objects.end()) {
            if (it->second) return it->second->clone();
            return fl::null;
        }
        throw fl::Exception("[cloning error] " + _name + " object by name <" + key + "> not registered", FL_AT);
    }

    template<typename T>
    std::vector<std::string> CloningFactory<T>::available() const {
        std::vector<std::string> result;
        typename std::map<std::string, T>::const_iterator it = this->_objects.begin();
        while (it != this->_objects.end()) {
            result.push_back(it->first);
        }
        return result;
    }

    template class fl::CloningFactory<fl::Function::Element*>;
}


