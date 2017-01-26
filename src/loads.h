#ifndef _LOADS_H_
#define _LOADS_H_

#include <io430.h>

#include "types.h"

enum Load_CmdTypeDef{
    LOAD1_ON,
    LOAD1_OFF,
    LOAD2_ON,
    LOAD2_OFF
};

void Loads_Init(void);
void Loads_Command(enum Load_CmdTypeDef cmd);

#endif

