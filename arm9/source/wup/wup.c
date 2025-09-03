#include <wup/wup.h>


typedef struct sIRQHandlerEntry
{
    fnIRQHandler Handler;
    void* UserData;
    int Priority;

} sIRQHandlerEntry;

sIRQHandlerEntry IRQTable[40];


void __libc_init_array();

static volatile u8 Timer0Flag;
static void Timer0IRQ(void* userdata);

static volatile u32 TickCount;
static void Timer1IRQ(void* userdata);

u32 Thread_PostIRQ();
void Thread_Tick();


void WUP_Init()
{
    if (WUP_HardwareType() == 0x41)
        ExceptionHandler(3, NULL);

    for (int i = 0; i < 40; i++)
    {
        IRQTable[i].Handler = NULL;
        IRQTable[i].UserData = NULL;
        IRQTable[i].Priority = 0;
    }

    for (int i = 0; i < 32; i++)
    {
        REG_IRQ_ENABLE(i) = 0x4F;
        REG_IRQ_TRIGGER(i) = 1;
    }
    *(vu32*)0xF00019D8 = 1;
    *(vu32*)0xF00019DC = 0;
    REG_IRQ_ACK_KEY = 0;
    REG_IRQ_ACK = 0xF;

    // IRQ trigger type?
    // for example for IRQ 15/16 it needs to be 1 for them to fire repeatedly
    const u8 irqtrig[40] = {
        1, 1, 5,                // 00..02
        5, 5, 5, 5, 5,          // 03..07
        1, 1, 1, 1, 1, 1, 1,    // 08..0E
        5, 5, 5, 5, 5, 5,       // 0F..14
        1, 1,                   // 15..16
        5, 5, 5, 5, 5, 5, 5,    // 17..1D
        1, 1,                   // 1E..1F
        1, 1, 1, 1, 1, 1, 1, 1  // 20..27
    };

    for (int i = 0; i < 40; i++)
        REG_IRQ_TRIGGER(i) = irqtrig[i];
    for (int i = 0; i < 32; i++)
        REG_IRQ_ENABLE(i) = 0x4A;
    for (int i = 32; i < 40; i++)
        REG_IRQ_ENABLE(i) = 0x60; // ???

    // GPIO setup
    REG_GPIO_UNK2C = 0;
    REG_GPIO_UNK38 = 0;
    REG_GPIO_AUDIO_WCLK = 0;
    REG_GPIO_AUDIO_BCLK = 0;
    REG_GPIO_AUDIO_MIC = 0;
    REG_GPIO_UNK48 = 0;
    REG_GPIO_CAM_DATA2 = 0;
    REG_GPIO_CAM_DATA3 = 0;
    REG_GPIO_CAM_DATA4 = 0;
    REG_GPIO_CAM_DATA5 = 0;
    REG_GPIO_CAM_DATA6 = 0;
    REG_GPIO_CAM_DATA7 = 0;
    REG_GPIO_CAM_DATA8 = 0;
    REG_GPIO_CAM_DATA9 = 0;
    REG_GPIO_CAM_HREF = 0;
    REG_GPIO_CAM_VSYNC = 0;
    REG_GPIO_CAM_XVCLK1 = 0;
    REG_GPIO_CAM_PCLK = 0;
    REG_GPIO_UNK80 = 0;
    REG_GPIO_UNK84 = 0;
    REG_GPIO_UNK88 = 0;
    REG_GPIO_UNK8C = 0;
    REG_GPIO_UNK90 = 0;
    REG_GPIO_UNK94 = 0;
    REG_GPIO_UNK98 = GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_I2C_SCL = 0;
    REG_GPIO_I2C_SDA = 0;
    REG_GPIO_SDIO_CLOCK = 0;
    REG_GPIO_SDIO_CMD = 0;
    REG_GPIO_SDIO_DAT0 = 0;
    REG_GPIO_SDIO_DAT1 = 0;
    REG_GPIO_SDIO_DAT2 = 0;
    REG_GPIO_SDIO_DAT3 = 0;
    REG_GPIO_UNKC4 = 0;
    REG_GPIO_UNKC8 = 0;
    REG_GPIO_UNKCC = 0;
    REG_GPIO_UNKD0 = 0;
    REG_GPIO_UART1_TX = 0;
    REG_GPIO_UART1_RX = 0;
    REG_GPIO_UART2_TX = 0;
    REG_GPIO_UART2_LED = 0;
    REG_GPIO_UART2_RX = 0;
    REG_GPIO_UART2_PWDN = 0;
    REG_GPIO_SPI_CLOCK = 0;
    REG_GPIO_SPI_MISO = 0;
    REG_GPIO_SPI_MOSI = 0;
    REG_GPIO_SPI_CS0 = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_HIGH;
    REG_GPIO_SPI_CS1 = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_HIGH;
    REG_GPIO_LCD_RESET = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_LOW;
    REG_GPIO_SPI_CS2 = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_HIGH;
    REG_GPIO_UNK108 = GPIO_SLEW_FAST;
    REG_GPIO_UNK10C = GPIO_SLEW_FAST_AND_UNK | GPIO_INPUT_MODE | GPIO_PULL_UP;
    REG_GPIO_UNK110 = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_MODE | GPIO_PULL_UP | GPIO_PULL_DOWN;
    REG_GPIO_RUMBLE = GPIO_SLEW_FAST;
    REG_GPIO_SENSOR_BAR = GPIO_SLEW_FAST;
    REG_GPIO_CAM_RESET = GPIO_SLEW_FAST;

    REG_CLK_UART0    = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(2);
    REG_CLK_UART1    = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(2);
    REG_CLK_UART2    = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(2);
    REG_CLK_SDIO     = CLK_SOURCE(CLKSRC_PLL_PRIM) | CLK_DIVIDER(18);   // 48 MHz
    REG_CLK_AUDIOAMP = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(2);
    REG_CLK_I2C      = CLK_SOURCE(CLKSRC_PLL_PRIM) | CLK_DIVIDER(216);  // 4 MHz
    REG_CLK_PIXELCLK = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(1);
    REG_CLK_CAMERA   = CLK_SOURCE(CLKSRC_PLL_PRIM) | CLK_DIVIDER(72);   // 12 MHz
    REG_CLK_UNK50    = CLK_SOURCE(CLKSRC_32MHZ)    | CLK_DIVIDER(2);

    // set the timers' base clock to tick every 108 cycles (ie. every microsecond)
    // for timers, we halve it so we get microsecond precision
    REG_TIMER_PRESCALER = 53;
    REG_COUNTUP_PRESCALER = 107;

    // reset hardware
    REG_HARDWARE_RESET |= 0x003FFFF8;
    REG_UNK64 = 6;
    REG_HARDWARE_RESET &= ~0x003FFFF8;
    REG_UNK30 |= 0x300;

    // more GPIO setup
    REG_GPIO_SPI_CLOCK = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_SPI_MISO = GPIO_ALT_FUNCTION;
    REG_GPIO_SPI_MOSI = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_SPI_CLOCK = SPI_CLK_48MHZ;

    REG_GPIO_SDIO_CLOCK = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_SDIO_CMD   = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_SDIO_DAT0  = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_SDIO_DAT1  = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_SDIO_DAT2  = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;
    REG_GPIO_SDIO_DAT3  = GPIO_ALT_FUNCTION | GPIO_SLEW_FAST_AND_UNK;

    REG_GPIO_UART1_TX = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_UART1_RX = GPIO_ALT_FUNCTION;

    REG_GPIO_I2C_SCL = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_I2C_SDA = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;

    REG_GPIO_UNK2C = GPIO_SLEW_FAST;

    REG_GPIO_CAM_DATA2 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA3 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA4 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA5 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA6 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA7 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA8 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_DATA9 = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_HREF = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_VSYNC = GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_XVCLK1 = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_CAM_PCLK = GPIO_ALT_FUNCTION;

    REG_GPIO_UART2_TX = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_UART2_LED = GPIO_SLEW_FAST | GPIO_ALT_FUNCTION;
    REG_GPIO_UART2_RX = GPIO_ALT_FUNCTION;
    REG_GPIO_UART2_PWDN = GPIO_ALT_FUNCTION;

    REG_GPIO_UNK48 = GPIO_SLEW_FAST;
    REG_GPIO_UNK38 = GPIO_SLEW_FAST;
    REG_GPIO_AUDIO_WCLK = GPIO_ALT_FUNCTION;
    REG_GPIO_AUDIO_BCLK = GPIO_ALT_FUNCTION;
    REG_GPIO_AUDIO_MIC = GPIO_ALT_FUNCTION;

    // configure timer 1 with a 1ms interval
    REG_TIMER_CNT(1) = 0;
    WUP_SetIRQHandler(IRQ_TIMER1, Timer1IRQ, NULL, 0);
    TickCount = 0;

    REG_TIMER_TARGET(1) = 124;
    REG_TIMER_CNT(1) = TIMER_DIV_16 | TIMER_ENABLE;

    // reset count-up timer
    REG_COUNTUP_VALUE = 0;

    Thread_Init();
    __libc_init_array();
    EnableIRQ();

    REG_TIMER_CNT(0) = 0;
    WUP_SetIRQHandler(IRQ_TIMER0, Timer0IRQ, NULL, 0);
    Timer0Flag = 0;

    DMA_Init();
    SPI_Init();
    I2C_Init();
    UART_Init(0, 115200, UART_DATA_8BIT | UART_STOP_1BIT | UART_PARITY_NONE);

    Flash_Init();
    UIC_Init();

    Video_Init();
    LCD_Init();

    AudioAmp_Init();
    Audio_Init();

    Input_Init();

    // setup rumble GPIO (checkme)
    REG_GPIO_RUMBLE = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_LOW;

    UIC_WaitWifiReady();
    SDIO_Init();
    Wifi_Init();
}

void WUP_DeInit()
{
    REG_GPIO_RUMBLE = GPIO_SLEW_FAST_AND_UNK | GPIO_OUTPUT_LOW;

    Wifi_DeInit();

    Input_DeInit();

    Audio_DeInit();
    AudioAmp_DeInit();

    LCD_DeInit();

    WUP_DelayUS(60);
}


void WUP_SetIRQHandler(u8 irq, fnIRQHandler handler, void* userdata, int prio)
{
    if (irq >= 40) return;

    int irqen = DisableIRQ();

    IRQTable[irq].Handler = handler;
    IRQTable[irq].UserData = userdata;
    IRQTable[irq].Priority = prio;

    if (handler)
        WUP_EnableIRQ(irq);
    else
        WUP_DisableIRQ(irq);

    RestoreIRQ(irqen);
}

void WUP_EnableIRQ(u8 irq)
{
    if (irq >= 40) return;

    int irqen = DisableIRQ();

    int prio = IRQTable[irq].Priority & 0xF;
    REG_IRQ_ENABLE(irq) = (REG_IRQ_ENABLE(irq) & ~0x4F) | prio;

    RestoreIRQ(irqen);
}

void WUP_DisableIRQ(u8 irq)
{
    if (irq >= 40) return;

    int irqen = DisableIRQ();

    REG_IRQ_ENABLE(irq) |= 0x40;

    RestoreIRQ(irqen);
}

u32 IRQHandler()
{
    u32 irqnum = REG_IRQ_CURRENT;
    u32 ack = REG_IRQ_ACK_KEY;

    if (irqnum < 40)
    {
        sIRQHandlerEntry* entry = &IRQTable[irqnum];
        if (entry->Handler)
            entry->Handler(entry->UserData);
    }

    REG_IRQ_ACK = ack;

    return Thread_PostIRQ();
}


static void Timer0IRQ(void* userdata)
{
    REG_TIMER_CNT(0) = 0;
    Timer0Flag = 1;
}

static void Timer1IRQ(void* userdata)
{
    TickCount++;
    Thread_Tick();
}

void WUP_DelayUS(int us)
{
    if (REG_TIMER_CNT(0)) printf("!! WUP TIMER ALREADY ENABLED 1\n");
    REG_TIMER_CNT(0) = 0;
    Timer0Flag = 0;
    REG_TIMER_VALUE(0) = 0;
    REG_TIMER_TARGET(0) = us - 1;
    REG_TIMER_CNT(0) = TIMER_ENABLE;

    while (!Timer0Flag)
        WaitForIRQ();

    REG_TIMER_CNT(0) = 0;
}

void WUP_DelayMS(int ms)
{
    if (REG_TIMER_CNT(0)) printf("!! WUP TIMER ALREADY ENABLED 2\n");
    REG_TIMER_CNT(0) = 0;
    Timer0Flag = 0;
    REG_TIMER_VALUE(0) = 0;
    REG_TIMER_TARGET(0) = (ms * 1000) - 1;
    REG_TIMER_CNT(0) = TIMER_ENABLE;

    while (!Timer0Flag)
        WaitForIRQ();

    REG_TIMER_CNT(0) = 0;
}

u32 WUP_GetTicks()
{
    return TickCount;
}
