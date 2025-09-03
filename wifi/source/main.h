#ifndef _MAIN_H_
#define _MAIN_H_

#include "types.h"
#include "regs.h"

void* memset(void* dst, int val, unsigned long len);
void* memcpy(void* dst, const void* src, unsigned long len);

void EnableIRQ();
void DisableIRQ();
void WaitForIRQ();

void DelayUS(int us);

#endif
