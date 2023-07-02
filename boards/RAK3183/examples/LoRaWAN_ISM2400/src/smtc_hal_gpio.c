/*!
 * \file      smtc_hal_gpio.c
 *
 * \brief     GPIO Hardware Abstraction Layer implementation
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


#include "hal/am_hal_gpio.h"
#include "smtc_hal_gpio.h"
#include "am_mcu_apollo.h"
#include "am_util.h"


/* 为了传参 定义一个数组 50个GPIO */
static hal_gpio_irq_t const* gpio_irq[50];
//
// MCU input pin Handling
//

//const am_hal_gpio_pincfg_t g_AM_HAL_GPIO_INPUT =
//{
//    .uFuncSel       = 3,
//    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
//    .eGPInput       = AM_HAL_GPIO_PIN_INPUT_ENABLE,
//    .eGPRdZero      = AM_HAL_GPIO_PIN_RDZERO_READPIN
//};


void hal_gpio_init_in( const hal_gpio_pin_names_t pin, const hal_gpio_pull_mode_t pull_mode,
                       const hal_gpio_irq_mode_t irq_mode, hal_gpio_irq_t* irq )
												 
{
	
			if(0 )
			{
//				const am_hal_gpio_pincfg_t g_AM_BSP_DIO1 =
//				{
//				.uFuncSel            = 3,   //
//				.eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
//				.eGPInput            = AM_HAL_GPIO_PIN_INPUT_ENABLE,
//				.eIntDir	           = AM_HAL_GPIO_PIN_INTDIR_LO2HI
//				};
//		
//				//am_hal_gpio_interrupt_register(DIO1,sx1262_dio1);
//			 am_hal_gpio_pinconfig(15, g_AM_BSP_DIO1);
//	
//       AM_HAL_GPIO_MASKCREATE(GpioIntMask0);
//		   am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask0, 15));
//		   am_hal_gpio_interrupt_enable(AM_HAL_GPIO_MASKBIT(pGpioIntMask0, 15));
//			
//			 NVIC_SetPriority(GPIO_IRQn, 1);	
//			 NVIC_EnableIRQ(GPIO_IRQn);
//				
//			 if(irq != NULL)
//			 hal_gpio_irq_attach(irq);
 
    
			}
			else
			{
	    /*  为了简便 这里不分层 没有实现gpio_init的功能 */

			/*  g_AM_HAL_GPIO_INPUT_PULLUP 是一个弱上拉 apollo3的上拉电阻是可以选择的*/
			am_hal_gpio_pincfg_t AM_GPIO =
		  {
					.uFuncSel       = 3,
          .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_DISABLE,
					.eGPInput       = AM_HAL_GPIO_PIN_INPUT_ENABLE,
		  };
			
			if(pull_mode == BSP_GPIO_PULL_MODE_UP)
			{
				AM_GPIO.ePullup = AM_HAL_GPIO_PIN_PULLUP_6K;
			} else if (pull_mode == BSP_GPIO_PULL_MODE_DOWN) {
				//AM_GPIO.ePullup = AM_HAL_GPIO_PIN_PULLDOWN;    //居然是下拉的问题  下拉之后 中断没有上升沿了
			} else 
			{
				AM_GPIO.ePullup = AM_HAL_GPIO_PIN_PULLUP_NONE;
			}
			
			switch (irq_mode) 
			{
				case BSP_GPIO_IRQ_MODE_OFF : AM_GPIO.eIntDir = 0;
				AM_GPIO .eGPRdZero      = AM_HAL_GPIO_PIN_RDZERO_READPIN;
					break;
				case BSP_GPIO_IRQ_MODE_RISING : AM_GPIO.eIntDir = AM_HAL_GPIO_PIN_INTDIR_LO2HI;
					break;
				case BSP_GPIO_IRQ_MODE_FALLING: AM_GPIO.eIntDir =  AM_HAL_GPIO_PIN_INTDIR_HI2LO ;
					break;
				case BSP_GPIO_IRQ_MODE_RISING_FALLING: AM_GPIO.eIntDir = AM_HAL_GPIO_PIN_INTDIR_BOTH;
					break;
		   	  AM_GPIO.eIntDir = 0;
					AM_GPIO .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN;
				default:	  AM_GPIO.eIntDir = 0;
					AM_GPIO .eGPRdZero = AM_HAL_GPIO_PIN_RDZERO_READPIN;;break;
			}
			
			//因为这不是一个静态函数 所以要独立出去
			if(irq != NULL)
			hal_gpio_irq_attach(irq);
	
	    am_hal_gpio_pinconfig(pin,AM_GPIO);
			
			if(irq_mode != BSP_GPIO_IRQ_MODE_OFF)
			{
			AM_HAL_GPIO_MASKCREATE(GpioIntMask0);
			am_hal_gpio_interrupt_clear(AM_HAL_GPIO_MASKBIT(pGpioIntMask0, pin));
			am_hal_gpio_interrupt_enable(AM_HAL_GPIO_MASKBIT(pGpioIntMask0, pin));
			
			NVIC_SetPriority(GPIO_IRQn, 1);  //这里的中断优先级 建议使用宏定义	
			NVIC_EnableIRQ(GPIO_IRQn);
			}
      }
}

void hal_gpio_init_out( const hal_gpio_pin_names_t pin, const uint32_t value )
{
	    am_hal_gpio_pinconfig(pin,g_AM_HAL_GPIO_OUTPUT);
			am_hal_gpio_state_write(pin,(am_hal_gpio_write_type_e)value);
}

void hal_gpio_irq_attach( const hal_gpio_irq_t* irq )
{
    if( ( irq != NULL ) && ( irq->callback != NULL ) )
    {
					//am_util_stdio_printf("irq->context!= NULL  PIN %d\n",irq->pin);
				  gpio_irq[( irq->pin )] = irq;
        	//am_hal_gpio_interrupt_register(irq->pin,(am_hal_gpio_handler_t)irq->callback);
    }
}

void hal_gpio_irq_deatach( const hal_gpio_irq_t* irq )
{
    if( irq != NULL )
    {
           am_hal_gpio_interrupt_register(irq->pin,NULL);
    }
}

void hal_gpio_irq_enable( void )
{
   	NVIC_SetPriority(GPIO_IRQn, 1);	
		NVIC_EnableIRQ(GPIO_IRQn);

}

void hal_gpio_irq_disable( void )
{
   	__NVIC_DisableIRQ(GPIO_IRQn);
}

//
// MCU pin state control
//

void hal_gpio_set_value( const hal_gpio_pin_names_t pin, const uint32_t value )
{
    am_hal_gpio_state_write(pin,(am_hal_gpio_write_type_e)value);
}



uint32_t hal_gpio_get_value( const hal_gpio_pin_names_t pin )
{
    uint32_t IOReadState ;
	  am_hal_gpio_state_read(pin,AM_HAL_GPIO_INPUT_READ,&IOReadState);
	  return IOReadState;
}



void hal_gpio_clear_pending_irq( const hal_gpio_pin_names_t pin )
{
    uint64_t ui64Status;
		am_hal_gpio_interrupt_status_get(false, &ui64Status);
    am_hal_gpio_interrupt_clear(ui64Status);
}



//15 是DIOX
void am_gpio_isr(void)
{
    uint64_t ui64Status;
		am_hal_gpio_interrupt_status_get(false, &ui64Status);
    am_hal_gpio_interrupt_clear(ui64Status);
   	am_hal_gpio_interrupt_service(ui64Status);
	
	if (ui64Status & (1 << 15)) {
	  
	  if( ( gpio_irq[15] != NULL ) && ( gpio_irq[15]->callback != NULL ) )
    {
        gpio_irq[15]->callback( gpio_irq[15]->context );
    }
		hal_gpio_init_out(45 ,0);
	}
    //am_hal_gpio_interrupt_service(ui64Status);
}

//void am_gpio_isr(void)
//{
//    uint64_t ui64Status;

//    // 获取所有GPIO引脚的中断状态
//    am_hal_gpio_interrupt_status_get(false, &ui64Status);

//    // 清除所有GPIO引脚的中断
//    am_hal_gpio_interrupt_clear(ui64Status);

//    // 处理每个GPIO引脚的中断
//    for (int i = 0; i < AM_HAL_GPIO_MAX_PADS; i++)
//    {
//        if (ui64Status & (1ULL << i))
//        {
//            // 处理GPIO引脚i的中断
//            am_hal_gpio_interrupt_service(1ULL << i);

//            // 调用回调函数处理GPIO引脚i的中断
//            if (gpio_irq[i] != NULL && gpio_irq[i]->callback != NULL)
//            {
//                gpio_irq[i]->callback(gpio_irq[i]->context);
//            }
//        }
//    }
//}


/* --- EOF ------------------------------------------------------------------ */
