#include <wup/wup.h>

typedef struct sUartContext
{
    int Num;
    u32 Params;
    u32 ClockDiv1, ClockMult, ClockDiv2;

} sUartContext;

static sUartContext Context[3];

void UART_Reset(int num);


static void UART_IRQ(void* userdata)
{
    sUartContext* ctx = (sUartContext*)userdata;
    int num = ctx->Num;

    for (;;)
    {
        u32 irq = REG_UART_IRQ_STATUS(num) & 0xF;
        if (irq == UART_IRQST_NONE) return;

        if (irq == UART_IRQST_TX_READY)
        {
            // TODO signal flag
        }
        else if (irq == UART_IRQST_RX)
        {
            // TODO receive byte
            printf("UART%d RX!!\n", num);
        }
        else
        {
            // TODO reset UART
        }
    }
}


static void CalcClockParams(u32 clk, u32 baudrate, u32* mul, u32* div)
{
    baudrate <<= 4;
    u32 multiplier = baudrate;
    u32 divider = clk;

    for (;;)
    {
        multiplier = multiplier % divider;
        if (multiplier != 0)
        {
            // baudrate is not a multiple of the input clock

            divider = divider % multiplier;
            if (divider != 0) continue;     // input clock not a multiple of baudrate

            divider = clk / multiplier;
            multiplier = baudrate / multiplier;
            break;
        }
        else
        {
            // baudrate is a multiple of the input clock

            multiplier = baudrate / divider;
            divider = clk / divider;
            break;
        }
    }

    // adjust parameters to fit within 16-bit range
    while ((multiplier > 0xFFFF) || (divider > 0xFFFF))
    {
        multiplier >>= 1;
        divider >>= 1;
    }

    *mul = multiplier;
    *div = divider;
}

void UART_Init(int num, int baud, u32 params)
{
    if (num > 2) return;
    sUartContext* ctx = &Context[num];

    memset(ctx, 0, sizeof(sUartContext));
    ctx->Num = num;
    ctx->Params = params;
    ctx->ClockDiv1 = 1;
    CalcClockParams(16000000, baud, &ctx->ClockMult, &ctx->ClockDiv2);

    WUP_SetIRQHandler(IRQ_UART0 + num, UART_IRQ, ctx, 0);

    UART_Reset(num);
}

void UART_Reset(int num)
{
    if (num > 2) return;
    sUartContext* ctx = &Context[num];

    u32 resetbit = RESET_UART0 << num;
    REG_HARDWARE_RESET |= resetbit;
    REG_HARDWARE_RESET &= ~resetbit;

    REG_UART_CNT1(num) = 0;
    REG_UART_IRQ_ENABLE(num) = 0;

    // TODO invert line for UART2??
    REG_UART_CNT2(num) = UART_ENABLE;

    REG_UART_CLK_DIV1(num) = ctx->ClockDiv1;
    REG_UART_CLK_MULT(num) = ctx->ClockMult;
    REG_UART_CLK_DIV2(num) = ctx->ClockDiv2;

    // TODO this is set to 0xC0 for UART2
    REG_UART_UNK10(num) = 0;
    REG_UART_CNT1(num) = ctx->Params;
    REG_UART_UNK18(num) = 0;
    REG_UART_IRQ_ENABLE(num) = UART_IRQEN_TX | UART_IRQEN_RX | UART_IRQEN_RXERR;
}
