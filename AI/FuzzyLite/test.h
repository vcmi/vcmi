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
 * File:   test.h
 * Author: jcrada
 *
 * Created on December 8, 2009, 11:11 PM
 */

#ifndef FL_TEST_H
#define	FL_TEST_H

namespace fl {

    class Test {
    public:
        static void SimpleMamdani();
        static void ComplexMamdani();
        static void SimpleTakagiSugeno();
        static void SimplePendulum();
        static void main(int args, char** argv);
    };
}


#endif	/* FL_TEST_H */

