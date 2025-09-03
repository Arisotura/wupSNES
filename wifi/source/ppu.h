#ifndef _PPU_H_
#define _PPU_H_

void PPU_Init();
void PPU_Reset(void);

void PPU_Write8(u32 addr, u8 val);
int PPU_DrawScanline(int line);
void PPU_VBlank(void);

void PPU_RunCommands(int index);

#endif
