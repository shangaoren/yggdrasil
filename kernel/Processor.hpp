/*MIT License

Copyright (c) 2018 Florian GERARD

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Except as contained in this notice, the name of Florian GERARD shall not be used 
in advertising or otherwise to promote the sale, use or other dealings in this 
Software without prior written authorization from Florian GERARD

*/

#pragma once

#ifdef STM32L432xx
#include "../processor/st/l4/stm32l432xx.h"
#define VECTOR_TABLE_SIZE (99)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif 

#ifdef STM32F411xe
#include "../processor/st/f4/stm32f411xe.h"
#define VECTOR_TABLE_SIZE (98)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32L496xx
#include "../processor/st/l4/stm32l496xx.h"
#define VECTOR_TABLE_SIZE (106)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32F303x8
#include "../processor/st/f3/stm32f303x8.h"
#define VECTOR_TABLE_SIZE (97)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32L476xx
#include "../processor/st/l4/stm32l476xx.h"
#define VECTOR_TABLE_SIZE (98)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32L475xx
#include "../processor/st/l4/stm32l475xx.h"
#define VECTOR_TABLE_SIZE (98)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32F303xE
#include "../processor/st/f3/stm32f303xe.h"
#define VECTOR_TABLE_SIZE (101)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif

#ifdef STM32F100C8
#include "../processor/st/f1/stm32f100xb.h"
#define VECTOR_TABLE_SIZE (72)
#define VECTOR_TABLE_ALIGNEMENT (512)
#endif
