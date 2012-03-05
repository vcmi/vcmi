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
 * File:   HedgeSet.h
 * Author: jcrada
 *
 * Created on March 5, 2010, 1:09 AM
 */

#ifndef FL_HEDGESET_H
#define	FL_HEDGESET_H

#include <vector>
#include "Hedge.h"
namespace fl{
    class HedgeSet{
    private:
        std::vector<Hedge*> _hedges;
    public:
        HedgeSet();
        virtual ~HedgeSet();

        virtual void add(Hedge* hedge);
        virtual Hedge* remove(int index);
        virtual Hedge* get(const std::string hedge) const;
        virtual Hedge* get(int index) const;
        virtual int indexOf(const std::string& hedge) const;
        virtual int numberOfHedges() const;

    };
}

#endif	/* FL_HEDGESET_H */

