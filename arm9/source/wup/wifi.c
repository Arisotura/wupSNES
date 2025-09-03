#include <wup/wup.h>


#define READ16LE(buf, off)  ((buf)[(off)] | ((buf)[(off)+1] << 8))
#define READ32LE(buf, off)  ((buf)[(off)] | ((buf)[(off)+1] << 8) | ((buf)[(off)+2] << 16) | ((buf)[(off)+3] << 24))

#define READ16BE(buf, off)  ((buf)[(off)+1] | ((buf)[(off)] << 8))
#define READ32BE(buf, off)  ((buf)[(off)+3] | ((buf)[(off)+2] << 8) | ((buf)[(off)+1] << 16) | ((buf)[(off)] << 24))



static void* RxThread;
static void* RxEventMask;


static u8 ClkEnable;
static u8 State;
static u32 LastActiveTime;

static u8 MACAddr[6];

static volatile u8 CardIRQFlag;

static u32 IRQMask;



void Wifi_ResetRxFlags(u8 flags);
void Wifi_WaitForRx(u8 flags);
void Wifi_CheckRx();



static void RxThreadFunc(void* userdata);
static void WifiThreadFunc(void* userdata);



int Wifi_Init()
{
    CardIRQFlag = 0;
    //State = State_Idle;
    //LastActiveTime = WUP_GetTicks();

    memset(MACAddr, 0, sizeof(MACAddr));

    /*ScanCB = NULL;
    ScanList = NULL;
    ScanNum = 0;

    JoinOpen = 0;
    JoinCB = NULL;
    JoinStartTimestamp = 0;

    LinkStatus = 0;*/

    //TxSemaphore = Semaphore_Create(1);
    //if (!TxSemaphore) return 0;

    /*RxCtlMailbox = Mailbox_Create(8);
    if (!RxCtlMailbox) return 0;
    RxEventMailbox = Mailbox_Create(32);
    if (!RxEventMailbox) return 0;*/

    RxEventMask = EventMask_Create();
    if (!RxEventMask) return 0;

    //WifiStop = 0;

    RxThread = Thread_Create(RxThreadFunc, NULL, 0x1000, 0, "wifi_rx");
    if (!RxThread) return 0;

    //WifiThread = Thread_Create(WifiThreadFunc, NULL, 0x1000, 4, "wifi");
    //if (!WifiThread) return 0;

    u32 regval;

    regval = 0;
    SDIO_ReadCardRegs(1, 0x10009, &regval, 1);
    if (regval & (1<<3))
    {
        // clear I/O isolation bit
        regval &= ~(1<<3);
        SDIO_WriteCardRegs(1, 0x10009, &regval, 1);
    }

    SDIO_SetClocks(1, SDIO_CLOCK_FORCE_HWREQ_OFF | SDIO_CLOCK_REQ_ALP);
    SDIO_SetClocks(1, SDIO_CLOCK_FORCE_HWREQ_OFF | SDIO_CLOCK_FORCE_ALP);

    regval = 0;
    SDIO_WriteCardRegs(1, 0x1000F, &regval, 1);

    if (!Wifi_AI_Enumerate())
        return 0;

    Wifi_AI_SetCore(WIFI_CORE_ARMCM3);
    Wifi_AI_DisableCore(0);

    if (Wifi_AI_GetCoreRevision() >= 20)
    {
        Wifi_AI_SetCore(WIFI_CORE_BACKPLANE);

        // GPIO pullup/pulldown
        Wifi_AI_WriteCoreMem(0x058, 0);
        Wifi_AI_WriteCoreMem(0x05C, 0);

        // only if revision >= 21
        // ??
        regval = Wifi_AI_ReadCoreMem(0x008);
        Wifi_AI_WriteCoreMem(0x008, regval | 0x4);

    }

    if (!Wifi_GetRAMSize())
        return 0;

    // reset backplane upon SDIO reset
    Wifi_AI_SetCore(WIFI_CORE_SDIOD);
    regval = Wifi_AI_ReadCoreMem(0x000);
    Wifi_AI_WriteCoreMem(0x000, regval | (1<<1));

    // enable F1
    regval = (1<<1);
    if (!SDIO_WriteCardRegs(0, 0x2, &regval, 1))
        return 0;

    SDIO_SetClocks(1, 0);

    /*Wifi_AI_SetCore(0x800);
    {
        u32 val1 = Wifi_AI_ReadCoreMem(0x0);
        u32 val2 = Wifi_AI_ReadCoreMem(0x2C);
        u32 val3 = Wifi_AI_ReadCoreMem(0x10);
        printf("derp = %08X:%08X:%08X\n", val1, val2, val3);
    }
    {
        u32 val1 = Wifi_AI_ReadCoreMem(0x600);
        u32 val2 = Wifi_AI_ReadCoreMem(0x604);
        u32 val3 = Wifi_AI_ReadCoreMem(0x608);
        u32 val4 = Wifi_AI_ReadCoreMem(0x60C);
        printf("shart = %08X:%08X:%08X:%08X\n", val1, val2, val3, val4);
    }
    u8* otp = malloc(0x300);
    for (int i = 0; i < 0x300; i+=4)
    {
        *(u32*)&otp[i] = Wifi_AI_ReadCoreMem(0x800+i);
    }
    void dump_data(u8* data, int len);
    dump_data(otp, 0x300);*/

    if (!Wifi_UploadFirmware())
        return 0;
printf("firmware uploaded\n");

    /*u8* boobs = malloc(0x2000);

    void dump_data(u8*, int);
    for (int i = 0; i < 0x48000; i+=0x2000)
    {
        SDIO_ReadF1Memory(i, boobs, 0x2000);
        dump_data(boobs, 0x2000);
    }*/

    SDIO_SetClocks(1, SDIO_CLOCK_REQ_HT);printf("ii\n");
    SDIO_SetClocks(1, SDIO_CLOCK_FORCE_HT);
printf("AA\n");
    Wifi_AI_SetCore(WIFI_CORE_SDIOD);
printf("BB\n");
    // enable frame transfers
    Wifi_AI_WriteCoreMem(0x048, (4<<16));
printf("CC\n");
    // enable F2
    u8 fn = (1<<1) | (1<<2);
    regval = fn;
    SDIO_WriteCardRegs(0, 0x2, &regval, 1);
    printf("prôn\n");

    /*for (;;)
    {
        u32 derp = 0;
        SDIO_ReadF1Memory(0x0, &derp, 4);
        printf("derp=%08X\n", derp);
        Thread_Sleep(1000);
    }*/

    // wait for it to be ready
    // TODO have a timeout here?
    /*for (;;)
    {
        regval = 0;
        SDIO_ReadCardRegs(0, 0x3, &regval, 1);
        if (regval == fn) break;
        WUP_DelayUS(1);
    }
printf("F2 came up\n");*/
    // enable interrupts
    IRQMask = 0x200000F0;
    Wifi_AI_WriteCoreMem(0x024, IRQMask);

    // set watermark
    regval = 8;
    SDIO_WriteCardRegs(1, 0x10008, &regval, 1);


    SDIO_SetClocks(1, SDIO_CLOCK_REQ_HT);

    /*TxSeqno = 0xFF;
    TxMax = 0;
    TxCtlId = 1;*/
    SDIO_EnableCardIRQ();


    EventMask_Wait(RxEventMask, 4, NoTimeout, NULL);
    EventMask_Clear(RxEventMask, 4);
    printf("Wifi: ready to go\n");
    return 1;
}



void Wifi_DeInit()
{
    SDIO_DisableCardIRQ();

    /*WifiStop = 1;
    Thread_Wait(WifiThread, NoTimeout);
    Thread_Delete(WifiThread);
    Mailbox_Delete(RxCtlMailbox);
    Mailbox_Delete(RxEventMailbox);

    EventMask_Signal(RxEventMask, 2);
    Thread_Wait(RxThread, NoTimeout);
    Thread_Delete(RxThread);
    EventMask_Delete(RxEventMask);*/

    u8 regval;

    SDIO_SetClocks(1, 0);

    regval = 0xF;
    SDIO_WriteCardRegs(1, 0x1000F, &regval, 1);

    // set the I/O isolation bit if needed
    regval = 0;
    SDIO_ReadCardRegs(1, 0x10009, &regval, 1);
    if (!(regval & (1<<3)))
    {
        regval |= (1<<3);
        SDIO_WriteCardRegs(1, 0x10009, &regval, 1);
    }

    // reset the wifi card
    regval = 0x8;
    SDIO_WriteCardRegs(0, 0x6, &regval, 1);

    REG_SD_POWERCNT = 0;
}


void Wifi_GetMACAddr(u8* addr)
{
    memcpy(addr, MACAddr, 6);
}


void Wifi_SetClkEnable(int enable)
{
    if (enable == ClkEnable)
        return;

    if (enable)
        SDIO_SetClocks(1, SDIO_CLOCK_REQ_HT);
    else
        SDIO_SetClocks(0, 0);

    ClkEnable = enable;
}


void Wifi_SendMail(int mb, u32 val)
{
    Wifi_AI_WriteCoreMem(0x048, val);
    Wifi_AI_WriteCoreMem(0x040, 1 << mb);
}

void Wifi_WaitCmdList()
{
    EventMask_Wait(RxEventMask, 8, NoTimeout, NULL);
    EventMask_Clear(RxEventMask, 8);
}


void Wifi_CardIRQ()
{
    SDIO_DisableCardIRQ();
    EventMask_Signal(RxEventMask, 1);
}

static void RxThreadFunc(void* userdata)
{
    for (;;)
    {
        u32 event;
        EventMask_Wait(RxEventMask, 3, NoTimeout, &event);
        if (event & 2) return;
        EventMask_Clear(RxEventMask, 1);

        SDIO_Lock();
        if (!ClkEnable)
            Wifi_SetClkEnable(1);

        u32 irqstatus = Wifi_AI_ReadCoreMem(0x020) & IRQMask;
        Wifi_AI_WriteCoreMem(0x020, irqstatus); // ack

        if (irqstatus & (1<<4))
        {
            u32 resp = Wifi_AI_ReadCoreMem(0x04C);

            EventMask_Signal(RxEventMask, 8);
            if ((resp >> 16) == 2)
            {
                void PPU_Present(int line);
                PPU_Present(resp & 0xFFFF);
            }
        }

        /*if (irqstatus & (1<<5))
        {
            u32 line = Wifi_AI_ReadCoreMem(0x04C);
            void PPU_Present(int line);
            PPU_Present(line);
        }*/

        if (irqstatus & (1<<7))
        {
            u32 ptr = Wifi_AI_ReadCoreMem(0x04C);
            extern u32 PPU_RemotePtrs[3];
            SDIO_ReadF1Memory(ptr, &PPU_RemotePtrs, 12);
            printf("received ptrs: %08X %08X %08X\n", PPU_RemotePtrs[0], PPU_RemotePtrs[1], PPU_RemotePtrs[2]);
            EventMask_Signal(RxEventMask, 4);
        }

        SDIO_EnableCardIRQ();
        SDIO_Unlock();
    }
}

