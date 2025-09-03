.arch armv7-m
.cpu cortex-m3

.text

.thumb
.align 1

.global EnableIRQ
.type EnableIRQ, %function
EnableIRQ:
    cpsie i
    bx lr

.global DisableIRQ
.type DisableIRQ, %function
DisableIRQ:
    cpsid i
    bx lr

.global WaitForIRQ
.type WaitForIRQ, %function
WaitForIRQ:
    wfi
    bx lr
