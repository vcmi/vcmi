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
 * File:   main.cpp
 * Author: jcrada
 *
 * Created on October 28, 2009, 2:29 PM
 */

#include <stdlib.h>
#include <vector>
#include "FuzzyLite.h"
#include "test.h"
#include <iomanip>

int main(int argc, char** argv) {
    FL_LOG(std::fixed << std::setprecision(3));
    fl::Test::main(argc, argv);

    return (EXIT_SUCCESS);
}

