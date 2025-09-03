.arch armv7-m
.cpu cortex-m3

.section ".crt0","ax"

.thumb
.align 1

.global _start
_start:
.word 0x48000			@ initial SP val
.word main
.word onException       @ 2
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException       @ 8
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException		@ 16
.word onException
.word onException
.word SDIO_IRQ
.word onException
.word onException
.word onException
.word onException
.word onException       @ 24
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException
.word onException       @ 31
