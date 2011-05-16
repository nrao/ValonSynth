//# Copyright (C) 2011 Associated Universities, Inc. Washington DC, USA.
//# 
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//# 
//# This program is distributed in the hope that it will be useful, but
//# WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//# General Public License for more details.
//# 
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//# 
//# Correspondence concerning GBT software should be addressed as follows:
//#    GBT Operations
//#    National Radio Astronomy Observatory
//#    P. O. Box 2
//#    Green Bank, WV 24944-0002 USA

#include "Serial.h"
#include "ValonSynth.h"
#include <iostream>

// This code assumes that the effective phase detector frequency is 10MHz.
const float EPDF = 10.0f;

Synthesizer::Synthesizer(const char *port)
    :
    s(port)
{
}

//------------------//
// Output Frequency //
//------------------//
bool
Synthesizer::get_frequency(unsigned char synth, float &frequency)
{
    unsigned char bytes[24];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(bytes, 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 24, checksum)) return false;
#endif//VERIFY_CHECKSUM
    registers regs;
    unpack_freq_registers(bytes, regs);
    frequency = (regs.ncount + float(regs.frac) / regs.mod) * EPDF / regs.dbf;
    return true;
}

bool
Synthesizer::set_frequency(unsigned char synth, float frequency,
                           float chan_spacing)
{
    vco_range vcor;
    int dbf = 1;
    get_vco_range(synth, vcor);
    while(((frequency * dbf) <= vcor.min) && (dbf <= 16))
    {
        dbf *= 2;
    }
    if(dbf > 16)
    {
        dbf = 16;
    }
    float vco = frequency * dbf;
    registers regs;
    regs.ncount = int(vco / EPDF);
    regs.frac = int((vco - regs.ncount * EPDF) / chan_spacing);
    regs.mod = int(EPDF / chan_spacing + 0.5);
    regs.dbf = dbf;
    // Reduce frac/mod to simplest fraction
    if((regs.frac != 0) && (regs.mod != 0))
    {
        while(!(regs.frac & 1) && !(regs.mod & 1))
        {
            regs.frac /= 2;
            regs.mod /= 2;
        }
    }
    else
    {
        regs.frac = 0;
        regs.mod = 1;
    }
    // Write values to hardware
    unsigned char bytes[26];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(&bytes[1], 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 24, checksum)) return false;
#endif//VERIFY_CHECKSUM
    bytes[0] = 0x00 | synth;
    pack_freq_registers(regs, &bytes[1]);
    bytes[25] = generate_checksum(bytes, 25);
    s.write(bytes, 26);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//---------------------//
// Reference Frequency //
//---------------------//
bool
Synthesizer::get_reference(unsigned int &frequency)
{
    unsigned char bytes[4];
    unsigned char checksum;
    bytes[0] = 0x81;
    s.write(bytes, 1);
    s.read(bytes, 4);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if (!verify_checksum(bytes, 4, checksum)) return false;
#endif//VERIFY_CHECKSUM
    unpack_int(bytes, frequency);
    return true;
}

bool
Synthesizer::set_reference(unsigned int frequency)
{
    unsigned char bytes[6];
    bytes[0] = 0x01;
    pack_int(frequency, &bytes[1]);
    bytes[5] = generate_checksum(bytes, 5);
    s.write(bytes, 6);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//---------------------//
// Synthesizer Options //
//---------------------//
bool
Synthesizer::get_options(unsigned char synth, options &opts)
{
    unsigned char bytes[24];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(bytes, 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 24, checksum)) return false;
#endif//VERIFY_CHECKSUM
    //unsigned int reg0, reg1;
    unsigned int reg2;
    //unsigned int reg3, reg4, reg5;
    //unpack_int(&bytes[0], reg0);
    //unpack_int(&bytes[4], reg1);
    unpack_int(&bytes[8], reg2);
    //unpack_int(&bytes[12], reg3);
    //unpack_int(&bytes[16], reg4);
    //unpack_int(&bytes[20], reg5);
    opts.low_spur = ((reg2 >> 30) & 1) & ((reg2 >> 29) & 1);
    opts.double_ref = (reg2 >> 25) & 1;
    opts.half_ref = (reg2 >> 24) & 1;
    opts.r = (reg2 >> 14) & 0x03ff;
    return true;
}

bool
Synthesizer::set_options(unsigned char synth, const options &opts)
{
    unsigned char bytes[26];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(&bytes[1], 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 24, checksum)) return;
#endif//VERIFY_CHECKSUM
    //unsigned int reg0, reg1;
    unsigned int reg2;
    //unsigned int reg3, reg4, reg5;
    //unpack_int(&bytes[0], reg0);
    //unpack_int(&bytes[4], reg1);
    unpack_int(&bytes[8], reg2);
    //unpack_int(&bytes[12], reg3);
    //unpack_int(&bytes[16], reg4);
    //unpack_int(&bytes[20], reg5);
    reg2 &= 0x9cffffff;
    reg2 |= (((opts.low_spur & 1) << 30) | ((opts.low_spur & 1) << 29) |
             ((opts.double_ref & 1) << 25) | ((opts.half_ref & 1) << 24) |
             ((opts.r & 0x03ff) << 14));
    // Write values to hardware
    bytes[0] = 0x00 | synth;
    pack_int(reg2, &bytes[8]);
    bytes[25] = generate_checksum(bytes, 25);
    s.write(bytes, 26);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//------------------//
// Reference Select //
//------------------//
bool
Synthesizer::get_ref_select(bool &e_not_i)
{
    unsigned char bytes;
    unsigned char checksum;
    bytes = 0x86;
    s.write(&bytes, 1);
    s.read(&bytes, 1);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(&bytes, 1, checksum)) return false;
#endif//VERIFY_CHECKSUM
    e_not_i = bytes & 1;
    return true;
}

bool
Synthesizer::set_ref_select(bool e_not_i)
{
    unsigned char bytes[3];
    bytes[0] = 0x06;
    bytes[1] = e_not_i & 1;
    bytes[2] = generate_checksum(bytes, 2);
    s.write(bytes, 3);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//-----------//
// VCO Range //
//-----------//
bool
Synthesizer::get_vco_range(unsigned char synth, vco_range &vcor)
{
    unsigned char bytes[4];
    unsigned char checksum;
    bytes[0] = 0x83 | synth;
    s.write(bytes, 1);
    s.read(bytes, 4);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 4, checksum)) return false;
#endif//VERIFY_CHECKSUM
    unpack_short(&bytes[0], vcor.min);
    unpack_short(&bytes[2], vcor.max);
    return true;
}

bool
Synthesizer::set_vco_range(unsigned char synth, const vco_range &vcor)
{
    unsigned char bytes[6];
    bytes[0] = 0x03 | synth;
    pack_short(vcor.min, &bytes[1]);
    pack_short(vcor.max, &bytes[3]);
    bytes[5] = generate_checksum(bytes, 5);
    s.write(bytes, 5);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//------------//
// Phase Lock //
//------------//
bool
Synthesizer::get_phase_lock(unsigned char synth, bool &locked)
{
    unsigned char bytes;
    unsigned char checksum;
    bytes = 0x86 | synth;
    s.write(&bytes, 1);
    s.read(&bytes, 1);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(&bytes, 1, checksum)) return false;
#endif//VERIFY_CHECKSUM
    int mask;
    // Synthesizer A
    if(synth == Synthesizer::A) mask = 0x20;
    // Synthesizer B
    else mask = 0x10;
    locked = bytes & mask;
    return true;
}

//-------------------//
// Synthesizer Label //
//-------------------//
bool
Synthesizer::get_label(unsigned char synth, char *label)
{
    unsigned char bytes[16];
    unsigned char checksum;
    bytes[0] = 0x82 | synth;
    s.write(bytes, 1);
    s.read(bytes, 16);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 16, checksum)) return false;
#endif//VERIFY_CHECKSUM
    memcpy(label, (char*)bytes, 16);
    return true;
}

bool
Synthesizer::set_label(unsigned char synth, const char *label)
{
    unsigned char bytes[18];
    bytes[0] = 0x02 | synth;
    memcpy(&bytes[1], label, 16);
    bytes[17] = generate_checksum(bytes, 17);
    s.write(bytes, 18);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//-------//
// Flash //
//-------//
bool
Synthesizer::flash()
{
    unsigned char bytes[2];
    bytes[0] = 0x40;
    bytes[1] = generate_checksum(bytes, 1);
    s.write(bytes, 2);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//----------//
// Checksum //
//----------//
unsigned char
Synthesizer::generate_checksum(const unsigned char *bytes, int length)
{
    unsigned int sum = 0;
    for(int i = 0; i < length; ++i)
    {
        sum += bytes[i];
    }
    return (unsigned char)(sum % 256);
}

bool
Synthesizer::verify_checksum(const unsigned char *bytes, int length,
                             unsigned char checksum)
{
    return (generate_checksum(bytes, length) == checksum);
}

//-------------//
// Bit Packing //
//-------------//
void
Synthesizer::pack_freq_registers(const registers &regs, unsigned char *bytes)
{
    int dbf = 0;
    switch(regs.dbf)
    {
    case 1: dbf = 0; break;
    case 2: dbf = 1; break;
    case 4: dbf = 2; break;
    case 8: dbf = 3; break;
    case 16: dbf = 4; break;
    }
    unsigned int reg0, reg1;
    //unsigned int reg2, reg3;
    unsigned int reg4;
    //unsigned int reg5;
    unpack_int(&bytes[0], reg0);
    unpack_int(&bytes[4], reg1);
    //unpack_int(&bytes[8], reg2);
    //unpack_int(&bytes[12], reg3);
    unpack_int(&bytes[16], reg4);
    //unpack_int(&bytes[20], reg5);
    reg0 &= 0x80000007;
    reg0 |= ((regs.ncount & 0xffff) << 15) | ((regs.frac & 0x0fff) << 3);
    reg1 &= 0xffff8007;
    reg1 |= (regs.mod & 0x0fff) << 3;
    reg4 &= 0xff8fffff;
    reg4 |= dbf << 20;
    pack_int(reg0, &bytes[0]);
    pack_int(reg1, &bytes[4]);
    //pack_int(reg2, &bytes[8]);
    //pack_int(reg3, &bytes[12]);
    pack_int(reg4, &bytes[16]);
    //pack_int(reg5, &bytes[20]);
}

void
Synthesizer::unpack_freq_registers(const unsigned char *bytes, registers &regs)
{
    unsigned int reg0, reg1;
    //unsigned int reg2, reg3;
    unsigned int reg4;
    //unsigned int reg5;
    unpack_int(&bytes[0], reg0);
    unpack_int(&bytes[4], reg1);
    //unpack_int(&bytes[8], reg2);
    //unpack_int(&bytes[12], reg3);
    unpack_int(&bytes[16], reg4);
    //unpack_int(&bytes[20], reg5);
    regs.ncount = (reg0 >> 15) & 0xffff;
    regs.frac = (reg0 >> 3) & 0x0fff;
    regs.mod = (reg1 >> 3) & 0x0fff;
    int dbf = (reg4 >> 20) & 0x07;
    switch(dbf)
    {
    case 0: regs.dbf = 1; break;
    case 1: regs.dbf = 2; break;
    case 2: regs.dbf = 4; break;
    case 3: regs.dbf = 8; break;
    case 4: regs.dbf = 16; break;
    default: regs.dbf = 1;
    }
}

void
Synthesizer::pack_int(unsigned int num, unsigned char *bytes)
{
    bytes[0] = (num >> 24) & 0xff;
    bytes[1] = (num >> 16) & 0xff;
    bytes[2] = (num >> 8) & 0xff;
    bytes[3] = (num) & 0xff;
}

void
Synthesizer::pack_short(unsigned short num, unsigned char *bytes)
{
    bytes[0] = (num >> 8) & 0xff;
    bytes[1] = (num) & 0xff;
}

void
Synthesizer::unpack_int(const unsigned char *bytes, unsigned int &num)
{
    num = ((int(bytes[0]) << 24) + (int(bytes[1]) << 16) +
           (int(bytes[2]) << 8) + int(bytes[3]));
}

void
Synthesizer::unpack_short(const unsigned char *bytes, unsigned short &num)
{
    num = (int(bytes[0]) << 8) + int(bytes[1]);
}
