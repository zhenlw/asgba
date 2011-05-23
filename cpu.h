/*
 * cpu.h
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

#ifndef CPU_H_
#define CPU_H_

enum ExpType {EXP_RESET = 0, EXP_UNDI, EXP_SWI, EXP_PABT, EXP_ABT, EXP_NONE, EXP_IRQ, EXP_FIQ};

void RaiseExp(ExpType eType, uint32_t ulSavedPc);

extern uint32_t g_cpsr;
extern uint32_t g_regs[17];
extern uint32_t &g_spsr;
extern uint32_t &g_pc;

extern uint32_t g_nirq;	//set to CPSR_FLAG_MASK_I when irq happens

#define CPSR_FLAG_MASK_V	(uint32_t(1) << 28)
#define CPSR_FLAG_MASK_C	(uint32_t(1) << 29)
#define CPSR_FLAG_MASK_Z	(uint32_t(1) << 30)
#define CPSR_FLAG_MASK_N	(uint32_t(1) << 31)

#define CPSR_FLAG_MASK_T	(uint32_t(1) << 5)

#define CPSR_FLAG_MASK_I	(uint32_t(1) << 7)

extern uint16_t g_usTicksThisPiece;

extern uint32_t g_ulPcDelta;	//2 or 4. for convenience. valid only when a piece of cpu codes is executing

//cpu modes, ignore the 5th 1 for easier operation
#define MODE_USR		0
#define MODE_FIQ		1UL
#define MODE_IRQ		2UL
#define MODE_SVC		3UL
#define MODE_ABT		7UL
#define MODE_UND		0xBUL
#define MODE_SYS		0xFUL

void SwitchRegs(uint32_t ulFrom, uint32_t ulTo);
void SwitchToMode(uint32_t ulNewCpsr);
void BackFromExp(uint32_t ulNewPc);

#define MAX_TICKS_PER_CPU_PIECE		64

#endif /* CPU_H_ */
