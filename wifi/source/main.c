#include "main.h"
#include "ppu.h"

extern u8 __bss_start__;
extern u8 __bss_end__;


void* memset(void* dst, int val, unsigned long len)
{
    for (unsigned long i = 0; i < len; i++)
        ((u8*)dst)[i] = val;

    return dst;
}

void* memcpy(void* dst, const void* src, unsigned long len)
{
    for (unsigned long i = 0; i < len; i++)
        ((u8*)dst)[i] = ((const u8*)src)[i];

    return dst;
}

void DelayUS(int us)
{
    u32 t1 = WREG_CPU_TICKCOUNT;
    us *= 90;
    for (;;)
    {
        u32 t2 = WREG_CPU_TICKCOUNT;
        if ((t2-t1) >= us)
            break;
    }
}

void Init()
{
    for (u8* bss = &__bss_start__; bss < &__bss_end__; bss++)
        *bss = 0;

    WREG_CPU_UNK1E0 = 0x21;

    WREG_CPU_CONFIGCNT |= 0x10;
    WREG_CPU_CONFIGCNT &= ~0x8;
    WREG_CPU_DEBUGCNT |= 0x1010000;
    WREG_DWT_MASK(0) = 7;
    WREG_DWT_COMP(0) = 0;
    WREG_DWT_FUNC(0) = 7;
    WREG_NVIC_IRQPRIO(IRQ_SDIO) = 32;

    WREG_BP_GPIOPULLUP = 0;
    WREG_BP_GPIOPULLDOWN = 0;

    WREG_BP_CORECNT |= 0x4;

    WREG_PMU_CNT |= 0x200;

    WREG_PMU_REG_CNT_ADDR = 2;
    WREG_PMU_REG_CNT_DATA = (WREG_PMU_REG_CNT_DATA & ~0x7) | 5;
    WREG_PMU_CHIP_CNT_ADDR = 0;
    WREG_PMU_CHIP_CNT_DATA = (WREG_PMU_CHIP_CNT_DATA & ~0x10000000);

    WREG_PMU_MIN_RES_MASK &= ~0x200000;
    WREG_PMU_MAX_RES_MASK &= ~0x200000;
    DelayUS(100);

    WREG_PMU_MIN_RES_MASK &= ~0x100000;
    WREG_PMU_MAX_RES_MASK &= ~0x100000;
    DelayUS(100);

    while (WREG_BP_CLKCNT & 0x20000)
    {
        DelayUS(10);
    }

    WREG_PMU_PLL_CNT_ADDR = 0;
    WREG_PMU_PLL_CNT_DATA = (WREG_PMU_PLL_CNT_DATA & ~0x1FF00000) | 0x01100000;

    WREG_PMU_PLL_CNT_ADDR = 2;
    WREG_PMU_PLL_CNT_DATA = (WREG_PMU_PLL_CNT_DATA & ~0x1FFE0000) | 0x03740000;

    WREG_PMU_PLL_CNT_ADDR = 3;
    WREG_PMU_PLL_CNT_DATA = (WREG_PMU_PLL_CNT_DATA & ~0x00FFFFFF) | 0x00627627;

    WREG_PMU_PLL_CNT_ADDR = 4;
    WREG_PMU_PLL_CNT_DATA = 0x202C2820;

    WREG_PMU_PLL_CNT_ADDR = 5;
    WREG_PMU_PLL_CNT_DATA = 0x22222215;

    WREG_PMU_CHIP_CNT_ADDR = 2;
    WREG_PMU_CHIP_CNT_DATA = (WREG_PMU_CHIP_CNT_DATA & ~0x180000);

    WREG_PMU_CNT |= 0x400;
    WREG_PMU_CNT = (WREG_PMU_CNT & 0xFF83) | 0xCB0030;

    const u16 updn_timer_init[6] = {0x05, 0x0301, 0x15, 0x0401, 0x0D, 0x1501};
    for (int i = 0; i < 3; i++)
    {
        WREG_PMU_RES_INDEX = updn_timer_init[i*2];
        WREG_PMU_RES_UPDN_TIMER = updn_timer_init[i*2+1];
    }

    const u32 depth_init[14] = {0x13, 0x4000, 0x12, 0x4000, 0x10, 0x4000, 0xF, 0x4000,
                                0x15, 0xD8000, 0x2, 0x20, 0x15, 0x20};
    for (int i = 0; i < 7; i++)
    {
        WREG_PMU_RES_INDEX = depth_init[i*2];
        WREG_PMU_RES_DEPTHMAX |= depth_init[i*2+1];
    }

    WREG_PMU_MAX_RES_MASK = 0x3FFFFF;
    WREG_PMU_MIN_RES_MASK = 0x12;

    DelayUS(2000);

    WREG_BP_GPIOTIMERVAL = 0x000A005A;

    WREG_RAM_BANK_INDEX = 2;
    WREG_RAM_STANDBY_CNT = 0x21017FFF;
    WREG_RAM_BANK_INDEX = 1;
    WREG_RAM_STANDBY_CNT = 0x21017FFF;
    WREG_RAM_BANK_INDEX = 0;
    WREG_RAM_STANDBY_CNT = 0x21017FFF;
    WREG_RAM_POWER_CNT &= ~0x10;

    WREG_CPU_UNK000 |= 0x01000000;
    WREG_CPU_UNK1E0 = 0;

    while (!(WREG_CPU_UNK1E0 & 0x20000))
    {
        DelayUS(10);
    }

    WREG_NVIC_IRQENABLE = IRQ_SDIO;
    WREG_CPU_IRQUNK = (IRQ_SDIO >> 1);

    WREG_CPU_UNK020 |= 3;

    WREG_PMU_WATCHDOG = 0;
    if (WREG_BP_IRQSTATUS & BP_IRQ_WDRESET)
        WREG_BP_IRQSTATUS = BP_IRQ_WDRESET;

    WREG_SDIO_INTRCVLAZY = (1<<24);
    WREG_SDIO_IRQENABLE_DEV = SDIO_IRQ_F2ENABLE | SDIO_IRQ_RESET | SDIO_IRQ_MBx_DEV;

    EnableIRQ();
}

void onException()
{
    // TODO?
    DisableIRQ();
    for (;;);
}

void main()
{
    Init();
    PPU_Init();

    for (;;)
    {
        WaitForIRQ();
    }
}
