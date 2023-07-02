/*!
 * \file      smtc_hal_rtc.c
 *
 * \brief     RTC Hardware Abstraction Layer implementation
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

#include <time.h>
#include "smtc_hal_rtc.h"

//#include "stm32l4xx_hal.h"
//#include "stm32l4xx_ll_rtc.h"
//#include "smtc_hal_mcu.h"
#include "am_mcu_apollo.h"
#include "am_util.h"


// clang-format on

/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE TYPES -----------------------------------------------------------
 */

static uint32_t seconds = 0;   //溢出的话 只有这个32位满了才会溢出   理论上几百年
static uint32_t milliseconds_div_10 = 0;    //单位100us 
static volatile bool wut_timer_irq_happened = false;

void timer1_callback(void)
{
	 hal_gpio_init_out(44 ,0);
	 milliseconds_div_10++;
	 if(milliseconds_div_10 == 10000)
	 {
		 //am_util_stdio_printf("second %d",seconds);
		 seconds++;
		 milliseconds_div_10 = 0;
	 }
}



/*注意相关的定时器 都要改为定时器1*/
void hal_rtc_init( void )
{
  am_hal_ctimer_config_single(1, AM_HAL_CTIMER_BOTH,
                                 AM_HAL_CTIMER_HFRC_3MHZ |
                                 AM_HAL_CTIMER_FN_REPEAT |
                                 AM_HAL_CTIMER_INT_ENABLE);
 
  am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA0C0);
	
	NVIC_SetPriority(UART0_IRQn,6);
  NVIC_EnableIRQ(CTIMER_IRQn); 
	
	float milliseconds = 0.1 ;  //定义一个100us 的定时器
  uint32_t period =  ((float )milliseconds/1000) * 3000000;
	//am_util_stdio_printf("period %d\r\n",period);
	am_hal_ctimer_period_set(1, AM_HAL_CTIMER_BOTH,period - 1 , 0);
  am_hal_ctimer_int_register(AM_HAL_CTIMER_INT_TIMERA1,(am_hal_ctimer_handler_t)timer1_callback);
	
	am_hal_ctimer_int_enable(AM_HAL_CTIMER_INT_TIMERA1);   // 
	am_hal_ctimer_start(1, AM_HAL_CTIMER_BOTH); 
}

uint32_t hal_rtc_get_time_s( void )
{
    return seconds;
}

uint32_t hal_rtc_get_time_100us( void )
{
    return (seconds *10000 + milliseconds_div_10);
}


uint32_t hal_rtc_get_time_ms( void )
{
    return seconds * 1000 + ( milliseconds_div_10 / 10 );
}


//这个定时器是用来设置低功耗唤醒用的  这里可以不实现   下面3个API都和低功耗相关
//void hal_rtc_wakeup_timer_set_ms( const int32_t milliseconds )
//{
//    uint32_t delay_ms_2_tick = rtc_ms_2_wakeup_timer_tick( milliseconds );

//    HAL_RTCEx_DeactivateWakeUpTimer( &bsp_rtc.handle );
//    // reset irq status
//    wut_timer_irq_happened = false;
//    HAL_RTCEx_SetWakeUpTimer_IT( &bsp_rtc.handle, delay_ms_2_tick, RTC_WAKEUPCLOCK_RTCCLK_DIV16 );
//}

//void hal_rtc_wakeup_timer_stop( void )
//{
//    am_hal_ctimer_stop(1, AM_HAL_CTIMER_BOTH); 
//}


//
//bool hal_rtc_has_wut_irq_happened( void )
//{
//    return wut_timer_irq_happened;
//}



/* --- EOF ------------------------------------------------------------------ */
