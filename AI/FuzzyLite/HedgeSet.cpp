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
#include "HedgeSet.h"

namespace fl {

    HedgeSet::HedgeSet() {

    }

    HedgeSet::~HedgeSet() {

    }

    void HedgeSet::add(Hedge* hedge) {
        this->_hedges.push_back(hedge);
    }

    Hedge* HedgeSet::remove(int index) {
        Hedge* result = this->_hedges[index];
        this->_hedges.erase(this->_hedges.begin() + index);
        return result;
    }

    Hedge* HedgeSet::get(const std::string hedge) const {
        int index = indexOf(hedge);
        return index == -1 ? NULL : this->_hedges[index];
    }

    Hedge* HedgeSet::get(int index) const {
        return this->_hedges[index];
    }

    int HedgeSet::indexOf(const std::string& hedge) const {
        for (int i = 0; i < numberOfHedges(); ++i) {
            if (get(i)->name() == hedge) {
                return i;
            }
        }
        return -1;
    }

    int HedgeSet::numberOfHedges() const {
        return this->_hedges.size();
    }
}
