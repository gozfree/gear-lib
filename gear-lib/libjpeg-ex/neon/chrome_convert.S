
.fpu neon
#ifdef __thumb2__
.syntax unified
.thumb
#endif
.text

/* ARM */
#define IN	r0
#define U_OUT	r1
#define V_OUT	r2
#define ROW	r3
#define COL	r4
#define PITCH	r5
#define CURRENT_ROW	r6
#define CURRENT_COL	r7
#define SPACING	r8

/* NEON */
#define U	D0
#define V	D1


	.align 2
	.global chrome_convert
	.type	chrome_convert, %function
chrome_convert:
	push		{r4-r8,r10-r11,lr}
	vpush		{q4-q7}

	/* load arguments */
	ldmia		r0,	{IN, U_OUT, V_OUT, ROW, COL, PITCH}

	mov CURRENT_ROW, #0
	sub SPACING, PITCH, COL

LOOP_ROW:
	cmp CURRENT_ROW, ROW
	beq	END
	mov			CURRENT_COL, #0

LOOP_COL:
	cmp CURRENT_COL, COL
	addeq IN, IN, SPACING
	addeq CURRENT_ROW, CURRENT_ROW, #1
	beq LOOP_ROW
	vld2.8		{U,V},	[IN]!
	vst1.8		{U},	[U_OUT]!
	vst1.8		{V},	[V_OUT]!

	add CURRENT_COL, CURRENT_COL, #16
	b LOOP_COL

END:
	/* exit if all rows have been processed */
	vpople	{q4-q7}
	pople	{r4-r8,r10-r11,pc}
