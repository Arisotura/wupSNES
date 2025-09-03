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

#ifndef PPU_H
#define PPU_H

void PPU_Init();
void PPU_Reset();

u8 PPU_Read8(u32 addr);
void PPU_Write8(u32 addr, u8 val);

void PPU_SNESVBlank();
void PPU_SNESVBlankEnd();
void PPU_SNESHBlank();

void PPU_VBlank();
void PPU_HBlank();

#endif
