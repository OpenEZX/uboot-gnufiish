/*
 * (C) 2009 by Harald Welte <laforge@gnumonks.org>
 *
 * based on existing S3C2410 startup code in u-boot:
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <s3c2440.h>
#include <i2c.h>
#include <bootmenu.h>
#include <asm/atomic.h>

#include "jbt6k74.h"

DECLARE_GLOBAL_DATA_PTR;

#define M_MDIV 42
#define M_PDIV 1
#define M_SDIV 0
#define U_M_MDIV 88
#define U_M_PDIV 4
#define U_M_SDIV 2

unsigned int neo1973_wakeup_cause;
extern unsigned char booted_from_nand;
extern int nobootdelay;
extern int udc_usb_maxcurrent;

char __cfg_prompt[20] = "X800 # ";

static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
	  "subs %0, %1, #1\n"
	  "bne 1b":"=r" (loops):"0" (loops));
}

enum glofiish_led {
	GFISH_LED_PWR_ORANGE	= 0,
	GFISH_LED_PWR_BLUE	= 1,
	GFISH_LED_AUX_RED	= 2,
};

/*
 * Miscellaneous platform dependent initialisations
 */

static void cpu_speed(int mdiv, int pdiv, int sdiv, int clkdivn)
{
	S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();

	/* clock divide */
	clk_power->CLKDIVN = clkdivn;

	/* to reduce PLL lock time, adjust the LOCKTIME register */
	clk_power->LOCKTIME = 0xFFFFFF;

	/* configure MPLL */
	clk_power->MPLLCON = ((mdiv << 12) + (pdiv << 4) + sdiv);

	/* some delay between MPLL and UPLL */
	delay (4000);

	/* configure UPLL */
	clk_power->UPLLCON = ((U_M_MDIV << 12) + (U_M_PDIV << 4) + U_M_SDIV);

	/* some delay between MPLL and UPLL */
	delay (8000);
}

int board_init(void)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();

	/* FCLK = 200MHz values from cpu/arm920t/start.S */
	cpu_speed(142, 7, 1, 3); /* 200MHZ, 1:2:4 */

	*((u_int16_t *)0x08000018) = 1;
	*((u_int16_t *)0x08000012) = 2;
	*((u_int16_t *)0x08000010) = 3;

	/* set up the I/O ports (from: booter on X800) */
	gpio->GPACON = 0x005E16B1;
	gpio->GPADAT = 0x00000B42;

	//gpio->GPBCON = 0x00015155;
	gpio->GPBCON = 0x0155555;
	gpio->GPBDAT = 0x00000410;	/* B10: microSD/MMC power */
	gpio->GPBUP = 0x0000FFFF;

	gpio->GPCCON = 0x55555555;
	gpio->GPCDAT = 0x00000000;
	gpio->GPCUP = 0x0000FFFF;

	gpio->GPDCON = 0x55415555;
	gpio->GPDDAT = 0x00000000;
	gpio->GPDUP = 0x0000FFFF;

	gpio->GPECON = 0x056AA915;
	gpio->GPEDAT = 0x00000000;
	gpio->GPEUP = 0x0000FFFF;

	gpio->GPFCON = 0x00000088;
	gpio->GPFDAT = 0x00000000;
	gpio->GPFUP = 0x0000FFFF;

	gpio->GPGCON = 0x55041400;
	gpio->GPGDAT = 0x00006200;
	gpio->GPGUP = 0x0000FFFF;

	gpio->GPHCON = 0x0014A1AA;
	gpio->GPHDAT = 0x00000000;
	gpio->GPHUP = 0x0000FFFF;

	gpio->GPJCON = 0x01400000;
	gpio->GPJDAT = 0x00000000;
	gpio->GPJUP = 0xFFFF;

	{
		u_int16_t f;
		u_int8_t b;

#if 0
		f = *((u_int16_t *)0x0800001c);
		f &= 0x0f;
		b = 0x0b020000 + f << 1;
		*((u_int16_t *)0x08000004) = *b++;
		*((u_int16_t *)0x08000000) = *b;
#endif
		*((u_int16_t *)0x0800001e) = 2;
	}

	/* arch number of X800-Board */
	gd->bd->bi_arch_number = MACH_TYPE_GLOFIISH_X800;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = 0x30000100;

	icache_enable();
	dcache_enable();

	return 0;
}

static void cpu_idle(void)
{
	S3C24X0_INTERRUPT * const intr = S3C24X0_GetBase_INTERRUPT();
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();
	S3C24X0_CLOCK_POWER * const clk = S3C24X0_GetBase_CLOCK_POWER();
	unsigned long flags;

	/*
	 * We don't want to execute interrupts throughout all this, since
	 * u-boot's interrupt handling code isn't modular, and getting a "real"
	 * interrupt without clearing it in the interrupt handler would cause
	 * us to loop permanently.
	 */
	local_irq_save(flags);

	/* go idle */
	clk->CLKCON |= 1 << 2;

	/* we're safe now */
	local_irq_restore(flags);
}


int board_late_init(void)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();
	uint8_t int1, int2;
	char buf[32];
	int menu_vote = 0; /* <= 0: no, > 0: yes */
	int seconds = 0;
	int enter_bootmenu;
	char *env_stop_in_menu;

	cpu_speed(M_MDIV, M_PDIV, M_SDIV, 5); /* 400MHZ, 1:4:8 */

	/* FIXME: issue a short pulse with the vibrator */

	jbt6k74_init();
	jbt6k74_enter_state(JBT_STATE_NORMAL);
	jbt6k74_display_onoff(1);
	/* switch on the backlight */
	glofiish_backlight(1);

	return 0;
}

int dram_init (void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

u_int32_t get_board_rev(void)
{
	return 0x1000;
}

void neo1973_poweroff(void)
{
	printf("poweroff\n");
	udc_disconnect();
	/* FIXME: actual poweroff */
	/* don't return to caller */
	while (1) ;
}

void glofiish_backlight(int on)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();

	if (on)
		gpio->GPBDAT |= 1;
	else
		gpio->GPBDAT &= ~1;
}

/* FIXME: shared */
void glofiish_vibrator(int on)
{
	if (on)
		*((u_int16_t *)0x08000014) |= (1 << 2);
	else
		*((u_int16_t *)0x08000014) &= ~(1 << 2);
}

void neo1973_gsm(int on)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();
	/* FIXME */
}

void neo1973_gps(int on)
{
	/* FIXME */
}

/* The sum of all part_size[]s must equal to or greater than the NAND size,
   i.e., 0x10000000. */

unsigned int dynpart_size[] = {
    CFG_UBOOT_SIZE, CFG_ENV_SIZE, 0x800000, 0xa0000, 0x40000, 0x10000000, 0 };
char *dynpart_names[] = {
    "u-boot", "u-boot_env", "kernel", "splash", "factory", "rootfs", NULL };

void glofiish_led(int led, int on)
{
	S3C24X0_GPIO * const gpio = S3C24X0_GetBase_GPIO();
	/* FIXME */
}
