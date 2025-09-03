#include "main.h"
#include "sdio.h"
#include "ppu.h"


void SDIO_SendMail(int mb, u32 val)
{
    WREG_SDIO_MBDATA_HOST = val;
    WREG_SDIO_MAILBOX_HOST = (1 << mb);
}


void SDIO_IRQ()
{
    u32 irqstatus = WREG_SDIO_IRQSTATUS & WREG_SDIO_IRQENABLE_DEV;
    WREG_SDIO_IRQSTATUS = irqstatus;

    if (irqstatus & SDIO_IRQ_F2ENABLE)
    {
        // TODO: set up DMA, if we were to use this

        // signal F2 ready or not
        if (WREG_SDIO_STATUS & (1<<2))
            WREG_SDIO_CNT |= (1<<2);
        else
            WREG_SDIO_CNT &= ~(1<<2);
    }

    if (irqstatus & SDIO_IRQ_MB0_DEV)
    {
        u32 val = WREG_SDIO_MBDATA_DEV;
        //DisableIRQ();
        PPU_RunCommands(val);
        //EnableIRQ();
    }

    if (irqstatus & SDIO_IRQ_MB1_DEV)
    {
        PPU_Reset();
    }
}
