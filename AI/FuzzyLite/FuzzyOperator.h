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
/* 
 * File:   FuzzyOperator.h
 * Author: jcrada
 *
 * Created on November 1, 2009, 3:05 PM
 */

#ifndef FL_FUZZYOPERATOR_H
#define	FL_FUZZYOPERATOR_H

#include "defs.h"
#include "flScalar.h"
#include "FuzzyOperation.h"
#include "FuzzyAnd.h"
#include "FuzzyOr.h"
#include "FuzzyModulation.h"
#include "AreaCentroidAlgorithm.h"
#include "FuzzyDefuzzifier.h"

namespace fl {
    class LinguisticTerm;
    
    class FuzzyOperator {
    private:
        const FuzzyOperation* _tnorm;
        const FuzzyOperation* _snorm;
        const FuzzyOperation* _cmodulation;
        const FuzzyOperation* _aggregation;
        const FuzzyDefuzzifier* _defuzzifier;
        const AreaCentroidAlgorithm* _acalgorithm;
        int _number_of_samples;
    public:
        FuzzyOperator(const FuzzyOperation* tnorm = new FuzzyAndMin,
                const FuzzyOperation* snorm = new FuzzyOrMax,
                const FuzzyOperation* cmod = new FuzzyModClip,
                const FuzzyOperation* aggregation = new FuzzyOrMax,
                const FuzzyDefuzzifier* defuzzifier = new CoGDefuzzifier,
                const AreaCentroidAlgorithm* algorithm = new TriangulationAlgorithm, //bugfix
                int number_of_samples = FL_SAMPLE_SIZE);
        virtual ~FuzzyOperator();

        virtual void setTnorm(const FuzzyOperation* tnorm);
        virtual void setSnorm(const FuzzyOperation* snorm);
        virtual void setModulation(const FuzzyOperation* modulation);
        virtual void setAggregation(const FuzzyOperation* aggregation);
        virtual void setDefuzzifier(const FuzzyDefuzzifier* defuzzifier);
        virtual void setAreaCentroidAlgorithm(const AreaCentroidAlgorithm* algorithm);
        virtual void setNumberOfSamples(int samples);

        virtual const FuzzyOperation& tnorm() const;
        virtual const FuzzyOperation& snorm() const;
        virtual const FuzzyOperation& modulation() const;
        virtual const FuzzyOperation& aggregation() const;
        virtual const FuzzyDefuzzifier& defuzzifier() const;
        virtual const AreaCentroidAlgorithm& acAlgorithm() const;
        virtual int numberOfSamples() const;

        
        virtual flScalar defuzzify(const LinguisticTerm* term) const;
        virtual flScalar area(const LinguisticTerm* term) const;
        virtual void centroid(const LinguisticTerm* term, flScalar& x, flScalar& y) const;
        virtual flScalar areaAndCentroid(const LinguisticTerm* term, flScalar& x, flScalar& y) const;

        static FuzzyOperator& DefaultFuzzyOperator();

    };
}

#endif	/* FL_FUZZYOPERATOR_H */

