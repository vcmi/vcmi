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

#include "fl/variable/Variable.h"

#include "fl/defuzzifier/Centroid.h"
#include "fl/imex/FllExporter.h"
#include "fl/norm/Norm.h"
#include "fl/term/Constant.h"
#include "fl/term/Linear.h"
#include "fl/term/Term.h"

#include <algorithm>
#include <map>
#include <sstream>

namespace fl {

    Variable::Variable(const std::string& name, scalar minimum, scalar maximum)
    : _name(name), _minimum(minimum), _maximum(maximum), _enabled(true) {
    }

    Variable::Variable(const Variable& other) {
        copyFrom(other);
    }

    Variable& Variable::operator=(const Variable& other) {
        if (this != &other) {
            for (std::size_t i = 0; i < _terms.size(); ++i) {
                delete _terms.at(i);
            }
            _terms.clear();
            copyFrom(other);
        }
        return *this;
    }

    void Variable::copyFrom(const Variable& other) {
        _name = other._name;
        _enabled = other._enabled;
        _minimum = other._minimum;
        _maximum = other._maximum;
        for (std::size_t i = 0; i < other._terms.size(); ++i) {
            _terms.push_back(other._terms.at(i)->clone());
        }
    }

    Variable::~Variable() {
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            delete _terms.at(i);
        }
    }

    void Variable::setName(const std::string& name) {
        this->_name = name;
    }

    std::string Variable::getName() const {
        return this->_name;
    }

    void Variable::setRange(scalar minimum, scalar maximum) {
        setMinimum(minimum);
        setMaximum(maximum);
    }

    scalar Variable::range() const {
        return this->_maximum - this->_minimum;
    }

    void Variable::setMinimum(scalar minimum) {
        this->_minimum = minimum;
    }

    scalar Variable::getMinimum() const {
        return this->_minimum;
    }

    void Variable::setMaximum(scalar maximum) {
        this->_maximum = maximum;
    }

    scalar Variable::getMaximum() const {
        return this->_maximum;
    }

    void Variable::setEnabled(bool enabled) {
        this->_enabled = enabled;
    }

    bool Variable::isEnabled() const {
        return this->_enabled;
    }

    std::string Variable::fuzzify(scalar x) const {
        std::ostringstream ss;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            scalar fx = fl::nan;
            try {
                fx = _terms.at(i)->membership(x);
            } catch (...) {
                //ignore
            }
            if (i == 0) {
                ss << fl::Op::str(fx);
            } else {
                if (fl::Op::isNaN(fx) or fl::Op::isGE(fx, 0.0))
                    ss << " + " << fl::Op::str(fx);
                else
                    ss << " - " << fl::Op::str(std::fabs(fx));
            }
            ss << "/" << _terms.at(i)->getName();
        }
        return ss.str();
    }

    Term* Variable::highestMembership(scalar x, scalar* yhighest) const {
        Term* result = fl::null;
        scalar ymax = 0.0;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            scalar y = fl::nan;
            try {
                y = _terms.at(i)->membership(x);
            } catch (...) {
                //ignore
            }
            if (fl::Op::isGt(y, ymax)) {
                ymax = y;
                result = _terms.at(i);
            }
        }
        if (yhighest) *yhighest = ymax;
        return result;
    }

    std::string Variable::toString() const {
        return FllExporter().toString(this);
    }

    /**
     * Operations for datatype _terms
     */

    struct SortByCoG {
        std::map<const Term*, scalar> centroids;

        bool operator()(const Term* a, const Term * b) {
            return fl::Op::isLt(
                    centroids.find(a)->second,
                    centroids.find(b)->second);
        }
    };

    void Variable::sort() {
        std::map<const Term*, scalar> centroids;
        Centroid defuzzifier;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            Term* term = _terms.at(i);
            try {
                if (dynamic_cast<const Constant*> (term) or dynamic_cast<const Linear*> (term)) {
                    centroids[term] = term->membership(0);
                } else {
                    centroids[term] = defuzzifier.defuzzify(term, _minimum, _maximum);
                }
            } catch (...) { //ignore error possibly due to Function not loaded
                centroids[term] = fl::inf;
            }
        }
        SortByCoG criterion;
        criterion.centroids = centroids;
        std::sort(_terms.begin(), _terms.end(), criterion);
    }

    void Variable::addTerm(Term* term) {
        this->_terms.push_back(term);
    }

    void Variable::insertTerm(Term* term, int index) {
        this->_terms.insert(this->_terms.begin() + index, term);
    }

    Term* Variable::getTerm(int index) const {
        return this->_terms.at(index);
    }

    Term* Variable::getTerm(const std::string& name) const {
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            if (_terms.at(i)->getName() == name) {
                return _terms.at(i);
            }
        }
        throw fl::Exception("[variable error] term <" + name + "> "
                "not found in variable <" + this->_name + ">", FL_AT);
    }

    bool Variable::hasTerm(const std::string& name) const {
        return getTerm(name) != fl::null;
    }

    Term* Variable::removeTerm(int index) {
        Term* result = this->_terms.at(index);
        this->_terms.erase(this->_terms.begin() + index);
        return result;
    }

    int Variable::numberOfTerms() const {
        return this->_terms.size();
    }

    const std::vector<Term*>& Variable::terms() const {
        return this->_terms;
    }

    void Variable::setTerms(const std::vector<Term*>& terms) {
        this->_terms = terms;
    }

    std::vector<Term*>& Variable::terms() {
        return this->_terms;
    }

}

