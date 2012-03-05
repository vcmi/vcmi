/*   Copyright 2010 Juan Rada-Vilela

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
#include "FuzzyOperator.h"
#include "StrOp.h"

#include <limits>
#include <math.h>

#include "CompoundTerm.h"
#include "TakagiSugenoTerm.h"

#include "MamdaniRule.h"
namespace fl {

    FuzzyOperator::FuzzyOperator(const FuzzyOperation* tnorm,
            const FuzzyOperation* snorm,
            const FuzzyOperation* cmod,
            const FuzzyOperation* aggregation,
            const FuzzyDefuzzifier* defuzzifier,
            const AreaCentroidAlgorithm* algorithm,
            int number_of_samples)
    : _tnorm(tnorm), _snorm(snorm), _cmodulation(cmod), _aggregation(aggregation),
    _defuzzifier(defuzzifier), _acalgorithm(algorithm),
    _number_of_samples(number_of_samples) {
    }

    FuzzyOperator::~FuzzyOperator() {
    }

    void FuzzyOperator::setTnorm(const FuzzyOperation* tnorm) {
        delete this->_tnorm;
        this->_tnorm = tnorm;
    }

    void FuzzyOperator::setSnorm(const FuzzyOperation* snorm) {
        delete this->_snorm;
        this->_snorm = snorm;
    }

    void FuzzyOperator::setModulation(const FuzzyOperation* modulation) {
        delete this->_cmodulation;
        this->_cmodulation = modulation;
    }

    void FuzzyOperator::setAggregation(const FuzzyOperation* aggregation) {
        delete this->_aggregation;
        this->_aggregation = aggregation;
    }

    void FuzzyOperator::setDefuzzifier(const FuzzyDefuzzifier* defuzzifier) {
        delete this->_defuzzifier;
        this->_defuzzifier = defuzzifier;
    }

    void FuzzyOperator::setAreaCentroidAlgorithm(const AreaCentroidAlgorithm* algorithm) {
        delete this->_acalgorithm;
        this->_acalgorithm = algorithm;
    }

    void FuzzyOperator::setNumberOfSamples(int samples) {
        this->_number_of_samples = samples;
    }

    const FuzzyOperation& FuzzyOperator::tnorm() const {
        return *this->_tnorm;
    }

    const FuzzyOperation& FuzzyOperator::snorm() const {
        return *this->_snorm;
    }

    const FuzzyOperation& FuzzyOperator::modulation() const {
        return *this->_cmodulation;
    }

    const FuzzyOperation& FuzzyOperator::aggregation() const {
        return *this->_aggregation;
    }

    const FuzzyDefuzzifier& FuzzyOperator::defuzzifier() const {
        return *this->_defuzzifier;
    }

    const AreaCentroidAlgorithm& FuzzyOperator::acAlgorithm() const {
        return *this->_acalgorithm;
    }

    int FuzzyOperator::numberOfSamples() const {
        return this->_number_of_samples;
    }

    flScalar FuzzyOperator::defuzzify(const LinguisticTerm* term) const {
        return defuzzifier().defuzzify(term, numberOfSamples(), &acAlgorithm());
    }

    flScalar FuzzyOperator::area(const LinguisticTerm* term) const {
        return acAlgorithm().area(term, numberOfSamples());
    }

    void FuzzyOperator::centroid(const LinguisticTerm* term, flScalar& x, flScalar& y) const {
        acAlgorithm().centroid(term, x, y, numberOfSamples());
    }

    flScalar FuzzyOperator::areaAndCentroid(const LinguisticTerm* term, flScalar& x, flScalar& y) const {
        return acAlgorithm().areaAndCentroid(term, x, y, numberOfSamples());
    }

    FuzzyOperator& FuzzyOperator::DefaultFuzzyOperator() {
        static FuzzyOperator* FUZZY_OP = new fl::FuzzyOperator;
        return *FUZZY_OP;
    }

}
