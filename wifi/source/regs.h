#ifndef _REGS_H_
#define _REGS_H_

// backplane registers

#define WREG_BP_CHIPID              *(vu32*)0x18000000
#define WREG_BP_CAPS                *(vu32*)0x18000004
#define WREG_BP_CORECNT             *(vu32*)0x18000008
#define WREG_BP_IRQSTATUS           *(vu32*)0x18000020
#define WREG_BP_IRQENABLE           *(vu32*)0x18000024

#define WREG_BP_GPIOPULLUP          *(vu32*)0x18000058
#define WREG_BP_GPIOPULLDOWN        *(vu32*)0x1800005C
#define WREG_BP_GPIOIN              *(vu32*)0x18000060
#define WREG_BP_GPIOOUT             *(vu32*)0x18000064
#define WREG_BP_GPIOOUTENABLE       *(vu32*)0x18000068
#define WREG_BP_GPIOCNT             *(vu32*)0x1800006C
#define WREG_BP_GPIOINTPOL          *(vu32*)0x18000070
#define WREG_BP_GPIOINTMASK         *(vu32*)0x18000074
#define WREG_BP_GPIOEVENT           *(vu32*)0x18000078
#define WREG_BP_GPIOEVENTINTMASK    *(vu32*)0x1800007C
#define WREG_BP_WATCHDOG            *(vu32*)0x18000080
#define WREG_BP_GPIOEVENTINTPOL     *(vu32*)0x18000084
#define WREG_BP_GPIOTIMERVAL        *(vu32*)0x18000088
#define WREG_BP_GPIOTIMEROUTMASK    *(vu32*)0x1800008C

#define WREG_BP_CLKCNT              *(vu32*)0x180001E0

#define WREG_PMU_CNT                *(vu32*)0x18000600
#define WREG_PMU_CAPS               *(vu32*)0x18000604
#define WREG_PMU_ST                 *(vu32*)0x18000608
#define WREG_PMU_RES_STATE          *(vu32*)0x1800060C
#define WREG_PMU_TIMER              *(vu32*)0x18000614
#define WREG_PMU_MIN_RES_MASK       *(vu32*)0x18000618
#define WREG_PMU_MAX_RES_MASK       *(vu32*)0x1800061C
#define WREG_PMU_RES_INDEX          *(vu32*)0x18000620
#define WREG_PMU_RES_DEPTHMAX       *(vu32*)0x18000624
#define WREG_PMU_RES_UPDN_TIMER     *(vu32*)0x18000628
#define WREG_PMU_RES_TIMER          *(vu32*)0x1800062C
#define WREG_PMU_WATCHDOG           *(vu32*)0x18000634
#define WREG_PMU_CHIP_CNT_ADDR      *(vu32*)0x18000650
#define WREG_PMU_CHIP_CNT_DATA      *(vu32*)0x18000654
#define WREG_PMU_REG_CNT_ADDR       *(vu32*)0x18000658
#define WREG_PMU_REG_CNT_DATA       *(vu32*)0x1800065C
#define WREG_PMU_PLL_CNT_ADDR       *(vu32*)0x18000660
#define WREG_PMU_PLL_CNT_DATA       *(vu32*)0x18000664

#define BP_IRQ_WDRESET              (1<<31)

// SDIO

#define WREG_SDIO_CNT               *(vu32*)0x18002000
#define WREG_SDIO_STATUS            *(vu32*)0x18002004
#define WREG_SDIO_IRQSTATUS         *(vu32*)0x18002020
#define WREG_SDIO_IRQENABLE_HOST    *(vu32*)0x18002024
#define WREG_SDIO_IRQENABLE_DEV     *(vu32*)0x18002028
#define WREG_SDIO_MAILBOX_DEV       *(vu32*)0x18002040
#define WREG_SDIO_MAILBOX_HOST      *(vu32*)0x18002044
#define WREG_SDIO_MBDATA_DEV        *(vu32*)0x18002048
#define WREG_SDIO_MBDATA_HOST       *(vu32*)0x1800204C
#define WREG_SDIO_INTRCVLAZY        *(vu32*)0x18002100

#define SDIO_IRQ_MB0_DEV            (1<<0)
#define SDIO_IRQ_MB1_DEV            (1<<1)
#define SDIO_IRQ_MB2_DEV            (1<<2)
#define SDIO_IRQ_MB3_DEV            (1<<3)
#define SDIO_IRQ_MBx_DEV            0xF
#define SDIO_IRQ_MB0_HOST           (1<<4)
#define SDIO_IRQ_MB1_HOST           (1<<5)
#define SDIO_IRQ_MB2_HOST           (1<<6)
#define SDIO_IRQ_MB3_HOST           (1<<7)
#define SDIO_IRQ_MBx_HOST           0xF0
#define SDIO_IRQ_RESET              (1<<30)
#define SDIO_IRQ_F2ENABLE           (1<<31)

// CPU

#define WREG_CPU_UNK000             *(vu32*)0x18003000
#define WREG_CPU_IRQUNK             *(vu32*)0x18003018
#define WREG_CPU_UNK020             *(vu32*)0x18003020
#define WREG_CPU_TICKCOUNT          *(vu32*)0x18003090
#define WREG_CPU_UNK1E0             *(vu32*)0x180031E0      // CPU clock control?

// RAM

#define WREG_RAM_BANK_INDEX         *(vu32*)0x18004010
#define WREG_RAM_STANDBY_CNT        *(vu32*)0x18004014

#define WREG_RAM_POWER_CNT          *(vu32*)0x180041E8

// OOB router

#define WREG_OOB_IRQNUM             *(vu32*)0x18108100

// private Cortex-M3 registers

#define WREG_DWT_COMP(n)            *(vu32*)(0xE0001020 + (n<<4))
#define WREG_DWT_MASK(n)            *(vu32*)(0xE0001024 + (n<<4))
#define WREG_DWT_FUNC(n)            *(vu32*)(0xE0001028 + (n<<4))

#define WREG_NVIC_IRQENABLE         *(vu32*)0xE000E100
#define WREG_NVIC_IRQDISABLE        *(vu32*)0xE000E180

#define WREG_NVIC_IRQPRIO(n)        *(vu32*)(0xE000E400 + (n<<2))

#define WREG_CPU_CONFIGCNT          *(vu32*)0xE000ED14
#define WREG_CPU_DEBUGCNT           *(vu32*)0xE000EDFC

#define IRQ_GENERAL                 (1<<0) // includes all IRQ sources
#define IRQ_BACKPLANE               (1<<1)
#define IRQ_MAC                     (1<<2)
#define IRQ_SDIO                    (1<<3)
#define IRQ_CPU                     (1<<4)
#define IRQ_USB                     (1<<6)

#endif
