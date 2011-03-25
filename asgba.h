#ifndef ASGBA_H_
#define ASGBA_H_

#include "utilities.h"

//typedef uint8_t (*RenderPtr)[240][3];

bool AsgbaInit(AsgbaEvtHdlr func, uint8_t *bios);
void AsgbaExec();

#endif
