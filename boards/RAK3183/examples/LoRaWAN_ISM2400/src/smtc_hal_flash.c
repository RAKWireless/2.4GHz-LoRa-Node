/*!
 * \file      smtc_hal_flash.c
 *
 * \brief     FLASH Hardware Abstraction Layer implementation
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
#include <stdio.h>    // TODO: check if needed

#include "smtc_hal_dbg_trace.h"

#include <string.h>
/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE MACROS-----------------------------------------------------------
 */

#define ARB_PAGE_ADDRESS (AM_HAL_FLASH_INSTANCE_SIZE + (9 * AM_HAL_FLASH_PAGE_SIZE))


/*
 * -----------------------------------------------------------------------------
 * --- PRIVATE CONSTANTS -------------------------------------------------------
 */

/*!
 * Generic definition
 */
#ifndef SUCCESS
#define SUCCESS 1
#endif

#ifndef FAIL
#define FAIL 0
#endif

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

uint8_t hal_flash_erase_page( uint32_t addr, uint8_t nb_page )
{
	  int32_t i32ReturnCode;
		
    uint32_t ui32FlashInst = AM_HAL_FLASH_ADDR2INST(addr);
    i32ReturnCode = am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY, ui32FlashInst,AM_HAL_FLASH_ADDR2PAGE(addr));
	
	  if(i32ReturnCode)
		{
			am_util_stdio_printf("hal_flash_erase_page error!\n");
			//while(1);
		}
		return i32ReturnCode;
}

uint32_t hal_flash_write_buffer( uint32_t addr, const uint8_t* buffer, uint32_t size )
{
	  uint32_t* pui32Dst = (uint32_t *)addr;
	  
	  uint32_t BufferIndex = 0, real_size = 0, AddrEnd = 0;
	
	  uint32_t data32                = 0;
	
		int32_t i32ReturnCode;
	
		if( ( size % 4 ) != 0 )   
    {
        real_size = size + ( 4 - ( size % 4 ) );
    }
    else
    {
        real_size = size;
    }
		
		AddrEnd = addr + real_size;
		
		
		while( addr < AddrEnd )
    {
        data32 = 0;
			
        for( uint8_t i = 0; i < 4; i++ )
        {
            data32 += ( ( ( uint32_t ) buffer[BufferIndex + i] ) << ( i * 8 ) );
        }
				
				pui32Dst = (uint32_t *)addr;
				
				//am_util_stdio_printf("am_hal_flash_program_main data32 %08x pui32Dst %08x \r\n",data32,pui32Dst);

				i32ReturnCode = am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,&data32,pui32Dst,1);
				
				if(i32ReturnCode)
				{
						am_util_stdio_printf("hal_flash_write_buffer error!\n");
						while(1);
				}
        /* increment to next word*/
				addr        = addr + 4;
        BufferIndex = BufferIndex + 4;
        
    }
		
	  return 0;
}

void hal_flash_read_buffer( uint32_t addr, uint8_t* buffer, uint32_t size )
{
    uint32_t     FlashIndex = 0;
    __IO uint8_t data8      = 0;

    while( FlashIndex < size )
    {
        data8 = *( __IO uint32_t* ) ( addr + FlashIndex );

        buffer[FlashIndex] = data8;

        FlashIndex++;
    }
}


void test()
{
	
	am_util_stdio_printf("ARB_PAGE_ADDRESS %08X ",ARB_PAGE_ADDRESS);
	
	hal_flash_erase_page(ARB_PAGE_ADDRESS,1);
	int i ;
	uint8_t test[17],read[17];
	for(i=0; i < 17 ;i++)
	{
		test[i]=i+100;
		read[i]=0;
	}
	hal_flash_write_buffer(ARB_PAGE_ADDRESS,test,17);
	hal_flash_read_buffer(ARB_PAGE_ADDRESS,read,17);
	
	for( i = 0; i < 17 ;i++)
	{
		am_util_stdio_printf("%d ",read[i]);
	}
	
	am_util_stdio_printf("\r\n");
		
	for( i = 0; i < 17 ;i++)
	{
		am_util_stdio_printf("%d ",test[i]);
	}
}


/* --- EOF ------------------------------------------------------------------ */
