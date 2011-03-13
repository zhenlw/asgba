#ifndef INTR_H
#define INTR_H

void SetIRQ(uint8_t idx);

#define INTR_INDEX_DMA0		8
#define INTR_INDEX_DMA1		9
#define INTR_INDEX_DMA2		10
#define INTR_INDEX_DMA3		11

#define INTR_INDEX_TMR0		3
#define INTR_INDEX_TMR1		4
#define INTR_INDEX_TMR2		5
#define INTR_INDEX_TMR3		6

#define INTR_INDEX_VBLK		0
#define INTR_INDEX_HBLK		1
#define INTR_INDEX_VCNT		2

#define INTR_INDEX_COMM		7
#define INTR_INDEX_KEY		12
#define INTR_INDEX_GPAK		13

#endif