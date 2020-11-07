#ifndef __EMI_HW_H__
#define __EMI_HW_H__

#define VER_BASE	0x08000000
#define EMI_BASE	0x10203000

#define EMI_CONA	(EMI_BASE + 0x000)	/* Address mapping */
#define EMI_CONB	(EMI_BASE + 0x008)	/* Data transfer overhead for specific read data size */
#define EMI_CONC	(EMI_BASE + 0x010)	/* Data transfer overhead for specific read data size */
#define EMI_COND	(EMI_BASE + 0x018)	/* Data transfer overhead for specific write data size */
#define EMI_CONE	(EMI_BASE + 0x020)	/* Data transfer overhead for specific write data size */
#define EMI_CONF	(EMI_BASE + 0x028)	/* Address scramble setting */
#define EMI_CONG	(EMI_BASE + 0x030)	/* Data transfer overhead for specific non-32bytes alignment read data size */
#define EMI_CONH	(EMI_BASE + 0x038)	/* Data transfer overhead for specific non-32bytes alignment write data size */
#define EMI_CONM	(EMI_BASE + 0x060)	/* MISC */
#define EMI_TESTB	(EMI_BASE + 0x0E8)	/* Test mode B. */
#define EMI_TESTC	(EMI_BASE + 0x0F0)	/* Test mode C. */
#define EMI_TESTD	(EMI_BASE + 0x0F8)	/* Test mode D. */
#define EMI_ARBA	(EMI_BASE + 0x100)	/* EMI bandwidth filter control APMCU */
#define EMI_ARBC	(EMI_BASE + 0x110)	/* EMI bandwidth filter control ARM9 */
#define EMI_ARBD	(EMI_BASE + 0x118)	/* EMI bandwidth filter control MDMCU */
#define EMI_ARBE	(EMI_BASE + 0x120)	/* EMI bandwidth filter control Modem Hardware */
#define EMI_ARBF	(EMI_BASE + 0x128)	/* EMI bandwidth filter control MM 0 */
#define EMI_ARBG	(EMI_BASE + 0x130)	/* EMI bandwidth filter control MM 1 */
#define EMI_ARBI	(EMI_BASE + 0x140)	/* Filter priority encode */
#define EMI_ARBI_2ND	(EMI_BASE + 0x144)	/* Filter priority encode for MD requirement */
#define EMI_ARBJ	(EMI_BASE + 0x148)	/* Turn around command number */
#define EMI_ARBJ_2ND	(EMI_BASE + 0x14C)	/* Turn around command number for MD requirement */
#define EMI_ARBK	(EMI_BASE + 0x150)	/* Page miss control */
#define EMI_ARBK_2ND	(EMI_BASE + 0x154)	/* Page miss control */
#define EMI_SLCT	(EMI_BASE + 0x158)	/* EMI slave control registers */
#define EMI_MPUA	(EMI_BASE + 0x160)	/* Memory protection unit control registers A */
#define EMI_MPUB	(EMI_BASE + 0x168)	/* Memory protection unit control registers B */
#define EMI_MPUC	(EMI_BASE + 0x170)	/* Memory protection unit control registers C */
#define EMI_MPUD	(EMI_BASE + 0x178)	/* Memory protection unit control registers D */
#define EMI_MPUE	(EMI_BASE + 0x180)	/* Memory protection unit control registers E */
#define EMI_MPUF	(EMI_BASE + 0x188)	/* Memory protection unit control registers F */
#define EMI_MPUG	(EMI_BASE + 0x190)	/* Memory protection unit control registers G */
#define EMI_MPUH	(EMI_BASE + 0x198)	/* Memory protection unit control registers H */
#define EMI_MPUI	(EMI_BASE + 0x1A0)	/* Memory protection unit control registers I */
#define EMI_MPUJ	(EMI_BASE + 0x1A8)	/* Memory protection unit control registers J */
#define EMI_MPUK	(EMI_BASE + 0x1B0)	/* Memory protection unit control registers K */
#define EMI_MPUL	(EMI_BASE + 0x1B8)	/* Memory protection unit control registers L */
#define EMI_MPUM	(EMI_BASE + 0x1C0)	/* Memory protection unit control registers M */
#define EMI_MPUN	(EMI_BASE + 0x1C8)	/* Memory protection unit control registers N */
#define EMI_MPUO	(EMI_BASE + 0x1D0)	/* Memory protection unit control registers O */
#define EMI_MPUP	(EMI_BASE + 0x1D8)	/* Memory protection unit control registers P */
#define EMI_MPUQ	(EMI_BASE + 0x1E0)	/* Memory protection unit control registers Q */
#define EMI_MPUR	(EMI_BASE + 0x1E8)	/* Memory protection unit control registers R */
#define EMI_MPUS	(EMI_BASE + 0x1F0)	/* Memory protection unit control registers S */
#define EMI_MPUT	(EMI_BASE + 0x1F8)	/* Memory protection unit control registers T */
#define EMI_MPUU	(EMI_BASE + 0x200)	/* Memory protection unit control registers U */
#define EMI_MPUY	(EMI_BASE + 0x220)	/* Memory protection unit control registers Y */
#define EMI_BMEN	(EMI_BASE + 0x400)	/* EMI bus monitor control registers */
#define EMI_BCNT	(EMI_BASE + 0x408)	/* EMI bus cycle counters */
#define EMI_TACT	(EMI_BASE + 0x410)	/* EMI total transaction counters */
#define EMI_TSCT	(EMI_BASE + 0x418)	/* EMI 1st transaction counters */
#define EMI_WACT	(EMI_BASE + 0x420)	/* EMI total word counters */
#define EMI_WSCT	(EMI_BASE + 0x428)	/* EMI 1st word counters */
#define EMI_BACT	(EMI_BASE + 0x430)	/* EMI bandwidth word counters */
#define EMI_BSCT	(EMI_BASE + 0x438)	/* EMI overhead word counters */
#define EMI_MSEL	(EMI_BASE + 0x440)	/* EMI master selection for 2nd and 3rd counters */
#define EMI_TSCT2	(EMI_BASE + 0x448)	/* EMI 2nd transaction counters of selected masters */
#define EMI_TSCT3	(EMI_BASE + 0x450)	/* EMI 3rd transaction counters of selected masters */
#define EMI_WSCT2	(EMI_BASE + 0x458)	/* EMI 2nd double-word counter of selected masters. */
#define EMI_WSCT3	(EMI_BASE + 0x460)	/* EMI 3rd double-word counter of selected masters. */
#define EMI_WSCT4	(EMI_BASE + 0x464)	/* EMI 4th double-word counter of selected masters. */
#define EMI_MSEL2	(EMI_BASE + 0x468)	/* EMI master selection for 4th and 5th counters */
#define EMI_MSEL3	(EMI_BASE + 0x470)	/* EMI master selection for 6th and 7th counters */
#define EMI_MSEL4	(EMI_BASE + 0x478)	/* EMI master selection for 8th and 9th counters */
#define EMI_MSEL5	(EMI_BASE + 0x480)	/* EMI master selection for 10th and 11th counters */
#define EMI_MSEL6	(EMI_BASE + 0x488)	/* EMI master selection for 12th and 13th counters */
#define EMI_MSEL7	(EMI_BASE + 0x490)	/* EMI master selection for 14th and 15th counters */
#define EMI_MSEL8	(EMI_BASE + 0x498)	/* EMI master selection for 16th and 17th counters */
#define EMI_MSEL9	(EMI_BASE + 0x4A0)	/* EMI master selection for 18th and 19th counters */
#define EMI_MSEL10	(EMI_BASE + 0x4A8)	/* EMI master selection for 20th and 21th counters */
#define EMI_BMID0	(EMI_BASE + 0x4B0)	/* EMI ID selection for 1- 4 transaction type counters */
#define EMI_BMID1	(EMI_BASE + 0x4B8)	/* EMI ID selection for 5- 8 transaction type counters */
#define EMI_BMID2	(EMI_BASE + 0x4C0)	/* EMI ID selection for 9- 12 transaction type counters */
#define EMI_BMID3	(EMI_BASE + 0x4C8)	/* EMI ID selection for 13- 16 transaction type counters */
#define EMI_BMID4	(EMI_BASE + 0x4D0)	/* EMI ID selection for 17- 20 transaction type counters */
#define EMI_BMID5	(EMI_BASE + 0x4D8)	/* EMI ID selection for 21  transaction type counters */
#define EMI_BMEN1	(EMI_BASE + 0x4E0)	/* High priority Filter */
#define EMI_BMEN2	(EMI_BASE + 0x4E8)	/* EMI ID selection enabling */
#define EMI_TTYPE1	(EMI_BASE + 0x500)	/* EMI 1st transaction type counter of selected masters */
#define EMI_TTYPE2	(EMI_BASE + 0x508)	/* EMI 2nd transaction type counter of selected masters */
#define EMI_TTYPE3	(EMI_BASE + 0x510)	/* EMI 3rd transaction type counter of selected masters */
#define EMI_TTYPE4	(EMI_BASE + 0x518)	/* EMI 4th transaction type counter of selected masters */
#define EMI_TTYPE5	(EMI_BASE + 0x520)	/* EMI 5th transaction type counter of selected masters */
#define EMI_TTYPE6	(EMI_BASE + 0x528)	/* EMI 6th transaction type counter of selected masters */
#define EMI_TTYPE7	(EMI_BASE + 0x530)	/* EMI 7th transaction type counter of selected masters */
#define EMI_TTYPE8	(EMI_BASE + 0x538)	/* EMI 8th transaction type counter of selected masters */
#define EMI_TTYPE9	(EMI_BASE + 0x540)	/* EMI 9th transaction type counter of selected masters */
#define EMI_TTYPE10	(EMI_BASE + 0x548)	/* EMI 10th transaction type counter of selected masters */
#define EMI_TTYPE11	(EMI_BASE + 0x550)	/* EMI 11th transaction type counter of selected masters */
#define EMI_TTYPE12	(EMI_BASE + 0x558)	/* EMI 12th transaction type counter of selected masters */
#define EMI_TTYPE13	(EMI_BASE + 0x560)	/* EMI 13th transaction type counter of selected masters */
#define EMI_TTYPE14	(EMI_BASE + 0x568)	/* EMI 14th transaction type counter of selected masters */
#define EMI_TTYPE15	(EMI_BASE + 0x570)	/* EMI 15th transaction type counter of selected masters */
#define EMI_TTYPE16	(EMI_BASE + 0x578)	/* EMI 16th transaction type counter of selected masters */
#define EMI_TTYPE17	(EMI_BASE + 0x580)	/* EMI 17th transaction type counter of selected masters */
#define EMI_TTYPE18	(EMI_BASE + 0x588)	/* EMI 18th transaction type counter of selected masters */
#define EMI_TTYPE19	(EMI_BASE + 0x590)	/* EMI 19th transaction type counter of selected masters */
#define EMI_TTYPE20	(EMI_BASE + 0x598)	/* EMI 20th transaction type counter of selected masters */
#define EMI_TTYPE21	(EMI_BASE + 0x5A0)	/* EMI 21th transaction type counter of selected masters */

#endif	/* __EMI_HW_H__ */
