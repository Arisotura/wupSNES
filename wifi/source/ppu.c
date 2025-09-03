#include "main.h"
#include "sdio.h"
#include "ppu.h"


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

    u16 CGRAM[256];
    u8 VRAM[0x10000];
    u8 OAM[0x220];

} PPU;

#define kCmdBufLength 1024
u8 CmdBuf[kCmdBufLength*2];

u8 ScanlineBuf[256*64*2];
u8 ScanlineBufPos;
u8 ScanlineBufStart;
u8 CurScanlineBuf;

const void* PtrList[3] = {&CmdBuf, &ScanlineBuf, &PPU.VRAM};

u16 Planar2Linear2[256];
u32 Planar2Linear4[256];
u64 Planar2Linear8[256];


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
        if (i & 0x0080) o |= 0x00000001;
        if (i & 0x0040) o |= 0x00000010;
        if (i & 0x0020) o |= 0x00000100;
        if (i & 0x0010) o |= 0x00001000;
        if (i & 0x0008) o |= 0x00010000;
        if (i & 0x0004) o |= 0x00100000;
        if (i & 0x0002) o |= 0x01000000;
        if (i & 0x0001) o |= 0x10000000;
        Planar2Linear4[i] = o;
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

    SDIO_SendMail(3, (u32)&PtrList[0]);
}


void PPU_Reset()
{
    memset(&PPU, 0, sizeof(PPU));

    memset(ScanlineBuf, 0, sizeof(ScanlineBuf));
    ScanlineBufPos = 0;
    ScanlineBufStart = 0;
    CurScanlineBuf = 0;
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

void PPU_Write8(u32 addr, u8 val)
{
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
                PPU.CGRAM[PPU.CGRAMAddr & 0xFF] = PPU.CGRAMVal | (val << 8);
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
    }
}

#if 0
void PPU_DrawBG_2bpp_8x8(u16 num, u8* dst, u16 ypos)
{
    u16* vram = (u16*)PPU.VRAM;
    u16 tileset = PPU.BGChr[num];// << 12;

    ypos += PPU.BGYPos[num];

    u16 bgscr = PPU.BGScr[num];
    u16 tilemap = ((bgscr & 0xFC) << 8) + ((ypos & 0xF8) << 2);
    if ((ypos & 0x100) && (bgscr & 0x2))
        tilemap += (bgscr & 0x1) ? 2048 : 1024;

    u16 xpos = PPU.BGXPos[num];
    /*u16 tmaddr = (xpos & 0xF8) >> 3;
    if ((xpos & 0x100) && (bgscr & 0x1))
        tmaddr += 1024;*/

    int xoff = xpos & 0x7;
    dst -= xoff;

    int w = xoff ? 33 : 32;
    for (int i = 0; i < w; i++)
    {
        u16 tmaddr = (xpos & 0xF8) >> 3;
        if ((xpos & 0x100) && (bgscr & 0x1))
            tmaddr += 1024;
        xpos += 8;

        u32 mask = 0xFFFFFFFF;
        if (i == 0) mask <<= (xoff * 4);
        else if (i == 32) mask >>= (xoff * 4);

        // load tile
        u16 curtile = vram[tilemap + tmaddr];
        u8 pal = (curtile & 0x1C00) >> 8;

        u16 tsaddr = (curtile & 0x03FF) << 3;
        if (curtile & 0x8000) tsaddr += (7 - (ypos & 0x7));
        else                  tsaddr += (ypos & 0x7);

        u16 tileraw1 = vram[tileset + tsaddr];
        u16 tiledata = Planar2Linear2[tileraw1 & 0xFF] |
                       (Planar2Linear2[tileraw1 >> 8] << 1);

        tiledata &= mask;
        /*if (!tiledata)
        {
            dst += 8;
            continue;
        }*/

        u32 c;
        c = tiledata & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 2) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 4) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 6) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 8) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 10) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 12) & 0x3;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 14) & 0x3;
        if (c) *dst = pal | c;
        dst++;
    }
}

void PPU_DrawBG_4bpp_8x8(u16 num, u8* dst, u16 ypos)
{
    u16* vram = (u16*)PPU.VRAM;
    u16 tileset = PPU.BGChr[num];// << 12;

    ypos += PPU.BGYPos[num];

    u16 bgscr = PPU.BGScr[num];
    u16 tilemap = ((bgscr & 0xFC) << 8) + ((ypos & 0xF8) << 2);
    if ((ypos & 0x100) && (bgscr & 0x2))
        tilemap += (bgscr & 0x1) ? 2048 : 1024;

    u16 xpos = PPU.BGXPos[num];
    /*u16 tmaddr = (xpos & 0xF8) >> 3;
    if ((xpos & 0x100) && (bgscr & 0x1))
        tmaddr += 1024;*/

    int xoff = xpos & 0x7;
    dst -= xoff;

    int w = xoff ? 33 : 32;
    for (int i = 0; i < w; i++)
    {
        u16 tmaddr = (xpos & 0xF8) >> 3;
        if ((xpos & 0x100) && (bgscr & 0x1))
            tmaddr += 1024;
        xpos += 8;

        u32 mask = 0xFFFFFFFF;
        if (i == 0) mask <<= (xoff * 4);
        else if (i == 32) mask >>= (xoff * 4);
        /*u64 mask = 0xFFFFFFFFFFFFFFFF;
        if (i == 0) mask <<= (xoff * 8);
        else if (i == 32) mask >>= (xoff * 8);*/

        // load tile
        u16 curtile = vram[tilemap + tmaddr];
        u8 pal = (curtile & 0x1C00) >> 6;

        u16 tsaddr = (curtile & 0x03FF) << 4;
        if (curtile & 0x8000) tsaddr += (7 - (ypos & 0x7));
        else                  tsaddr += (ypos & 0x7);

        u16 tileraw1 = vram[tileset + tsaddr];
        u16 tileraw2 = vram[tileset + tsaddr + 8];
        u32 tiledata = Planar2Linear4[tileraw1 & 0xFF] |
                      (Planar2Linear4[tileraw1 >> 8] << 1) |
                      (Planar2Linear4[tileraw2 & 0xFF] << 2) |
                      (Planar2Linear4[tileraw2 >> 8] << 3);

        tiledata &= mask;
        /*if (!tiledata)
        {
            dst += 8;
            continue;
        }*/

        u32 c;
        c = tiledata & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 4) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 8) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 12) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 16) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 20) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 24) & 0xF;
        if (c) *dst = pal | c;
        dst++;
        c = (tiledata >> 28) & 0xF;
        if (c) *dst = pal | c;
        dst++;

        /*u64 tiledata = Planar2Linear8[tileraw1 & 0xFF] |
                       (Planar2Linear8[tileraw1 >> 8] << 1) |
                       (Planar2Linear8[tileraw2 & 0xFF] << 2) |
                       (Planar2Linear8[tileraw2 >> 8] << 3);

        tiledata |= (pal * 0x0101010101010101);
        tiledata &= mask;

        u32 c;
        c = tiledata & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 8) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 16) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 24) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 32) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 40) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 48) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;
        c = (tiledata >> 56) & 0xFF;
        if (c & 0xF) *dst = c;
        dst++;*/
    }
}
#else

void PPU_DrawBG_2bpp_8x8(u16 num, u16* dst, u16 ypos)
{
    u16* vram = (u16*)PPU.VRAM;
    u16 tileset = PPU.BGChr[num];// << 12;

    ypos += PPU.BGYPos[num];

    u16 bgscr = PPU.BGScr[num];
    u16 tilemap = ((bgscr & 0xFC) << 8) + ((ypos & 0xF8) << 2);
    if ((ypos & 0x100) && (bgscr & 0x2))
        tilemap += (bgscr & 0x1) ? 2048 : 1024;

    u16 xpos = PPU.BGXPos[num];
    /*u16 tmaddr = (xpos & 0xF8) >> 3;
    if ((xpos & 0x100) && (bgscr & 0x1))
        tmaddr += 1024;*/

    int xoff = xpos & 0x7;
    dst -= xoff;

    int w = xoff ? 33 : 32;
    for (int i = 0; i < w; i++)
    {
        u16 tmaddr = (xpos & 0xF8) >> 3;
        if ((xpos & 0x100) && (bgscr & 0x1))
            tmaddr += 1024;
        xpos += 8;

        u32 mask = 0xFFFFFFFF;
        if (i == 0) mask <<= (xoff * 4);
        else if (i == 32) mask >>= (xoff * 4);

        // load tile
        u16 curtile = vram[tilemap + tmaddr];
        u16* pal = &PPU.CGRAM[(curtile & 0x1C00) >> 8];

        u16 tsaddr = (curtile & 0x03FF) << 3;
        if (curtile & 0x8000) tsaddr += (7 - (ypos & 0x7));
        else                  tsaddr += (ypos & 0x7);

        u16 tileraw1 = vram[tileset + tsaddr];
        u16 tiledata = Planar2Linear2[tileraw1 & 0xFF] |
                       (Planar2Linear2[tileraw1 >> 8] << 1);

        tiledata &= mask;
        /*if (!tiledata)
        {
            dst += 8;
            continue;
        }*/

        u32 c;
        c = tiledata & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 2) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 4) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 6) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 8) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 10) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 12) & 0x3;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 14) & 0x3;
        if (c) *dst = pal[c];
        dst++;
    }
}

void PPU_DrawBG_4bpp_8x8(u16 num, u16* dst, u16 ypos)
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

        u32 mask = 0xFFFFFFFF;
        if (i == 0) mask <<= (xoff * 4);
        else if (i == 32) mask >>= (xoff * 4);

        //u32 t2 = WREG_CPU_TICKCOUNT;
        // load tile
        //u16 curtile = vram[tilemap + tmaddr];
        u16 curtile = *tilemap++;
        u16* pal = &PPU.CGRAM[(curtile & 0x1C00) >> 6];
        //u32 t2 = WREG_CPU_TICKCOUNT;
        // expensive part?
        u16 tsaddr = (curtile & 0x03FF) << 4;
        if (curtile & 0x8000) tsaddr += (7 - (ypos & 0x7));
        else                  tsaddr += (ypos & 0x7);

        //u32 t3 = WREG_CPU_TICKCOUNT;
        //u16 tileraw1 = vram[tileset + tsaddr];
        //u16 tileraw2 = vram[tileset + tsaddr + 8];
        register u32* p2l = Planar2Linear4;
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
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 4) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 8) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 12) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 16) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 20) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 24) & 0xF;
        if (c) *dst = pal[c];
        dst++;
        c = (tiledata >> 28) & 0xF;
        if (c) *dst = pal[c];
        dst++;

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
                    tilemap -= (1024+32+32);
            }
            else
            {
                tilemap -= 32;
            }
        }
    }
}

#endif

int z = 0;
int y = 0;

int PPU_DrawScanline(int line)
{
    if (line < 1)
    {
        // blank
        // TODO
        // also should still do sprites for next line
        //return;
    }

    // TODO

    int pos = ScanlineBufPos;
    u8* buf_main = &ScanlineBuf[pos * 512];
    u8* buf_sub  = &ScanlineBuf[pos * 512 + 256];
    //u8 temp[256];

    //if (!(line & 1))
    {
        /*for (int i = 0; i < 256; i++)
        {
            buf_main[i] = 0;
            buf_sub[i] = 0;
            //temp[i] = 0;
        }*/
        if (line == 0) *(vu32 *) 0x40000 = 0;

        // TEST
        //u32 t1 = WREG_CPU_TICKCOUNT;

        /*u32 backdrop = PPU.CGRAM[0];
        backdrop |= (backdrop << 16);
        for (int i = 0; i < 256; i+=2)
        {
            *(u32*)&buf_main[i*2] = backdrop;
        }*/
        u16 backdrop = PPU.CGRAM[0];
        for (int i = 0; i < 256; i++)
        {
            *(u16*)&buf_main[i*2] = backdrop;
        }

        u32 t1 = WREG_CPU_TICKCOUNT;

        PPU_DrawBG_4bpp_8x8(0, (u16*)buf_main, line);
        //PPU_DrawBG_2bpp_8x8(2, (u16*)buf_main, line);
        /*PPU_DrawBG_4bpp_8x8(0, temp, line);
        PPU_DrawBG_2bpp_8x8(2, temp, line);
        for (int i = 0; i < 256; i++)
        {
            ((u16*)buf_main)[i] = PPU.CGRAM[temp[i]];
        }*/
        u32 t2 = WREG_CPU_TICKCOUNT;
        *(vu32 *) 0x40000 += t2 - t1;
    }

    int ret = -1;
    ScanlineBufPos++;
    if ((ScanlineBufPos >= 64) || (line == 223))
    {
        //SDIO_SendMail(1, ScanlineBufStart);
        ret = ScanlineBufStart;
        ScanlineBufPos = 0;
        ScanlineBufStart = (line == 223) ? 0 : (line + 1);
    }

    return ret;
}

void PPU_VBlank(void)
{
    PPU.OAMAddr = PPU.OAMReload;
    z++;
}


void PPU_RunCommands(int index)
{
    index &= 1;
    u8* cmdbuf = &CmdBuf[index * kCmdBufLength];
    u8* cmdend = &cmdbuf[kCmdBufLength];
    u32 resp = 0x10000;

    while (cmdbuf < cmdend)
    {
        u8 cmd = *cmdbuf++;
        if (cmd == 0xFF) break;

        u8 cmdtype = cmd >> 6;
        if (cmdtype == 0)
        {
            // PPU reg write
            u8 val = *cmdbuf++;
            PPU_Write8(cmd, val);
        }
        else if (cmdtype == 1)
        {
            // start new frame
            PPU.VCount = 0;
        }
        else if (cmdtype == 2)
        {
            // VBlank
            PPU_VBlank();
        }
        else
        {
            // draw scanline
            int ret = PPU_DrawScanline(PPU.VCount);
            if (ret >= 0)
                resp = 0x20000 | ret;

            PPU.VCount++;
        }
    }

    SDIO_SendMail(0, resp);
}
