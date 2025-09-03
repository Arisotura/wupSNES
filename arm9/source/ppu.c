/*
    Copyright 2025 Arisotura

    This file is part of wupSNES.

    wupSNES is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    wupSNES is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS 
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along 
    with wupSNES. If not, see http://www.gnu.org/licenses/.
*/

#include <wup/wup.h>
#include <stdio.h>

#include "memory.h"


struct
{
    u16 VCount;

    u16 DispCnt;
    u16 SetIni;

    u16 BGMode;
    u16 BGMosaic;
    u16 BGEnable;
    u16 M7Mode;

    u16 ColorMath;
    u16 SubBackdrop;

    u16 BGOld;
    u16 M7Old;

    u16 BGScr[4];
    u16 BGChr[4];
    u16 BGXScr2[4];
    u16 BGYScr2[4];
    u16 BGXPos[4];
    u16 BGYPos[4];

    s16 M7XPos;
    s16 M7YPos;
    s16 M7XCenter;
    s16 M7YCenter;
    s16 M7A;
    s16 M7B;
    s16 M7C;
    s16 M7D;

    u16 OBJChr;
    u16 OBJGap;
    u16 OBJWidth[2];
    u16 OBJHeight[2];

    u16 WindowDirty;
    u16 WindowX[4];
    u16 BGWindowMask[4];
    u16 OBJWindowMask;
    u16 ColorMathWindowMask;
    u16 BGWindowCombine[4];
    u16 OBJWindowCombine;
    u16 ColorMathWindowCombine;
    u16 WindowEnable;

    u16 CGRAMAddr;
    u16 CGRAMVal;
    u16 VRAMInc;
    u16 VRAMStep;
    u16 VRAMAddr;
    u16 VRAMPref;
    u16 OAMAddr;
    u16 OAMVal;
    u16 OAMReload;
    u16 OAMPrio;
    u16 FirstOBJ;

    u16 OrigCGRAM[256];
    u16 CGRAM[256];
    u8 VRAM[0x10000];
    u8 OAM[0x220];

    u32 CGRAM32[256];

} PPU;


s16 PPU_MulA;
s8 PPU_MulB;
s32 PPU_MulResult;



u32 Mem_WRAMAddr;





u8 PPU_CmdBuf[1024];
u16 PPU_CmdBufPos;

u16 Planar2Linear2[256];
u32 Planar2Linear4[256];
u32 Planar2Linear4R[256];
u64 Planar2Linear8[256];

// TODO clean up this mess!
void Wifi_SendMail(int mb, u32 val);
void Wifi_WaitCmdList();
u32 PPU_RemotePtrs[3];
u8 PPU_CurRemoteCmdBuf;


void PPU_SetOBJCHR(u16 base, u16 gap);

void PPU_SetM7ScrollX(u32 val);
void PPU_SetM7ScrollY(u32 val);


void PPU_Init()
{
    for (u32 i = 0; i < 256; i++)
    {
        u16 o = 0;
        if (i & 0x0080) o |= 0x0001;
        if (i & 0x0040) o |= 0x0004;
        if (i & 0x0020) o |= 0x0010;
        if (i & 0x0010) o |= 0x0040;
        if (i & 0x0008) o |= 0x0100;
        if (i & 0x0004) o |= 0x0400;
        if (i & 0x0002) o |= 0x1000;
        if (i & 0x0001) o |= 0x4000;
        Planar2Linear2[i] = o;
    }

    for (u32 i = 0; i < 256; i++)
    {
        u32 o = 0;
        if (i & 0x80) o |= 0x00000001;
        if (i & 0x40) o |= 0x00000010;
        if (i & 0x20) o |= 0x00000100;
        if (i & 0x10) o |= 0x00001000;
        if (i & 0x08) o |= 0x00010000;
        if (i & 0x04) o |= 0x00100000;
        if (i & 0x02) o |= 0x01000000;
        if (i & 0x01) o |= 0x10000000;
        Planar2Linear4[i] = o;

        o = 0;
        if (i & 0x01) o |= 0x00000001;
        if (i & 0x02) o |= 0x00000010;
        if (i & 0x04) o |= 0x00000100;
        if (i & 0x08) o |= 0x00001000;
        if (i & 0x10) o |= 0x00010000;
        if (i & 0x20) o |= 0x00100000;
        if (i & 0x40) o |= 0x01000000;
        if (i & 0x80) o |= 0x10000000;
        Planar2Linear4R[i] = o;
    }

    for (u32 i = 0; i < 256; i++)
    {
        u64 o = 0;
        if (i & 0x0080) o |= 0x0000000000000001;
        if (i & 0x0040) o |= 0x0000000000000100;
        if (i & 0x0020) o |= 0x0000000000010000;
        if (i & 0x0010) o |= 0x0000000001000000;
        if (i & 0x0008) o |= 0x0000000100000000;
        if (i & 0x0004) o |= 0x0000010000000000;
        if (i & 0x0002) o |= 0x0001000000000000;
        if (i & 0x0001) o |= 0x0100000000000000;
        Planar2Linear8[i] = o;
    }
}

void PPU_Reset()
{
	memset(&PPU, 0, sizeof(PPU));

	PPU_MulA = 0;
	PPU_MulB = 0;
	PPU_MulResult = 0;
		
		
	Mem_WRAMAddr = 0;


    memset(PPU_CmdBuf, 0, sizeof(PPU_CmdBuf));
    PPU_CmdBufPos = 0;
    PPU_CurRemoteCmdBuf = 0;

    Wifi_SendMail(1, 0);
}


void PPU_SetOBJCnt(u16 val)
{
    PPU.OBJChr = (val & 0x03) << 13;
    PPU.OBJGap = (val & 0x18) << 9;

    switch (val >> 5)
    {
        case 0: PPU.OBJWidth[0] = 8; PPU.OBJHeight[0] = 8; PPU.OBJWidth[1] = 16; PPU.OBJHeight[1] = 16; break;
        case 1: PPU.OBJWidth[0] = 8; PPU.OBJHeight[0] = 8; PPU.OBJWidth[1] = 32; PPU.OBJHeight[1] = 32; break;
        case 2: PPU.OBJWidth[0] = 8; PPU.OBJHeight[0] = 8; PPU.OBJWidth[1] = 64; PPU.OBJHeight[1] = 64; break;
        case 3: PPU.OBJWidth[0] = 16; PPU.OBJHeight[0] = 16; PPU.OBJWidth[1] = 32; PPU.OBJHeight[1] = 32; break;
        case 4: PPU.OBJWidth[0] = 16; PPU.OBJHeight[0] = 16; PPU.OBJWidth[1] = 64; PPU.OBJHeight[1] = 64; break;
        case 5: PPU.OBJWidth[0] = 32; PPU.OBJHeight[0] = 32; PPU.OBJWidth[1] = 64; PPU.OBJHeight[1] = 64; break;
        case 6: PPU.OBJWidth[0] = 16; PPU.OBJHeight[0] = 32; PPU.OBJWidth[1] = 32; PPU.OBJHeight[1] = 64; break;
        case 7: PPU.OBJWidth[0] = 16; PPU.OBJHeight[0] = 32; PPU.OBJWidth[1] = 32; PPU.OBJHeight[1] = 32; break;
    }
}

void PPU_SetBGScr(u16 num, u16 val)
{
    PPU.BGScr[num] = val;//(val & 0x7C) << 8;

    switch (val & 0x3)
    {
        case 0: PPU.BGXScr2[num] = 0;    PPU.BGYScr2[num] = 0;    break;
        case 1: PPU.BGXScr2[num] = 1024; PPU.BGYScr2[num] = 0;    break;
        case 2: PPU.BGXScr2[num] = 0;    PPU.BGYScr2[num] = 1024; break;
        case 3: PPU.BGXScr2[num] = 1024; PPU.BGYScr2[num] = 2048; break;
    }
}

void PPU_SetBGXPos(u16 num, u16 val)
{
    PPU.BGXPos[num] = (val << 8) | (PPU.BGOld & 0xF8) | ((PPU.BGXPos[num] >> 8) & 0x7);
    PPU.BGOld = val;
}

void PPU_SetBGYPos(u16 num, u16 val)
{
    PPU.BGYPos[num] = (val << 8) | PPU.BGOld;
    PPU.BGOld = val;
}

u32 PPU_TranslateVRAMAddress(u32 addr)
{
    switch (PPU.VRAMInc & 0x0C)
    {
        case 0x00: return addr;

        case 0x04:
            return (addr & 0x1FE01) |
                   ((addr & 0x001C0) >> 5) |
                   ((addr & 0x0003E) << 3);

        case 0x08:
            return (addr & 0x1FC01) |
                   ((addr & 0x00380) >> 6) |
                   ((addr & 0x0007E) << 3);

        case 0x0C:
            return (addr & 0x1F801) |
                   ((addr & 0x00700) >> 7) |
                   ((addr & 0x000FE) << 3);
    }

    // herp
    return addr;
}




void PPU_LatchHVCounters()
{
	// TODO simulate this one based on CPU cycle counter
	/*PPU_OPHCT = 22;
	
	PPU_OPVCT = 1 + PPU_VCount;
	if (PPU_OPVCT > 261) PPU_OPVCT = 0;
	
	PPU_OPLatch = 0x40;*/
}

int fr = 0;
void PPU_FlushCmdBuf()
{
    if (PPU_CmdBufPos == 0)
        return;

    if (PPU_CmdBufPos < 1024)
        PPU_CmdBuf[PPU_CmdBufPos++] = 0xFF;

    /*fr++;
    if (fr == 100)
    {
        u8* derp = malloc(0x2000);
        for (int i = 0; i < 0x10000; i+=0x2000)
        {
            SDIO_ReadF1Memory(PPU_RemotePtrs[2]+i, derp, 0x2000);
            void dump_data(u8* data, int len);
            dump_data(&derp[i], 0x2000);
        }
        free(derp);
    }*/

printf("flushzorz %d\n", Mem_Status->VCount);
    u32 raddr = PPU_RemotePtrs[0] + (PPU_CurRemoteCmdBuf * 1024);
    u32 rlen = (PPU_CmdBufPos + 0x3F) & ~0x3F;
    SDIO_WriteF1Memory(raddr, PPU_CmdBuf, rlen);

    Wifi_SendMail(0, PPU_CurRemoteCmdBuf);
    Wifi_WaitCmdList();
    printf("waited\n");

    PPU_CurRemoteCmdBuf ^= 1;
    PPU_CmdBufPos = 0;
}

void PPU_SendCmd(u8 cmd, u8 param)
{return;
    int len = 1;
    if ((cmd >> 6) == 0)
        len = 2;

    if ((PPU_CmdBufPos + len) > 1024)
        PPU_FlushCmdBuf();
    else if ((cmd == 0xC0) && (!(Mem_Status->VCount & 0x3F)))
        PPU_FlushCmdBuf();
    else if (cmd == 0x80)
        PPU_FlushCmdBuf();

    PPU_CmdBuf[PPU_CmdBufPos++] = cmd;
    if (len > 1)
        PPU_CmdBuf[PPU_CmdBufPos++] = param;
}


u8 _tempbuf[256 * 64 * 2];
int z = 0;
void PPU_Present(int line)
{return;
    extern u16* Framebuffer;
    u16* buf = &Framebuffer[line * 256];

    u32 fark=0;
    SDIO_ReadF1Memory(0x40000, &fark, 4);

    printf("presentzored %d %d - time=%d\n", line, z++, fark);

    u32 raddr = PPU_RemotePtrs[1];
    u32 rlen = 256 * 64 * 2;
    SDIO_ReadF1Memory(raddr, _tempbuf, rlen);
    
    int h = 64;
    if ((line + h) > 224)
        h = 224 - line;
    for (int y = 0; y < h; y++)
    {
        /*u8* buf_main = &_tempbuf[y * 512];
        u8* buf_sub =  &_tempbuf[y * 512 + 256];

        for (int x = 0; x < 256; x++)
        {
            // TODO color math!
            u16 col_main = PPU_Palette[buf_main[x]];

            *buf++ = col_main;
        }*/
        u16* sbuf = (u16*)&_tempbuf[y * 512];
        for (int x = 0; x < 256; x++)
        {
            *buf++ = sbuf[x];
        }
    }
}


// I/O
// addr = lowest 8 bits of address in $21xx range

u8 PPU_Read8(u32 addr)
{
	u8 ret = 0;
	switch (addr)
	{
#if 0
		case 0x34: ret = PPU_MulResult & 0xFF; break;
		case 0x35: ret = (PPU_MulResult >> 8) & 0xFF; break;
		case 0x36: ret = (PPU_MulResult >> 16) & 0xFF; break;
		
		case 0x37:
			PPU_LatchHVCounters();
			break;
			
		case 0x38:
			ret = PPU_OAM[PPU_OAMAddr];
			PPU_OAMAddr++;
			break;
		
		case 0x39:
			{
				ret = PPU_VRAMPref & 0xFF;
				if (!(PPU_VRAMInc & 0x80))
				{
					PPU_VRAMPref = *(u16*)&PPU_VRAM[(PPU_VRAMAddr << 1) & 0xFFFEFFFF];
					PPU_VRAMAddr += PPU_VRAMStep;
				}
			}
			break;
		case 0x3A:
			{
				ret = PPU_VRAMPref >> 8;
				if (PPU_VRAMInc & 0x80)
				{
					PPU_VRAMPref = *(u16*)&PPU_VRAM[(PPU_VRAMAddr << 1) & 0xFFFEFFFF];
					PPU_VRAMAddr += PPU_VRAMStep;
				}
			}
			break;
			
		case 0x3C:
			if (PPU_OPHFlag)
			{
				PPU_OPHFlag = 0;
				ret = PPU_OPHCT >> 8;
			}
			else
			{
				PPU_OPHFlag = 1;
				ret = PPU_OPHCT & 0xFF;
			}
			break;
		case 0x3D:
			if (PPU_OPVFlag)
			{
				PPU_OPVFlag = 0;
				ret = PPU_OPVCT >> 8;
			}
			else
			{
				PPU_OPVFlag = 1;
				ret = PPU_OPVCT & 0xFF;
			}
			break;
			
		case 0x3E: ret = 0x01; break;
		case 0x3F: 
			ret = 0x01 | (ROM_Region ? 0x10 : 0x00) | PPU_OPLatch;
			PPU_OPLatch = 0;
			PPU_OPHFlag = 0;
			PPU_OPVFlag = 0;
			break;
#endif
		case 0x40: ret = SPC_IOPorts[4]; break;
		case 0x41: ret = SPC_IOPorts[5]; break;
		case 0x42: ret = SPC_IOPorts[6]; break;
		case 0x43: ret = SPC_IOPorts[7]; break;

		case 0x80: ret = Mem_SysRAM[Mem_WRAMAddr++]; break;
	}

	return ret;
}

u16 PPU_Read16(u32 addr)
{
	u16 ret = 0;
	switch (addr)
	{
		// not in the right place, but well
		// our I/O functions are mapped to the whole $21xx range
		
		//case 0x40: ret = *(u16*)&SPC_IOPorts[4]; break;
		//case 0x42: ret = *(u16*)&SPC_IOPorts[6]; break;
		
		default:
			ret = PPU_Read8(addr);
			ret |= (PPU_Read8(addr+1) << 8);
			break;
	}

	return ret;
}
int numio=0;
void PPU_Write8(u32 addr, u8 val)
{
	if (addr < 0x40)
	{//printf("PPU WRITE %02X %02X\n", addr, val);
        PPU_SendCmd(addr, val);
	}
	//printf("PPU write8 %08X %02X\n", addr, val);
	switch (addr)
	{
        case 0x00: PPU.DispCnt = val; break;
        case 0x01: PPU_SetOBJCnt(val); break;

        case 0x02:
            PPU.OAMAddr = (PPU.OAMAddr & 0x100) | val;
            PPU.OAMReload = PPU.OAMAddr;
            PPU.FirstOBJ = PPU.OAMPrio ? ((PPU.OAMAddr >> 1) & 0x7F) : 0;
            break;
        case 0x03:
            PPU.OAMAddr = (PPU.OAMAddr & 0xFF) | ((val & 0x01) << 8);
            PPU.OAMPrio = val & 0x80;
            PPU.OAMReload = PPU.OAMAddr;
            PPU.FirstOBJ = PPU.OAMPrio ? ((PPU.OAMAddr >> 1) & 0x7F) : 0;
            break;

        case 0x04:
            if (PPU.OAMAddr >= 0x200)
            {
                PPU.OAM[PPU.OAMAddr & 0x21F] = val;
            }
            else if (PPU.OAMAddr & 0x1)
            {
                *(u16*)&PPU.OAM[PPU.OAMAddr - 1] = PPU.OAMVal | (val << 8);
            }
            else
            {
                PPU.OAMVal = val;
            }
            PPU.OAMAddr++;
            PPU.OAMAddr &= ~0x400;
            break;

        case 0x05: PPU.BGMode = val; break;
        case 0x06: PPU.BGMosaic = val; break;

        case 0x07: PPU_SetBGScr(0, val); break;
        case 0x08: PPU_SetBGScr(1, val); break;
        case 0x09: PPU_SetBGScr(2, val); break;
        case 0x0A: PPU_SetBGScr(3, val); break;

        case 0x0B:
            PPU.BGChr[0] = (val & 0x07) << 12;
            PPU.BGChr[1] = (val & 0x70) << 8;
            break;
        case 0x0C:
            PPU.BGChr[2] = (val & 0x07) << 12;
            PPU.BGChr[3] = (val & 0x70) << 8;
            break;

        case 0x0D:
            PPU_SetBGXPos(0, val);
            PPU.M7XPos = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x0E:
            PPU_SetBGYPos(0, val);
            PPU.M7YPos = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x0F: PPU_SetBGXPos(1, val); break;
        case 0x10: PPU_SetBGYPos(1, val); break;
        case 0x11: PPU_SetBGXPos(2, val); break;
        case 0x12: PPU_SetBGYPos(2, val); break;
        case 0x13: PPU_SetBGXPos(3, val); break;
        case 0x14: PPU_SetBGYPos(3, val); break;

        case 0x15:
            PPU.VRAMInc = val;
            switch (val & 0x03)
            {
                case 0x00: PPU.VRAMStep = 2; break;
                case 0x01: PPU.VRAMStep = 64; break;
                case 0x02:
                case 0x03: PPU.VRAMStep = 256; break;
            }
            break;

        case 0x16:
            PPU.VRAMAddr &= 0xFE00;
            PPU.VRAMAddr |= (val << 1);
            addr = PPU_TranslateVRAMAddress(PPU.VRAMAddr);
            PPU.VRAMPref = *(u16*)&PPU.VRAM[addr];
            break;
        case 0x17:
            PPU.VRAMAddr &= 0x01FE;
            PPU.VRAMAddr |= ((val & 0x7F) << 9);
            addr = PPU_TranslateVRAMAddress(PPU.VRAMAddr);
            PPU.VRAMPref = *(u16*)&PPU.VRAM[addr];
            break;

        case 0x18: // VRAM shit
            {
                addr = PPU_TranslateVRAMAddress(PPU.VRAMAddr);
                PPU.VRAM[addr] = val;

                if (!(PPU.VRAMInc & 0x80))
                    PPU.VRAMAddr += PPU.VRAMStep;
            }
            break;
        case 0x19:
            {
                addr = PPU_TranslateVRAMAddress(PPU.VRAMAddr);
                PPU.VRAM[addr+1] = val;

                if (PPU.VRAMInc & 0x80)
                    PPU.VRAMAddr += PPU.VRAMStep;
            }
            break;

        case 0x1A:
            PPU.M7Mode = val;
            break;
        case 0x1B:
            PPU.M7A = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x1C:
            PPU.M7B = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x1D:
            PPU.M7C = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x1E:
            PPU.M7D = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x1F:
            PPU.M7XCenter = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;
        case 0x20:
            PPU.M7YCenter = (s16)((val << 8) | PPU.M7Old);
            PPU.M7Old = val;
            break;

        case 0x21:
            PPU.CGRAMAddr = val;
            break;

        case 0x22:
            if (!(PPU.CGRAMAddr & 0x100))
            {
                PPU.CGRAMVal = val;
                PPU.CGRAMAddr |= 0x100;
            }
            else
            {
                u16 color = PPU.CGRAMVal | (val << 8);
                PPU.OrigCGRAM[PPU.CGRAMAddr & 0xFF] = color;
                PPU.CGRAM[PPU.CGRAMAddr & 0xFF] = ((color & 0x001F) << 10) | (color & 0x03E0) | ((color & 0x7C00) >> 10);
                PPU.CGRAM32[PPU.CGRAMAddr & 0xFF] = PPU.CGRAM[PPU.CGRAMAddr & 0xFF] | (PPU.CGRAM[PPU.CGRAMAddr & 0xFF] << 16);
                PPU.CGRAMAddr++;
                PPU.CGRAMAddr &= 0xFF;
            }
            break;

        case 0x23:
            PPU.BGWindowMask[0] = val & 0xF;
            PPU.BGWindowMask[1] = val >> 4;
            PPU.WindowDirty = 1;
            break;
        case 0x24:
            PPU.BGWindowMask[2] = val & 0xF;
            PPU.BGWindowMask[3] = val >> 4;
            PPU.WindowDirty = 1;
            break;
        case 0x25:
            PPU.OBJWindowMask = val & 0xF;
            PPU.ColorMathWindowMask = val >> 4;
            PPU.WindowDirty = 1;
            break;

        case 0x26:
            PPU.WindowX[0] = val;
            PPU.WindowDirty = 1;
            break;
        case 0x27:
            PPU.WindowX[1] = val;
            PPU.WindowDirty = 1;
            break;
        case 0x28:
            PPU.WindowX[2] = val;
            PPU.WindowDirty = 1;
            break;
        case 0x29:
            PPU.WindowX[3] = val;
            PPU.WindowDirty = 1;
            break;

        case 0x2A:
            PPU.BGWindowCombine[0] = val & 0x3;
            PPU.BGWindowCombine[1] = (val >> 2) & 0x3;
            PPU.BGWindowCombine[2] = (val >> 4) & 0x3;
            PPU.BGWindowCombine[3] = (val >> 6) & 0x3;
            PPU.WindowDirty = 1;
            break;
        case 0x2B:
            PPU.OBJWindowCombine = val & 0x3;
            PPU.ColorMathWindowCombine = (val >> 2) & 0x3;
            PPU.WindowDirty = 1;
            break;

        case 0x2C: PPU.BGEnable = (PPU.BGEnable & 0xFF00) | val; break;
        case 0x2D: PPU.BGEnable = (PPU.BGEnable & 0x00FF) | (val << 8); break;

        case 0x2E: PPU.WindowEnable = (PPU.WindowEnable & 0xFF00) | val; break;
        case 0x2F: PPU.WindowEnable = (PPU.WindowEnable & 0x00FF) | (val << 8); break;

        case 0x30: PPU.ColorMath = (PPU.ColorMath & 0xFF00) | val; break;
        case 0x31: PPU.ColorMath = (PPU.ColorMath & 0x00FF) | (val << 8); break;

        case 0x32:
            addr = val & 0x1F;
            if (val & 0x20) PPU.SubBackdrop = (PPU.SubBackdrop & 0x7FE0) | addr;
            if (val & 0x40) PPU.SubBackdrop = (PPU.SubBackdrop & 0x7C1F) | (addr << 5);
            if (val & 0x80) PPU.SubBackdrop = (PPU.SubBackdrop & 0x03FF) | (addr << 10);
            break;

        case 0x33: PPU.SetIni = val; break;

		case 0x40: SPC_IOPorts[0] = val; break;
		case 0x41: SPC_IOPorts[1] = val; break;
		case 0x42: SPC_IOPorts[2] = val; break;
		case 0x43: SPC_IOPorts[3] = val; break;
		
		case 0x80: Mem_SysRAM[Mem_WRAMAddr++] = val; break;
		case 0x81: Mem_WRAMAddr = (Mem_WRAMAddr & 0x0001FF00) | val; break;
		case 0x82: Mem_WRAMAddr = (Mem_WRAMAddr & 0x000100FF) | (val << 8); break;
		case 0x83: Mem_WRAMAddr = (Mem_WRAMAddr & 0x0000FFFF) | ((val & 0x01) << 16); break;
				
		default:
			//printf("PPU_Write8(%08X, %08X)\n", addr, val);
			break;
	}
}

void PPU_Write16(u32 addr, u16 val)
{
	switch (addr)
	{
		// optimized route
		
		/*case 0x16:
			PPU_VRAMAddr = val;
			PPU_VRAMPref = *(u16*)&PPU_VRAM[(PPU_VRAMAddr << 1) & 0xFFFEFFFF];
			break;*/
			
		/*case 0x40: *(u16*)&IPC->SPC_IOPorts[0] = val; break;
		case 0x41: IPC->SPC_IOPorts[1] = val & 0xFF; IPC->SPC_IOPorts[2] = val >> 8; break;
		case 0x42: *(u16*)&IPC->SPC_IOPorts[2] = val; break;*/
		
		case 0x43: printf("!! write $21%02X %04X\n", addr, val); break;
		
		case 0x81: Mem_WRAMAddr = (Mem_WRAMAddr & 0x00010000) | val; break;
		
		// otherwise, just do two 8bit writes
		default:
			PPU_Write8(addr, val & 0x00FF);
			PPU_Write8(addr+1, val >> 8);
			break;
	}
}



void PPU_DrawBG_4bpp_8x8(u16 num, u32* dst, u16 ypos)
{
    //u16* vram = (u16*)PPU.VRAM;
    register u16* tileset = &((u16*)PPU.VRAM)[PPU.BGChr[num]];
    register u16* tilemap = &((u16*)PPU.VRAM)[(PPU.BGScr[num] & 0xFC) << 8];
    //register u32* p2l = Planar2Linear4;
    //u16 tileset = PPU.BGChr[num];// << 12;

    ypos += PPU.BGYPos[num];

    u16 bgscr = PPU.BGScr[num];
    //u16 tilemap = ((bgscr & 0xFC) << 8) + ((ypos & 0xF8) << 2);
    tilemap += ((ypos & 0xF8) << 2);
    if ((ypos & 0x100) && (bgscr & 0x2))
        tilemap += (bgscr & 0x1) ? 2048 : 1024;

    u16 xpos = PPU.BGXPos[num];
    tilemap += ((xpos & 0xF8) >> 3);
    if ((xpos & 0x100) && (bgscr & 0x1))
        tilemap += 1024;

    int xoff = xpos & 0x7;
    dst -= xoff;

    int w = xoff ? 33 : 32;
    for (int i = 0; i < w; i++)
    {
        //u32 t1 = WREG_CPU_TICKCOUNT;
        /*u16 tmaddr = (xpos & 0xF8) >> 3;
        if ((xpos & 0x100) && (bgscr & 0x1))
            tmaddr += 1024;
        xpos += 8;*/

        // FIXME it leaks out!
        u32 mask = 0xFFFFFFFF;
        if (i == 0) mask <<= (xoff * 4);
        else if (i == 32) mask >>= ((8-xoff) * 4);

        //u32 t2 = WREG_CPU_TICKCOUNT;
        // load tile
        //u16 curtile = vram[tilemap + tmaddr];
        u16 curtile = *tilemap++;
        u32* pal = &PPU.CGRAM32[(curtile & 0x1C00) >> 6];
        //u32 t2 = WREG_CPU_TICKCOUNT;
        // expensive part?
        u16 tsaddr = (curtile & 0x03FF) << 4;
        if (curtile & 0x8000) tsaddr += (7 - (ypos & 0x7));
        else                  tsaddr += (ypos & 0x7);

        //u32 t3 = WREG_CPU_TICKCOUNT;
        //u16 tileraw1 = vram[tileset + tsaddr];
        //u16 tileraw2 = vram[tileset + tsaddr + 8];
        register u32* p2l = (curtile & 0x4000) ? Planar2Linear4R : Planar2Linear4;
        u16 tileraw1 = tileset[tsaddr];
        u16 tileraw2 = tileset[tsaddr + 8];
        u32 tiledata = p2l[tileraw1 & 0xFF] |
                       (p2l[tileraw1 >> 8] << 1) |
                       (p2l[tileraw2 & 0xFF] << 2) |
                       (p2l[tileraw2 >> 8] << 3);

        tiledata &= mask;
        /*if (!tiledata)
        {
            dst += 8;
            continue;
        }*/
        //u32 t4 = WREG_CPU_TICKCOUNT;
        //u32 t1 = WREG_CPU_TICKCOUNT;

        u32 c;
        c = tiledata & 0xF;
        if (c) dst[0] = pal[c];
        c = (tiledata >> 4) & 0xF;
        if (c) dst[1] = pal[c];
        c = (tiledata >> 8) & 0xF;
        if (c) dst[2] = pal[c];
        c = (tiledata >> 12) & 0xF;
        if (c) dst[3] = pal[c];
        c = (tiledata >> 16) & 0xF;
        if (c) dst[4] = pal[c];
        c = (tiledata >> 20) & 0xF;
        if (c) dst[5] = pal[c];
        c = (tiledata >> 24) & 0xF;
        if (c) dst[6] = pal[c];
        c = (tiledata >> 28) & 0xF;
        if (c) dst[7] = pal[c];

        dst += 8;

        //u32 t5 = WREG_CPU_TICKCOUNT;
        //*(vu32 *) 0x40000 += t3-t2;//t2 - t1;
        xpos += 8;
        if (!(xpos & 0xF8))
        {
            // wraparound in 256x256 block
            if (bgscr & 0x1)
            {
                if (xpos & 0x100)
                    tilemap += (1024-32);
                else
                    tilemap -= (1024+32);
            }
            else
            {
                tilemap -= 32;
            }
        }
    }
}


u32 drawtime;
void PPU_DrawScanline(int line)
{
    if (line < 1)
    {
        // blank
        // TODO
        // also should still do sprites for next line
        //return;
    }

    // TODO

    //u8* buf_main = &ScanlineBuf[pos * 512];
    //u8* buf_sub  = &ScanlineBuf[pos * 512 + 256];
    //u8 temp[256];
    extern u16* Framebuffer;
    u16* buf_main = &Framebuffer[line * 512*2];

    //if (!(line & 1))
    {
        /*for (int i = 0; i < 256; i++)
        {
            buf_main[i] = 0;
            buf_sub[i] = 0;
            //temp[i] = 0;
        }*/
        if (line == 0) drawtime = 0;

        // TEST
        //u32 t1 = WREG_CPU_TICKCOUNT;

        /*u32 backdrop = PPU.CGRAM[0];
        backdrop |= (backdrop << 16);
        for (int i = 0; i < 256; i+=2)
        {
            *(u32*)&buf_main[i*2] = backdrop;
        }*/
        u32 t1 = REG_COUNTUP_VALUE;
        /*u16 backdrop = PPU.CGRAM[0];
        for (int i = 0; i < 256; i++)
        {
            buf_main[i] = backdrop;
        }*/
        u32 backdrop = PPU.CGRAM[0];
        backdrop |= (backdrop << 16);
        for (int i = 0; i < 512; i+=2)
        {
            *(u32*)&buf_main[i] = backdrop;
        }

        //u32 t1 = REG_COUNTUP_VALUE;

        PPU_DrawBG_4bpp_8x8(0, (u32*)buf_main, line);
        //PPU_DrawBG_2bpp_8x8(2, (u16*)buf_main, line);
        /*PPU_DrawBG_4bpp_8x8(0, temp, line);
        PPU_DrawBG_2bpp_8x8(2, temp, line);
        for (int i = 0; i < 256; i++)
        {
            ((u16*)buf_main)[i] = PPU.CGRAM[temp[i]];
        }*/
        u32 t2 = REG_COUNTUP_VALUE;
        drawtime += t2 - t1;
    }

}



int lastvc = 0;
int zarp=0;
int nvbl = 0;

void PPU_SNESVBlank()
{
	//printf("IO: %d\n", numio); numio = 0;
	//printf("VBLANK %d/%d\n", zarp, Mem_Status->ScreenHeight); zarp++;
    PPU.OAMAddr = PPU.OAMReload;
	//dsp_sendData(0, 0x8000);
	//printf("zorp\n");
	//printf("VBlank: %d\n", nvbl);
    //printf("vblank: draw time = %d\n", drawtime);

    PPU_SendCmd(0x80, 0);

    // herpderp
    extern u16* Framebuffer;
    GPDMA_BlitTransfer(0, Framebuffer, 1024*2, Framebuffer+512, 1024*2, 512*2, 256*480*2);
    //GPDMA_BlitTransfer(1, Framebuffer, 4, Framebuffer+1, 4, 2, 256*480*2);
}

void PPU_SNESVBlankEnd()
{
	PPU_SendCmd(0x40, 0);
}

void PPU_SNESHBlank()
{numio++;
	// TODO 239-line resolution for PAL mode (see SETINI 2133 bit2)
	//if (Mem_Status->VCount > 0)// && PPU_VCount <= 224)
	//	dsp_sendData(0, 0x4000 | Mem_Status->VCount);// | (CurFrame ? 0:0x100));
	//printf("sc%d/%d\n", Mem_Status->VCount, Mem_Status->ScreenHeight);
	//if (PPU_VCount != (lastvc+1))
	//	printf("AAA %d->%d\n", lastvc, PPU_VCount);
	//lastvc = PPU_VCount;
    if (Mem_Status->VCount < 224)
    {
        PPU_DrawScanline(Mem_Status->VCount);
        PPU_SendCmd(0xC0, 0);
    }
}

// remove me
void PPU_VBlank()
{
}

void PPU_HBlank()
{
}
