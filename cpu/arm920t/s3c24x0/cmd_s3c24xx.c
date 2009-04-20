/*
 * (C) Copyright 2006-2008 by OpenMoko, Inc.
 * Authors: Harald Welte <laforge@openmoko.org>
 *	    Werner Almesberger <werner@openmoko.org>
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

/*
 * Boot support
 */
#include <common.h>
#include <command.h>
#include <net.h>		/* for print_IPaddr */
#if defined(CONFIG_S3C2410)
#include <s3c2410.h>
#elif defined(CONFIG_S3C2440) || defined(CONFIG_S3C2442)
#include <s3c2440.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CMD_BDI)

#define ARRAY_SIZE(x)           (sizeof(x) / sizeof((x)[0]))
#define MHZ	1000000

static void print_cpu_speed(void)
{
	printf("FCLK = %u MHz, HCLK = %u MHz, PCLK = %u MHz, UCLK = %u MHz\n",
		get_FCLK()/MHZ, get_HCLK()/MHZ, get_PCLK()/MHZ, get_UCLK()/MHZ);
}

struct s3c24x0_pll_speed {
	u_int16_t	mhz;
	u_int32_t	mpllcon;
	u_int32_t	clkdivn;
	u_int32_t	camdivn;
};

#define CLKDIVN_1_1_1	0x00
#define CLKDIVN_1_2_2	0x02
#define CLKDIVN_1_2_4	0x03
#define CLKDIVN_1_4_4	0x04

#if defined(CONFIG_S3C2440) || defined(CONFIG_S3C2442)
#define CLKDIVN_1_4_8	0x05
#define CLKDIVN_1_3_6	0x07
#endif

#if defined(CONFIG_S3C2410)
static const u_int32_t upllcon = ((0x78 << 12) + (0x2 << 4) + 0x3);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 50,
		.mpllcon = ((0x5c << 12) + (0x4 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_1_1,
	},
	{
		.mhz = 101,
		.mpllcon = ((0x7f << 12) + (0x2 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_2_2,
	},
	{
		.mhz = 202,
		.mpllcon = ((0x90 << 12) + (0x7 << 4) + 0x0),
		.clkdivn = CLKDIVN_1_2_4,
	},
	{
		.mhz = 266,
		.mpllcon = ((0x7d << 12) + (0x1 << 4) + 0x1),
		.clkdivn = CLKDIVN_1_2_4,
	},
};
#elif defined(CONFIG_S3C2440)
/* from page 7-21 of S3C2440A user's manual Revision 1 */
#if (CONFIG_SYS_CLK_FREQ == 12000000)
static const u_int32_t upllcon = ((0x38 << 12) + (2 << 4) + 2);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 200,
		.mpllcon = ((142 << 12) + (7 << 4) + 1),
		.clkdivn = CLKDIVN_1_2_4,
	},
	{
		.mhz = 271,
		.mpllcon = ((0xad << 12) + (0x2 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_2_4,
	},
	{
		.mhz = 304,
		.mpllcon = ((0x7d << 12) + (0x1 << 4) + 0x1),
		.clkdivn = CLKDIVN_1_3_6,
	},
	{
		.mhz = 405,
		.mpllcon = ((0x7f << 12) + (0x2 << 4) + 0x1),
		.clkdivn = CLKDIVN_1_3_6,
	},
#elif (CONFIG_SYS_CLK_FREQ == 16934400)
static const u_int32_t upllcon = ((0x3c << 12) + (2 << 4) + 2);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 200,
		.mpllcon = ((181 << 12) + (14 << 4) + 1),
		.clkdivn = CLKDIVN_1_2_4,
	},
	{
		.mhz = 266,
		.mpllcon = ((0x76 << 12) + (0x2 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_2_4,
		.camdivn = 0,
	},
	{
		.mhz = 296,
		.mpllcon = ((0x61 << 12) + (0x1 << 4) + 0x2),
		.clkdivn = CLKDIVN_1_3_6,
		.camdivn = 0,
	},
	{
		.mhz = 399,
		.mpllcon = ((0x6e << 12) + (0x3 << 4) + 0x1),
		.clkdivn = CLKDIVN_1_3_6,
		.camdivn = 0,
	},
#else
#error "clock frequencies != 12MHz / 16.9344MHz not supported"
#endif
};
#elif defined(CONFIG_S3C2442)
#if (CONFIG_SYS_CLK_FREQ == 12000000)
/* The value suggested in the user manual ((80 << 12) + (8 << 4) + 1) leads to
 * 52MHz, i.e. completely wrong */
static const u_int32_t upllcon = ((88 << 12) + (4 << 4) + 2);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 200,
		.mpllcon = ((42 << 12) + (1 << 4) + 1),
		.clkdivn = CLKDIVN_1_2_4,
		.camdivn = 0,
	},
	{
		.mhz = 300,
		.mpllcon = ((67 << 12) + (1 << 4) + 1),
		.clkdivn = CLKDIVN_1_3_6,
		.camdivn = 0,
	},
	{
		/* Make sure you are running at 1.4VDDiarm if you use this mode*/
		.mhz = 400,
		.mpllcon = ((42 << 12) + (1 << 4) + 0),
		.clkdivn = CLKDIVN_1_4_8,
		.camdivn = 0,
	},
	{
		/* This is MSP54 specific, as per openmoko calculations */
		/* Make sure you are running at 1.7VDDiarm if you use this mode*/
		.mhz = 500,
		.mpllcon = ((96 << 12) + (3 << 4) + 0),
		.clkdivn = CLKDIVN_1_4_8,
		.camdivn = 0,
	},
#elif (CONFIG_SYS_CLK_FREQ == 16934400)
static const u_int32_t upllcon = ((26 << 12) + (4 << 4) + 1);
static const struct s3c24x0_pll_speed pll_configs[] = {
	{
		.mhz = 296,
		.mpllcon = ((62 << 12) + (1 << 4) + 2),
		.clkdivn = CLKDIVN_1_3_6,
		.camdivn = 0,
	},
	{
		/* Make sure you are running at 1.4VDDiarm if you use this mode*/
		.mhz = 400,
		.mpllcon = ((63 << 12) + (4 << 4) + 0),
		.clkdivn = CLKDIVN_1_4_8,
		.camdivn = 0,
	},
	{
		/* This is MSP54 specific, as per data sheet. */
		/* Make sure you are running at 1.7VDDiarm if you use this mode*/
		.mhz = 500,
		.mpllcon = ((110 << 12) + (2 << 4) + 1),
		.clkdivn = CLKDIVN_1_4_8,
		.camdivn = 0,
	},
#else
#error "clock frequencies != 12MHz / 16.9344MHz not supported"
#endif
};
#else
#error "please define valid pll configurations for your cpu type"
#endif

static void list_cpu_speeds(void)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(pll_configs); i++)
		printf("%u MHz\n", pll_configs[i].mhz);
}

static int reconfig_mpll(u_int16_t mhz)
{
	S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();
	int i;

	for (i = 0; i < ARRAY_SIZE(pll_configs); i++) {
		if (pll_configs[i].mhz == mhz) {
#if defined(CONFIG_S3C2440)
			clk_power->CAMDIVN &= ~0x30;
			clk_power->CAMDIVN |= pll_configs[i].camdivn;
#endif
			/* to reduce PLL lock time, adjust the LOCKTIME register */
			clk_power->LOCKTIME = 0xFFFFFF;

			/* configure MPLL */
			clk_power->MPLLCON = pll_configs[i].mpllcon;
			clk_power->UPLLCON = upllcon;
			clk_power->CLKDIVN = pll_configs[i].clkdivn;

			/* If we changed the speed, we need to re-configure
			 * the serial baud rate generator */
			serial_setbrg();
			return 0;
		}
	}
	return -1;
}

/* GPIO handling, taken from Openmoko SVN trunk/src/target/gpio/gpio.c */

#define BASE 0x56000000

static volatile void *mem = (void *)BASE;

#define CON_IN	0
#define CON_OUT	1
#define CON_F1	2
#define CON_F2	3

static struct port {
	const char *name;
	int offset;
	int last;
	enum { pt_a, pt_b } type;
} ports[] = {
	{ "A", 0x00, 22, pt_a },
	{ "B", 0x10, 10, pt_b },
	{ "C", 0x20, 15, pt_b },
	{ "D", 0x30, 15, pt_b },
	{ "E", 0x40, 15, pt_b },
	{ "F", 0x50,  7, pt_b },
	{ "G", 0x60, 15, pt_b },
	{ "H", 0x70, 10, pt_b },
	{ "J", 0xd0, 12, pt_b },
	{ NULL, }
};

static void print_n(const char *name, int last)
{
	int i;

	printf("%s ", name);
	for (i = 0; i <= last; i++)
		printf("%2d ", i);
	putc('\n');
}

/* ----- Read a pin -------------------------------------------------------- */

static const char *pin_a(int con, int dat)
{
	if (con)
		return dat ? "F1" : "F0";
	else
		return dat ? ">1" : ">0";
}


static const char *pin_b(int con, int dat, int pud)
{
	static char res[4];

	res[0] = " >FX"[con];
	res[1] = dat ? '1' : '0';
	res[2] = pud ? ' ' : 'R';
	return res;
}


static const char *pin(const struct port *p, int num)
{
	uint32_t con, dat, pud;

	con = *(uint32_t *) (mem+p->offset);
	dat = *(uint32_t *) (mem+p->offset+4);
	if (p->type == pt_a)
		return pin_a((con >> num) & 1, (dat >> num) & 1);
	else {
		pud = *(uint32_t *) (mem+p->offset+8);
		return pin_b((con >> (num*2)) & 3, (dat >> num) & 1,
		    (pud >> num) & 1);
	}
}


/* ----- Set a pin --------------------------------------------------------- */


static void set_a(const struct port *p, int num, int c, int d)
{
	uint32_t con, dat;

	con = *(uint32_t *) (mem+p->offset);
	con = (con & ~(1 << num)) | ((c == CON_F1) << num);
	*(uint32_t *) (mem+p->offset) = con;
	
	if (d != -1) {
		dat = *(uint32_t *) (mem+p->offset+4);
		dat = (dat & ~(1 << num)) | (d << num);
		*(uint32_t *) (mem+p->offset+4) = dat;
	}
}


static void set_b(const struct port *p, int num, int c, int d, int r)
{
	uint32_t con, dat, pud;

	con = *(uint32_t *) (mem+p->offset);
	con = (con & ~(3 << (num*2))) | (c << (num*2));
	*(uint32_t *) (mem+p->offset) = con;
	
	if (d != -1) {
		dat = *(uint32_t *) (mem+p->offset+4);
		dat = (dat & ~(1 << num)) | (d << num);
		*(uint32_t *) (mem+p->offset+4) = dat;
	}

	pud = *(uint32_t *) (mem+p->offset+8);
	pud = (pud & ~(1 << num)) | (!r << num);
	*(uint32_t *) (mem+p->offset+8) = pud;
}


static int set_pin(const struct port *p, int num, int c, int d, int r)
{
	if (num > p->last) {
		printf("invalid pin %s%d\n", p->name, num);
		return -1;
	}
	if (p->type == pt_a) {
		if (r) {
			printf("pin %s%d has no pull-down\n", p->name, num);
			return -1;
		}
		if (c == CON_IN) {
			printf("pin %s%d cannot be an input\n", p->name, num);
			return -1;
		}
		if (c == CON_F2) {
			printf("pin %s%d has no second function\n",
				p->name, num);
			return -1;
		}
		set_a(p, num, c, d);
	}
	else
		set_b(p, num, c, d, r);

	return 0;
}

static size_t strcspn(const char *s, const char *reject)
{
	int len = strlen(s);
	int rej_len = strlen(reject);
	int i, j;

	for (i = 0; i < len; i++) {
		for (j = 0; j < rej_len; j++) {
			if (s[i] == reject[j])
				return i;
		}
	}
	return len;
}

static int port_op(const char *name, const char *op)
{
	const char *eq, *s;
	const struct port *p;
	int num, c, d = -1, r = 0;
	char *end;

	eq = strchr(op, '=');
	if (!eq)
		eq = strchr(op, 0);
	num = strcspn(op, "0123456789");
	if (!num || op+num >= eq)
		return -1;
	for (p = ports; p->name; p++)
		if (strlen(p->name) == num && !strncmp(p->name, op, num))
			break;
	if (!p->name) {
		printf("invalid port \"%.*s\"\n", num, op);
		return -1;
	}
	num = simple_strtoul(op+num, &end, 10);
	if (end != eq)
		return -1;
	if (!*eq) {
		s = pin(p, num);
		if (*s == ' ')
			s++;
		printf("%s\n", s);
		return 0;
	}
	switch (eq[1]) {
	case '0':
		d = 0;
		c = CON_OUT;
		break;
	case '1':
		d = 1;
		c = CON_OUT;
		break;
	case 'Z':
	case 'z':
		c = CON_IN;
		break;
	case 'R':
	case 'r':
		if (eq[2])
			return -1;
		c = CON_IN;
		r = 1;
		break;
	case 'F':
	case 'f':
		c = CON_F1;
		break;
	case 'X':
	case 'x':
		c = CON_F2;
		break;
	default:
		return -1;
	}
	if (eq[2]) {
		if (eq[2] != 'R' && eq[2] != 'r')
			return -1;
		if (eq[3])
			return -1;
		r = 1;
	}
	return set_pin(p, num, c, d, r);
}



/* ----- Dump all ports ---------------------------------------------------- */

static void dump_all(void)
{
	const struct port *p;
	uint32_t con, dat, pud;
	int i;

	for (p = ports; p->name; p++) {
		con = *(uint32_t *) (mem+p->offset);
		dat = *(uint32_t *) (mem+p->offset+4);
		if (p->type == pt_a) {
			print_n(p->name, p->last);
			printf("%*s ", strlen(p->name), "");
			for (i = 0; i <= p->last; i++)
				printf("%s ",
				    pin_a((con >> i) & 1, (dat >> i) & 1));
			putc('\n');
		}
		else {
			pud = *(uint32_t *) (mem+p->offset+8);
			print_n(p->name, p->last);
			printf("%*s ", strlen(p->name), "");
			for (i = 0; i <= p->last; i++)
				printf("%s",
				    pin_b((con >> (i*2)) & 3, (dat >> i) & 1,
				    (pud >> i) & 1));
			putc('\n');
		}
	}
}



int do_s3c24xx ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	if (!strcmp(argv[1], "speed")) {
		if (argc < 2)
			goto out_help;
		if (!strcmp(argv[2], "get"))
			print_cpu_speed();
		else if (!strcmp(argv[2], "list"))
			list_cpu_speeds();
		else if (!strcmp(argv[2], "set")) {
			unsigned long mhz;
			if (argc < 3)
				goto out_help;

			mhz = simple_strtoul(argv[3], NULL, 10);

			if (reconfig_mpll(mhz) < 0)
				printf("error, speed %uMHz unknown\n", mhz);
			else
				print_cpu_speed();
		} else
			goto out_help;
	} else if (!strcmp(argv[1], "gpio")) {
		if (argc < 2)
			goto out_help;
		else if (!strcmp(argv[2], "show"))
			dump_all();
		else if (!strcmp(argv[2], "set")) {
			int i;
			if (argc < 3)
				goto out_help;
			for (i = 3; i < argc; i++) {
				int rc;
				rc = port_op("s3c24xx-gpio", argv[i]);
				if (rc < 0)
					goto out_help;
			}
		} else
			goto out_help;
	} else {
out_help:
		printf("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	return 0;
}

/* -------------------------------------------------------------------- */


U_BOOT_CMD(
	s3c24xx,	4,	1,	do_s3c24xx,
	"s3c24xx - SoC  specific commands\n",
	"speed get - display current PLL speed config\n"
	"s3c24xx speed list - display supporte PLL speed configs\n"
	"s3c24xx speed set - set PLL speed\n"
	"s3c24xx gpio show - show current GPIO configuration\n"
	"s3c24xx gpio set - set one GPIO\n"
);

#endif
