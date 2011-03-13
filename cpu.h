/*
 * cpu.h
 *
 *  Created on: 2011-1-29
 *      Author: zlw
 */

#ifndef CPU_H_
#define CPU_H_

extern const uint32 MODE_USR; //ignore the 5th 1 for easier operation
extern const uint32 MODE_FIQ;
extern const uint32 MODE_IRQ;
extern const uint32 MODE_SVC;
extern const uint32 MODE_ABT;
extern const uint32 MODE_UND;
extern const uint32 MODE_SYS;

enum ExpType {EXP_RESET = 0, EXP_UNDI, EXP_SWI, EXP_PABT, EXP_ABT, EXP_NONE, EXP_IRQ, EXP_FIQ};

void RaiseExp(ExpType eType, uint32_t ulPcDelta);

extern uint32_t g_cpsr;
extern uint32_t g_regs[17];
extern uint32_t &g_spsr;
extern uint32_t &g_pc;

extern uint32_t g_nirq;	//set to CPSR_FLAG_MASK_I when irq happens

inline void SwitchRegs(uint32_t ulFrom, uint32_t ulTo);

#define CPSR_FLAG_MASK_V	(uint32_t(1) << 28)
#define CPSR_FLAG_MASK_C	(uint32_t(1) << 29)
#define CPSR_FLAG_MASK_Z	(uint32_t(1) << 30)
#define CPSR_FLAG_MASK_N	(uint32_t(1) << 31)

#define CPSR_FLAG_MASK_T	(uint32_t(1) << 5)

#define CPSR_FLAG_MASK_I	(uint32_t(1) << 7)

enum SystemEvent{EVT_NONE = 0, EVT_VBLK, EVT_HBLK, EVT_FIFO, EVT_VSTART, EVT_HSTART, EVT_FIFO_FULL};

#endif /* CPU_H_ */
