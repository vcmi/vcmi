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

#include "fl/term/Accumulated.h"

#include "fl/imex/FllExporter.h"
#include "fl/norm/SNorm.h"
#include "fl/norm/s/Maximum.h"
#include "fl/term/Activated.h"


namespace fl {

    Accumulated::Accumulated(const std::string& name, scalar minimum, scalar maximum,
            SNorm* accumulation)
    : Term(name), _minimum(minimum), _maximum(maximum), _accumulation(accumulation) {
    }

    Accumulated::Accumulated(const Accumulated& other) : Term(other) {
        copyFrom(other);
    }

    Accumulated& Accumulated::operator=(const Accumulated& other) {
        if (this != &other) {
            clear();
            _accumulation.reset(fl::null);

            Term::operator=(other);
            copyFrom(other);
        }
        return *this;
    }

    Accumulated::~Accumulated() {
        clear();
    }

    void Accumulated::copyFrom(const Accumulated& source) {
        _minimum = source._minimum;
        _maximum = source._maximum;

        if (source._accumulation.get())
            _accumulation.reset(source._accumulation->clone());

        for (std::size_t i = 0; i < source._terms.size(); ++i) {
            _terms.push_back(source._terms.at(i)->clone());
        }
    }

    std::string Accumulated::className() const {
        return "Accumulated";
    }

    scalar Accumulated::membership(scalar x) const {
        if (fl::Op::isNaN(x)) return fl::nan;
        if (not (_terms.empty() or _accumulation.get())) { //Exception for IntegralDefuzzifiers
            throw fl::Exception("[accumulation error] "
                    "accumulation operator needed to accumulate " + toString(), FL_AT);
        }
        scalar mu = 0.0;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            mu = _accumulation->compute(mu, _terms.at(i)->membership(x));
        }
        return mu;
    }

    scalar Accumulated::activationDegree(const Term* forTerm) const {
        scalar result = 0.0;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            Activated* activatedTerm = _terms.at(i);
            if (activatedTerm->getTerm() == forTerm) {
                if (_accumulation.get()) result = _accumulation->compute(result, activatedTerm->getDegree());
                else result += activatedTerm->getDegree(); //Default for WeightDefuzzifier
            }
        }
        return result;
    }

    std::string Accumulated::parameters() const {
        FllExporter exporter;
        std::ostringstream ss;
        ss << exporter.toString(_accumulation.get());
        ss << " " << Op::str(_minimum) << " " << Op::str(_maximum) << " ";
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            ss << " " << exporter.toString(_terms.at(i));
        }
        return ss.str();
    }

    void Accumulated::configure(const std::string& parameters) {
        (void) parameters;
    }

    Accumulated* Accumulated::clone() const {
        return new Accumulated(*this);
    }

    std::string Accumulated::toString() const {
        std::vector<std::string> accumulate;
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            accumulate.push_back(_terms.at(i)->toString());
        }
        FllExporter exporter;
        std::ostringstream ss;
        ss << _name << ": " << className() << " "
                << exporter.toString(_accumulation.get()) << "["
                << fl::Op::join(accumulate, ",") << "]";
        return ss.str();
    }

    void Accumulated::setMinimum(scalar minimum) {
        this->_minimum = minimum;
    }

    scalar Accumulated::getMinimum() const {
        return this->_minimum;
    }

    void Accumulated::setMaximum(scalar maximum) {
        this->_maximum = maximum;
    }

    scalar Accumulated::getMaximum() const {
        return this->_maximum;
    }

    void Accumulated::setRange(scalar minimum, scalar maximum) {
        setMinimum(minimum);
        setMaximum(maximum);
    }

    scalar Accumulated::range() const {
        return this->_maximum - this->_minimum;
    }

    void Accumulated::setAccumulation(SNorm* accumulation) {
        this->_accumulation.reset(accumulation);
    }

    SNorm* Accumulated::getAccumulation() const {
        return this->_accumulation.get();
    }

    /**
     * Operations for std::vector _terms
     */


    void Accumulated::addTerm(const Term* term, scalar degree, const TNorm* activation) {
        this->_terms.push_back(new Activated(term, degree, activation));
    }

    void Accumulated::addTerm(Activated* term) {
        this->_terms.push_back(term);
    }

    Activated* Accumulated::removeTerm(int index) {
        Activated* term = this->_terms.at(index);
        this->_terms.erase(this->_terms.begin() + index);
        return term;
    }

    void Accumulated::clear() {
        for (std::size_t i = 0; i < _terms.size(); ++i) {
            delete _terms.at(i);
        }
        _terms.clear();
    }

    Activated* Accumulated::getTerm(int index) const {
        return this->_terms.at(index);
    }

    const std::vector<Activated*>& Accumulated::terms() const {
        return this->_terms;
    }

    std::vector<Activated*>& Accumulated::terms() {
        return this->_terms;
    }

    int Accumulated::numberOfTerms() const {
        return _terms.size();
    }

    bool Accumulated::isEmpty() const {
        return _terms.empty();
    }

}
