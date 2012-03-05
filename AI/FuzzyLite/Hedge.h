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
 * File:   Hedge.h
 * Author: jcrada
 *
 * Created on November 1, 2009, 2:55 PM
 */

#ifndef FL_HEDGE_H
#define	FL_HEDGE_H

#include <string>
#include <math.h>

#include "flScalar.h"
#include "defs.h"

namespace fl {

    class Hedge {
    public:
        virtual ~Hedge();
        virtual std::string name() const = 0;
        virtual flScalar hedge(flScalar mu) const = 0;
    };

    class HedgeNot : public Hedge {
    public:
        std::string name() const;
        flScalar hedge(flScalar mu) const;
    };

    class HedgeSomewhat : public Hedge {
    public:
        std::string name() const;
        flScalar hedge(flScalar mu) const;
    };

    class HedgeVery : public Hedge {
    public:
        std::string name() const;
        flScalar hedge(flScalar mu) const;
    };

    class HedgeAny : public Hedge {
    public:
        std::string name() const;
        flScalar hedge(flScalar mu) const;
    };
}

#endif	/* FL_HEDGE_H */

