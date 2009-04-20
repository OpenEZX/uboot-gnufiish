
_start:
	b 	reset
undefvec:
	b	undefvec
swivec:
	b	swivec
pabtvec:
	b	pabtvec
dabtvec:
	b	dabtvec
rsvdvec:
	b	rsvdvec
irqvec:
	b	irqvec
fiqvec:
	b	fiqvec

reset:
	/*
	 * set the cpu to SVC32 mode
	 */
	mrs	r0,cpsr
	bic	r0,r0,#0x1f
	orr	r0,r0,#0xd3
	msr	cpsr,r0

/* turn off the watchdog */
#define pWTCON		0x53000000
#define LOCKTIME	0x4C000000
#define	IRQ_BASE	0x4A000000
#define	SRCPND		0x00
#define INTMSK		0x08
#define INTPND		0x10
#define INTSUBMSK	0x1C

	ldr     r0, =pWTCON
	mov     r1, #0x0
	str     r1, [r0]

	mov	r1, #0xffffffff
	ldr	r0, =IRQ_BASE
	str	r1, [r0,#INTMSK]
	ldr	r1, =0x3ff
	str	r1, [r0,#INTSUBMSK]

	/* clear any pending interrupts.  This can only happen in case
	 * we do something like soft_reset_halt in OpenOCD, resulting
	 * in only the CPU core being reset, not the peripherals */
	ldr	r1, [r0,#SRCPND]
	str	r1, [r0,#SRCPND]
	ldr	r1, [r0,#INTPND]
	str	r1, [r0,#INTPND]

#if 0
	/* configure PLL */
	mov	r0, #LOCKTIME

	/* FCLK:HCLK:PCLK = 1:4:8 (500:125:62.5)*/
	mov	r1, #5
	str	r1, [r0,#14]		/* CLKDIVN */

	mov	r1, #0xffffff
	str	r1, [r0]
	ldr	r1, =0x3C042		/* 48MHz */
	str	r1, [r0,#8]		/* UPLLCON */
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ldr	r1, =0x6e021		/* 500MHz */
	str	r1, [r0,#4]		/* MPLLCON */
#endif

	bl	cpu_init_crit
	ldr	r0,=TEXT_BASE
	mov	pc, r0

cpu_init_crit:
	/*
	 * flush v4 I/D caches
	 */
	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0	/* flush v3/v4 cache */
	mcr	p15, 0, r0, c8, c7, 0	/* flush v4 TLB */

	/*
	 * disable MMU stuff and caches
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #0x00002300	@ clear bits 13, 9:8 (--V- --RS)
	bic	r0, r0, #0x00000087	@ clear bits 7, 2:0 (B--- -CAM)
	orr	r0, r0, #0x00000002	@ set bit 2 (A) Align
	orr	r0, r0, #0x00001000	@ set bit 12 (I) I-Cache
	mcr	p15, 0, r0, c1, c0, 0

	/*
	 * before relocating, we have to setup RAM timing
	 * because memory timing is board-dependend, you will
	 * find a lowlevel_init.S in your board directory.
	 */
	mov	ip, lr
	bl	lowlevel_init
	mov	lr, ip
	mov	pc, lr

