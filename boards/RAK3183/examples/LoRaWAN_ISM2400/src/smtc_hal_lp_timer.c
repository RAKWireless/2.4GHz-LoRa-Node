/*!
 * \file      smtc_hal_lp_timer.c
 *
 * \brief     Implements Low Power Timer utilities functions.
 *
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <stdint.h>   // C99 types
#include <stdbool.h>  // bool type

#include "smtc_hal_lp_timer.h"
//#include "smtc_hal_mcu.h"
#include "smtc_hal_gpio.h"
#include "am_mcu_apollo.h"
#include "am_util.h"

#define LED1  44
#define LED2	45
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE VARIABLES -------------------------------------------------------
 */


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DECLARATION -------------------------------------------
 */

/*
 * -----------------------------------------------------------------------------
 * --- PUBLIC FUNCTIONS DEFINITION ---------------------------------------------
 */
 hal_lp_timer_irq_t my_tmr_irq ={ .context = NULL, .callback = NULL };
 
void test_timer0()
{
	 if(my_tmr_irq.callback != NULL)
	 {
			my_tmr_irq.callback(my_tmr_irq.context);
	 }
	 //am_util_stdio_printf("hal_lp_timer_start \r\n");
}	 


void hal_lp_timer_init( void )
{
	am_hal_ctimer_config_single(0, AM_HAL_CTIMER_BOTH,
                                 AM_HAL_CTIMER_HFRC_3MHZ |
                                 AM_HAL_CTIMER_FN_REPEAT |
                                 AM_HAL_CTIMER_INT_ENABLE);
 
  am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA0C0);
 
  am_hal_interrupt_master_enable();  //需要查一下这个中断的含义

 
}

void hal_lp_timer_start( const uint32_t milliseconds, const hal_lp_timer_irq_t* tmr_irq )
{
	  if(tmr_irq != NULL)
			my_tmr_irq = *tmr_irq;
	 
	  /* 定时时间  mill/1000 = 1/3000000 * Period  */
	  uint32_t period =  ((float )milliseconds/1000) * 3000000;

	  am_hal_ctimer_period_set(0, AM_HAL_CTIMER_BOTH,period - 1 , 0);

    am_hal_ctimer_int_register(AM_HAL_CTIMER_INT_TIMERA0C0,test_timer0);
	
		NVIC_EnableIRQ(CTIMER_IRQn); 
	
	  am_hal_ctimer_start(0, AM_HAL_CTIMER_BOTH);
}

void hal_lp_timer_stop( void )
{
    am_hal_ctimer_stop(0, AM_HAL_CTIMER_BOTH);
}

void hal_lp_timer_irq_enable( void )
{
	   //am_hal_interrupt_priority_set(AM_HAL_INTERRUPT_CTIMER, configKERNEL_INTERRUPT_PRIORITY);
     NVIC_EnableIRQ(CTIMER_IRQn);  
}

void hal_lp_timer_irq_disable( void )
{
    //NVIC_DisableIRQ(CTIMER_IRQn);  
}

void am_ctimer_isr(void)  //问题可能就出现这这里了   1只清1  0只清0 的中断  
{
	
  // Clear TimerA0 Interrupt.
  uint32_t status = am_hal_ctimer_int_status_get(true);
	if(status & AM_HAL_CTIMER_INT_TIMERA1)
	{
		//hal_gpio_init_out(LED1 ,1);
		am_hal_ctimer_int_clear(AM_HAL_CTIMER_INT_TIMERA1);
		am_hal_ctimer_int_service(status & AM_HAL_CTIMER_INT_TIMERA1);
	}
	
	if(status & AM_HAL_CTIMER_INT_TIMERA0)
	{
		 //hal_gpio_init_out(LED2 ,1);
		 am_hal_ctimer_int_clear(AM_HAL_CTIMER_INT_TIMERA0);
		 am_hal_ctimer_int_service(status & AM_HAL_CTIMER_INT_TIMERA0);
	}
 
}

void test_callback(void *contex)
{
	 am_util_stdio_printf("123\r\n");
}



void am_ctimer_test(void)
{
	hal_lp_timer_irq_t tmr_irq = {NULL,test_callback};
	
	hal_lp_timer_init();
	
	hal_lp_timer_irq_enable();
		
	hal_lp_timer_start(50,&tmr_irq);
	
}
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE FUNCTIONS DEFINITION --------------------------------------------
 */

/* --- EOF ------------------------------------------------------------------ */
