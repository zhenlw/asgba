#ifndef ASGBA_H_
#define ASGBA_H_

#include "utilities.h"

//typedef uint8_t (*RenderPtr)[240][3];

bool AsgbaInit(AsgbaEvtHdlr hdlr, uint8_t *bios, uint8_t *rom, uint32_t romsz);
void AsgbaExec();

#endif
