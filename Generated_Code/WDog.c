/* ###################################################################
**     THIS COMPONENT MODULE IS GENERATED BY THE TOOL. DO NOT MODIFY IT.
**     Filename    : WDog.c
**     Project     : IOM_A
**     Processor   : MC9S08PA4VTJ
**     Component   : WatchDog
**     Version     : Component 02.142, Driver 01.21, CPU db: 3.00.000
**     Compiler    : CodeWarrior HCS08 C Compiler
**     Date/Time   : 2015-05-02, 22:09, # CodeGen: 5
**     Abstract    :
**         This device "WatchDog" implements a watchdog.
**         When enabled, the watchdog timer counts until it reaches
**         a critical value corresponding to the period specified
**         in 'Properties'. Then, the watchdog causes a CPU reset.
**         Applications may clear the timer before it reaches the critical
**         value. The timer then restarts the watchdog counter.
**         Watchdog is used for increasing the safety of the system
**         (unforeseeable situations can cause system crash or an
**         infinite loop - watchdog can restart the system or handle
**         the situation).
**
**         Note: Watchdog can be enabled or disabled in the initialization
**               code only. If the watchdog is once enabled user have
**               to call Clear method in defined time intervals.
**     Settings    :
**         Watchdog causes             : Reset CPU
**
**         Clock Source                : Internal clock 1-kHz
**
**         Initial state               : Disabled
**
**         Mode of operation           : Normal
**
**         High speed mode
**           Watchdog period/frequency
**             microseconds            : 205432
**             milliseconds            : 205
**             Hz                      : 5
**
**         Run register                : WDOG_CS1  [$3030]
**         Mode register               : WDOG_CNT  [$3032]
**         Prescaler register          : WDOG_TOVAL [$3034]
**     Contents    :
**         Enable  - byte WDog_Enable(void);
**         Disable - byte WDog_Disable(void);
**         Clear   - byte WDog_Clear(void);
**
**     Copyright : 1997 - 2014 Freescale Semiconductor, Inc. 
**     All Rights Reserved.
**     
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**     
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**     
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**     
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**     
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**     
**     http: www.freescale.com
**     mail: support@freescale.com
** ###################################################################*/
/*!
** @file WDog.c
** @version 01.21
** @brief
**         This device "WatchDog" implements a watchdog.
**         When enabled, the watchdog timer counts until it reaches
**         a critical value corresponding to the period specified
**         in 'Properties'. Then, the watchdog causes a CPU reset.
**         Applications may clear the timer before it reaches the critical
**         value. The timer then restarts the watchdog counter.
**         Watchdog is used for increasing the safety of the system
**         (unforeseeable situations can cause system crash or an
**         infinite loop - watchdog can restart the system or handle
**         the situation).
**
**         Note: Watchdog can be enabled or disabled in the initialization
**               code only. If the watchdog is once enabled user have
**               to call Clear method in defined time intervals.
*/         
/*!
**  @addtogroup WDog_module WDog module documentation
**  @{
*/         


/* MODULE WDog. */

#include "WDog.h"
static bool EnUser;                    /* Enable/Disable device by user */

/* Internal macros and method prototypes */

static void Unlock(void)
{
  /* WDOG_CNT: CNT=0xC520 */
  setReg16(WDOG_CNT, 0xC520U);         /* First part of unlock sequence */ 
  /* WDOG_CNT: CNT=0xD928 */
  setReg16(WDOG_CNT, 0xD928U);         /* Second part of unlock sequence */ 
}

/* End of internal method prototypes */

/*
** ===================================================================
**     Method      :  WDog_Enable (component WatchDog)
*/
/*!
**     @brief
**         Enables the component - starts the watchdog timer counting. 
**     @return
**                         - Error code, possible codes:
**                           ERR_OK - OK
**                           ERR_SPEED - This device does not work in
**                           the active speed mode
**                           ERR_PROTECT - This device does is protected
*/
/* ===================================================================*/
byte WDog_Enable(void)
{
  EnUser = TRUE;                       /* Set the flag "device enabled" */
  Unlock();                            /* Unlock sequence */
  /* WDOG_CS1: EN=1 */
  setReg8Bits(WDOG_CS1, 0x80U);        /* Enable WDOG */ 
  return ERR_OK;                       /* OK */
}

/*
** ===================================================================
**     Method      :  WDog_Disable (component WatchDog)
*/
/*!
**     @brief
**         Disables the component - stops the watchdog timer counting.
**     @return
**                         - Error code, possible codes:
**                           ERR_OK - OK
**                           ERR_SPEED - This device does not work in
**                           the active speed mode
**                           ERR_PROTECT - This device does is protected
*/
/* ===================================================================*/
byte WDog_Disable(void)
{
  EnUser = FALSE;                      /* Set the flag "device disabled" */
  Unlock();                            /* Unlock sequence */
  /* WDOG_CS1: EN=0 */
  clrReg8Bits(WDOG_CS1, 0x80U);        /* Disable WDOG */ 
  return ERR_OK;                       /* OK */
}

/*
** ===================================================================
**     Method      :  WDog_Clear (component WatchDog)
**     Description :
**         Clears the watchdog timer (it makes the timer restart from
**         zero).
**     Parameters  : None
**     Returns     :
**         ---             - Error code, possible codes:
**                           ERR_OK - OK
**                           ERR_SPEED - This device does not work in
**                           the active speed mode
**                           ERR_DISABLED - The component is disabled
** ===================================================================
*/
byte WDog_Clear(void)
{
  if (!EnUser) {                       /* Is WDOG disabled? */
    return ERR_DISABLED;               /* If yes then error */
  }
  EnterCritical();
  /* WDOG_CNT: CNT=0xA602 */
  setReg16(WDOG_CNT, 0xA602U);         /* First part of refresh sequence */ 
  /* WDOG_CNT: CNT=0xB480 */
  setReg16(WDOG_CNT, 0xB480U);         /* Second part of refresh sequence */ 
  ExitCritical();
  return ERR_OK;                       /* OK */
}

/*
** ===================================================================
**     Method      :  WDog_Init (component WatchDog)
**
**     Description :
**         Initializes the associated peripheral(s) and the component 
**         internal variables. The method is called automatically as a 
**         part of the application initialization code.
**         This method is internal. It is used by Processor Expert only.
** ===================================================================
*/
void WDog_Init(void)
{
  EnUser = FALSE;                      /* Set the flag "device disabled" by user */
}

/* END WDog. */

/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.3 [05.09]
**     for the Freescale HCS08 series of microcontrollers.
**
** ###################################################################
*/
