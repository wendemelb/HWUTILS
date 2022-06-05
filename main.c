/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <string.h>
#include "dvp.h"
#include "fpioa.h"
#include "lcd.h"
#include "ov5640.h"
#include "ov2640.h"
#include "plic.h"
#include "math.h"
#include "bsp.h"

#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "board_config.h"
#include "unistd.h"
#include "iomem.h"
#include "image.h"
static uint32_t *g_lcd_gram0;
static uint32_t *g_lcd_gram1;
static uint32_t *g_lcd_gram2;
static uint32_t *g_lcd_gram3;

volatile uint8_t g_dvp_finish_flag;
volatile uint8_t g_ram_mux;

static int on_irq_dvp(void* ctx)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);

        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    }
    else
    {
        if(g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return 0;
}

static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(42, FUNC_CMOS_RST);
    fpioa_set_function(44, FUNC_CMOS_PWDN);
    fpioa_set_function(46, FUNC_CMOS_XCLK);
    fpioa_set_function(43, FUNC_CMOS_VSYNC);
    fpioa_set_function(45, FUNC_CMOS_HREF);
    fpioa_set_function(47, FUNC_CMOS_PCLK);
    fpioa_set_function(41, FUNC_SCCB_SCLK);
    fpioa_set_function(40, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(38, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(36, FUNC_SPI0_SS3);
    fpioa_set_function(39, FUNC_SPI0_SCLK);
    fpioa_set_function(37, FUNC_GPIOHS0 + RST_GPIONUM);

    sysctl_set_spi0_dvp_data(1);

}

static void io_set_power(void)
{
    sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
    sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);

}

#define MASKR(x) ((x&(0xf800))>>8)
#define MASKB(x) ((x&(0x001f))>>0)
#define MASKG(x) ((x&(0x07e0))>>4)
#define THRER (0xf8/8)
#define THREG (0x7e/8)
#define THREB (0x1f/8)

volatile int start=0;






int main(void)
{
	int64_t cy;
    /* Set CPU and dvp clk */
    sysctl_pll_set_freq(SYSCTL_PLL0, 800000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL1, 160000000UL);
    sysctl_pll_set_freq(SYSCTL_PLL2, 45158400UL);
    uarths_init();

    io_mux_init();
    io_set_power();
    plic_init();

    /* LCD init */
    printf("LCD init\n");
    lcd_init();
    lcd_set_direction(DIR_YX_RLUD);
    lcd_clear(BLACK);

    g_lcd_gram0 = (uint32_t *)iomem_malloc(320*240*2);
    g_lcd_gram1 = (uint32_t *)iomem_malloc(320*240*2);
	g_lcd_gram2 = (uint32_t *)iomem_malloc(320*240*2);
	g_lcd_gram3 = (uint32_t *)iomem_malloc(320*240*2);
    /* DVP init */
    printf("DVP init\n");

    dvp_init(8);
    dvp_set_xclk_rate(24000000);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);

    dvp_set_image_size(320, 240);
    ov2640_init();

    dvp_set_ai_addr((uint32_t)0x40600000, (uint32_t)0x40612C00, (uint32_t)0x40625800);
    dvp_set_display_addr((uint32_t)g_lcd_gram0);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* DVP interrupt config */
    printf("DVP interrupt config\n");
    plic_set_priority(IRQN_DVP_INTERRUPT, 1);
    plic_irq_register(IRQN_DVP_INTERRUPT, on_irq_dvp, NULL);
    plic_irq_enable(IRQN_DVP_INTERRUPT);

    /* enable global interrupt */
    sysctl_enable_irq();

    /* system start */
    printf("system start\n");
    g_ram_mux = 0;
    g_dvp_finish_flag = 0;
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
	start=0;
	//register_core1(subt2, NULL);
    while (1)
    {
        /* ai cal finish*/
        while (g_dvp_finish_flag == 0)
            ;
        
        /* display pic*/
        g_ram_mux ^= 0x01;
		if(!g_ram_mux){//1 finish
			/*
			cy=read_cycle ();
			grayScale(g_lcd_gram1, g_lcd_gram2, 320*240);
			grayScale(g_lcd_gram0, g_lcd_gram1, 320*240);
			printf("cucle imgprocess16gray: %ld\r\n",read_cycle ()-cy);
			cy=read_cycle ();
			subt(g_lcd_gram1,g_lcd_gram2, g_lcd_gram0, 1);
			printf("cucle imgprocess16: %ld\r\n",read_cycle ()-cy);
			*/
			//cy=read_cycle ();
			//grayScale64(g_lcd_gram0, g_lcd_gram2, 320*240*2);
			g_dvp_finish_flag = 0;
			grayScale64(g_lcd_gram1, g_lcd_gram3, 320*240*2);
			
			//printf("cucle imgprocess64gray: %ld\r\n",read_cycle ()-cy);
			//cy=read_cycle ();
			subt64(g_lcd_gram2,g_lcd_gram3, g_lcd_gram1, 320*240*2);
			biv(g_lcd_gram1, 320*240);
			//printf("cucle imgprocess64: %ld\r\n",read_cycle ()-cy);
			//cy=read_cycle ();
        	lcd_draw_picture(0, 0, 320, 240,g_lcd_gram1);
			//printf("cucle draw: %ld\r\n",read_cycle ()-cy);
		}
		else
		{
			//grayScale64(g_lcd_gram1, g_lcd_gram2, 320*240*2);
			g_dvp_finish_flag = 0;
			grayScale64(g_lcd_gram0, g_lcd_gram2, 320*240*2);
			
			//printf("cucle imgprocess64gray: %ld\r\n",read_cycle ()-cy);
			//cy=read_cycle ();
			subt64(g_lcd_gram3,g_lcd_gram2, g_lcd_gram0, 320*240*2);
			biv(g_lcd_gram0, 320*240);
			//printf("cucle imgprocess64: %ld\r\n",read_cycle ()-cy);
			//cy=read_cycle ();
        	lcd_draw_picture(0, 0, 320, 240,g_lcd_gram0);
			//printf("cucle draw: %ld\r\n",read_cycle ()-cy);
		

		}
    }
    iomem_free(g_lcd_gram0);
    iomem_free(g_lcd_gram1);
    return 0;
}
