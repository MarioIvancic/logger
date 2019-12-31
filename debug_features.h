/*  Copyright (c) 2014, 2019, Mario Ivančić
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// debug_features.h

#ifndef __DEBUG_FEATURES_H__
#define __DEBUG_FEATURES_H__

#ifdef __cplusplus
extern "C" {
#endif


// arbitrary debug features, must have unique single bit values
#define CSVDEBUG    (1 << 0)
#define XMLDEBUG    (1 << 1)
#define VPSTACK     (1 << 2)
#define VARTREE     (1 << 3)
#define LISTEND     (1 << 4)
#define INTARRAY    (1 << 5)
#define CALLTRACE   (1 << 6)
#define SQLDEBUG    (1 << 7)
#define VARDEBUG    (1 << 8)

#ifdef __cplusplus
}
#endif

#endif // __DEBUG_FEATURES_H__

