/*
 * cpu.h
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

#ifndef CPU_H_
#define CPU_H_

enum ExpType {EXP_RESET = 0, EXP_UNDI, EXP_SWI, EXP_PABT, EXP_ABT, EXP_NONE, EXP_IRQ, EXP_FIQ};

void RaiseExp(ExpType eType, uint32_t ulPcDelta);

extern uint32_t g_cpsr;
extern uint32_t g_regs[17];
extern uint32_t &g_spsr;


#define CPSR_FLAG_MASK_V	(uint32_t(1) << 28)
#define CPSR_FLAG_MASK_C	(uint32_t(1) << 29)
#define CPSR_FLAG_MASK_Z	(uint32_t(1) << 30)
#define CPSR_FLAG_MASK_N	(uint32_t(1) << 31)

#endif /* CPU_H_ */
