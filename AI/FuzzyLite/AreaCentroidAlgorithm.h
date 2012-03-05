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
 * File:   AreaCentroidAlgorithm.h
 * Author: jcrada
 *
 * Created on April 6, 2010, 5:33 PM
 */

#ifndef FL_AREACENTROIDALGORITHM_H
#define	FL_AREACENTROIDALGORITHM_H


#include "flScalar.h"
#include <string>

namespace fl {

    class LinguisticTerm;
    
    class AreaCentroidAlgorithm {
    public:

        virtual ~AreaCentroidAlgorithm(){}

        virtual std::string name() const = 0;

        virtual flScalar area(const LinguisticTerm* term, int samples) const = 0;
        virtual void centroid(const LinguisticTerm* term, flScalar& centroid_x, flScalar& centroid_y,
                int samples) const = 0;
        virtual flScalar areaAndCentroid(const LinguisticTerm* term, flScalar& centroid_x,
                flScalar& centroid_y, int samples) const = 0;
    };

    class TriangulationAlgorithm : public AreaCentroidAlgorithm {
    public:
        virtual std::string name() const;

        virtual flScalar area(const LinguisticTerm* term, int samples) const;
        virtual void centroid(const LinguisticTerm* term, flScalar& centroid_x, flScalar& centroid_y,
                int samples) const;
        virtual flScalar areaAndCentroid(const LinguisticTerm* term, flScalar& centroid_x,
                flScalar& centroid_y, int samples) const;
    };

    class TrapezoidalAlgorithm : public AreaCentroidAlgorithm {
    	//Feature suggested by William Roeder
    public:
        virtual std::string name() const;

        virtual flScalar area(const LinguisticTerm* term, int samples) const;
        virtual void centroid(const LinguisticTerm* term, flScalar& centroid_x, flScalar& centroid_y,
                int samples) const;
        virtual flScalar areaAndCentroid(const LinguisticTerm* term, flScalar& centroid_x,
                flScalar& centroid_y, int samples) const;
    };


}


#endif	/* FL_AREACENTROIDALGORITHM_H */

