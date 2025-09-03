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

#include "cpu.h"
#include "memory.h"
#include "ppu.h"
#include "spc700.h"
#include "dsp.h"
#include "mixrate.h"


u16* Framebuffer;
s16* Audiobuffer;



// FPGA debug output

void send_binary(u8* data, int len)
{
#ifdef FPGA_LOG
    len &= 0x3FFF;
    u16 header = len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, 0x8400);
    SPI_Write(buf, 3);
    SPI_Write(data, len);
    SPI_Finish();
#endif
}

void send_string(char* str)
{
#ifdef FPGA_LOG
    int len = strlen(str);

    len &= 0x3FFF;
    u16 header = 0x8000 | len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, 0x8400);
    SPI_Write(buf, 3);
    SPI_Write((u8*)str, len);
    SPI_Finish();
#endif
}

void dump_data(u8* data, int len)
{
#ifdef FPGA_LOG
    len &= 0x3FFF;
    u16 header = 0x4000 | len;

    u8 buf[3];
    buf[0] = 0xF2;
    buf[1] = header >> 8;
    buf[2] = header & 0xFF;

    SPI_Start(SPI_DEVICE_FLASH, 0x8400);
    SPI_Write(buf, 3);
    SPI_Write(data, len);
    SPI_Finish();
#endif
}

extern s16 DSP_Buffer[MIXBUFSIZE<<2];
u32 streampos = 0;

void StreamCB(void* buffer, int length)
{
    memcpy(buffer, &DSP_Buffer[streampos], length);
    streampos += (length >> 1);
    streampos &= ((MIXBUFSIZE<<2)-1);
}


int main(void)
{
	printf("wupSNES\n");

    //int fblen = 256 * 240 * sizeof(u16);
    int fblen = 512 * 480 * sizeof(u16);
    Framebuffer = (u16*)memalign(16, fblen);
    memset(Framebuffer, 0, fblen);
    //Video_SetOvlFramebuffer(Framebuffer, (854-256)/2, (480-240)/2, 256, 240, 256*2, PIXEL_FMT_ARGB1555);
    Video_SetOvlFramebuffer(Framebuffer, (854-512)/2, 0, 512, 480, 512*2, PIXEL_FMT_ARGB1555);
    Video_SetOvlEnable(1);

    Video_SetDisplayEnable(1);

    int ablen = 1024 * 4;
    Audiobuffer = (s16*)memalign(16, ablen);
    memset(Audiobuffer, 0, ablen);
    Audio_StartStream(Audiobuffer, ablen, AUDIO_FORMAT_PCM16, AUDIO_FREQ_48KHZ, 2, StreamCB);

    PPU_Init();

    if (!Mem_LoadROM("ROM_"))
    {
        printf("failed to load ROM\n");
        return 0;
    }

    CPU_Reset();
    SPC_Reset();

	for (;;)
	{
        Input_Scan();

        u32 t1 = REG_COUNTUP_VALUE;
        CPU_MainLoop();
        u32 t2 = REG_COUNTUP_VALUE;
        extern int mixcnt;
        printf("CPU time = %d - mix = %d\n", t2-t1, mixcnt);
        mixcnt = 0;
		Video_WaitForVBlank();
	}
	return 0;
}

void printstuff(u32 foo, u32 bar, u32 blarg)
{
	//iprintf("printstuff %08X %08X %08X\n", foo, bar, blarg);
}
