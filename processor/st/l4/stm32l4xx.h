/**
  ******************************************************************************
  * @file    stm32l4xx.h
  * @author  MCD Application Team
  * @brief   CMSIS STM32L4xx Device Peripheral Access Layer Header File.
  *
  *          The file is the unique include file that the application programmer
  *          is using in the C source code, usually in main.c. This file contains:
  *           - Configuration section that allows to select:
  *              - The STM32L4xx device used in the target application
  *              - To use or not the peripheral�s drivers in application code(i.e.
  *                code will be based on direct access to peripheral�s registers
  *                rather than drivers API), this option is controlled by
  *                "#define USE_HAL_DRIVER"
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#pragma once

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

#ifdef STM32L4

/**
  * @brief CMSIS Device version number $VERSION$
  */
#define __STM32L4_CMSIS_VERSION_MAIN   (0x01) /*!< [31:24] main version */
#define __STM32L4_CMSIS_VERSION_SUB1   (0x04) /*!< [23:16] sub1 version */
#define __STM32L4_CMSIS_VERSION_SUB2   (0x00) /*!< [15:8]  sub2 version */
#define __STM32L4_CMSIS_VERSION_RC     (0x00) /*!< [7:0]  release candidate */
#define __STM32L4_CMSIS_VERSION        ((__STM32L4_CMSIS_VERSION_MAIN << 24)\
                                       |(__STM32L4_CMSIS_VERSION_SUB1 << 16)\
                                       |(__STM32L4_CMSIS_VERSION_SUB2 << 8 )\
                                       |(__STM32L4_CMSIS_VERSION_RC))

 typedef enum
 {
   RESET = 0,
   SET = !RESET
 } FlagStatus, ITStatus;

 typedef enum
 {
   DISABLE = 0,
   ENABLE = !DISABLE
 } FunctionalState;
 #define IS_FUNCTIONAL_STATE(STATE) (((STATE) == DISABLE) || ((STATE) == ENABLE))

 typedef enum
 {
   ERROR = 0,
   SUCCESS = !ERROR
 } ErrorStatus;

 /**
   * @}
   */


 /** @addtogroup Exported_macros
   * @{
   */
 #define SET_BIT(REG, BIT)     ((REG) |= (BIT))

 #define CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))

 #define READ_BIT(REG, BIT)    ((REG) & (BIT))

 #define CLEAR_REG(REG)        ((REG) = (0x0))

 #define WRITE_REG(REG, VAL)   ((REG) = (VAL))

 #define READ_REG(REG)         ((REG))

 #define MODIFY_REG(REG, CLEARMASK, SETMASK)  WRITE_REG((REG), (((READ_REG(REG)) & (~(CLEARMASK))) | (SETMASK)))

 #define POSITION_VAL(VAL)     (__CLZ(__RBIT(VAL)))


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* STM32L4 */




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
