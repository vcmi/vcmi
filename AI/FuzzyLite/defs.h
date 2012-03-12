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
 * File:   defs.h
 * Author: jcrada
 *
 * Created on October 30, 2009, 7:09 PM
 */

#ifndef FL_DEFS_H
#define	FL_DEFS_H

#ifdef _MSC_VER
    #ifndef NAN
    static const unsigned long __nan[2] = {0xffffffff, 0x7fffffff};
    #define NAN (*(const float *) __nan)
    #endif

    #ifndef INFINITY
    union MSVC_FL
    {
       unsigned __int8 Bytes[4];
       float Value;
    };
    static union MSVC_FL INFINITY_HACK = {{0x00, 0x00, 0x80, 0x7F}};
    #define INFINITY (INFINITY_HACK.Value)
    #endif
    #define isnan(x) ((x) != (x))
    template <typename T> bool isinf (T val)
    {
        return val == INFINITY || val == -INFINITY;
    }
    //#include <windows.h> //nees for S
#endif

#include <iostream>
#include <sstream>
#include <assert.h>
namespace fl {

#define FL_AT __FILE__, __LINE__, __FUNCTION__
 
#ifdef FL_USE_INLINE
#define FL_INLINE inline
#else
#define FL_INLINE
#endif

#ifdef FL_USE_ASSERT
#define FL_ASSERT(assertion) assert(assertion);
#define FL_ASSERTM(value,assertion) if (!(assertion)){FL_LOGM(value);} assert(assertion);
#else
#define FL_ASSERT(assertion)
#define FL_ASSERTM(value,assertion) 
#endif

#ifdef FL_USE_LOG
#define FL_LOG_PREFIX __FILE__ << " [" << __LINE__ << "]:"
#define FL_LOG(message) std::cout << FL_LOG_PREFIX << message << std::endl
#define FL_LOGW(message) std::cout << FL_LOG_PREFIX << "WARNING: " << message << std::endl
#else
#define FL_LOG(message) 
#define FL_LOGW(message) 
#endif


#ifdef FL_USE_DEBUG
#define FL_DEBUG_PREFIX __FILE__ << " [" << __LINE__ << "]::" << __FUNCTION__ << ": "
#define FL_DEBUG(message) std::cout << FL_DEBUG_PREFIX << message << std::endl

#define FL_DEBUGI  std::cout << FL_DEBUG_PREFIX << ++FL_DEBUG_COUNTER << std::endl
#else
#define FL_DEBUG(message)
#define FL_DEBUGI 
#endif

#ifndef FL_SAMPLE_SIZE
#define FL_SAMPLE_SIZE 100
#endif


}

#endif	/* FL_DEFS_H */

