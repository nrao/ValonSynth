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
#include <cstring>


ValonSynth::ValonSynth(const char *port)
    :
    s(port)
{
}

//------------------//
// Output Frequency //
//------------------//
bool
ValonSynth::get_frequency(enum ValonSynth::Synthesizer synth, float &frequency)
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
    float EPDF = getEPDF(synth);
    unpack_freq_registers(bytes, regs);
    frequency = (regs.ncount + float(regs.frac) / regs.mod) * EPDF / regs.dbf;
    return true;
}

bool
ValonSynth::set_frequency(enum ValonSynth::Synthesizer synth, float frequency,
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
    float EPDF = getEPDF(synth);
    regs.ncount = int(vco / EPDF);
    regs.frac = int((vco - regs.ncount * EPDF) / chan_spacing + 0.5);
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
ValonSynth::get_reference(unsigned int &frequency)
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
ValonSynth::set_reference(unsigned int frequency)
{
    unsigned char bytes[6];
    bytes[0] = 0x01;
    pack_int(frequency, &bytes[1]);
    bytes[5] = generate_checksum(bytes, 5);
    s.write(bytes, 6);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//----------//
// RF Level //
//----------//
bool
ValonSynth::get_rf_level(enum ValonSynth::Synthesizer synth, int &rf_level)
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
    //unsigned int reg0, reg1, reg2, reg3;
    unsigned int reg4;
    //unsigned int reg5;
    //unpack_int(&bytes[0], reg0);
    //unpack_int(&bytes[4], reg1);
    //unpack_int(&bytes[8], reg2);
    //unpack_int(&bytes[12], reg3);
    unpack_int(&bytes[16], reg4);
    //unpack_int(&bytes[20], reg5);
    int rfl = (reg4 >> 3) & 0x03;
    switch(rfl)
    {
    case 0: rf_level = -4; break;
    case 1: rf_level = -1; break;
    case 2: rf_level =  2; break;
    case 3: rf_level =  5; break;
    }
    return true;
}

bool
ValonSynth::set_rf_level(enum ValonSynth::Synthesizer synth, int rf_level)
{
    int rfl = 0;
    switch(rf_level)
    {
    case -4: rfl = 0; break;
    case -1: rfl = 1; break;
    case 2:  rfl = 2; break;
    case 5:  rfl = 3; break;
    default: return false;
    }
    unsigned char bytes[26];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(&bytes[1], 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(&bytes[1], 24, checksum)) return;
#endif//VERIFY_CHECKSUM
    //unsigned int reg0, reg1, reg2, reg3;
    unsigned int reg4;
    //unsigned int reg5;
    //unpack_int(&bytes[1], reg0);
    //unpack_int(&bytes[5], reg1);
    //unpack_int(&bytes[9], reg2);
    //unpack_int(&bytes[13], reg3);
    unpack_int(&bytes[17], reg4);
    //unpack_int(&bytes[21], reg5);
    reg4 &= 0xffffffe7;
    reg4 |= (rfl & 0x03) << 3;
    // Write values to hardware
    bytes[0] = 0x00 | synth;
    pack_int(reg4, &bytes[17]);
    bytes[25] = generate_checksum(bytes, 25);
    s.write(bytes, 26);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//---------------------//
// ValonSynth Options //
//---------------------//
bool
ValonSynth::get_options(enum ValonSynth::Synthesizer synth, options &opts)
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
ValonSynth::set_options(enum ValonSynth::Synthesizer synth,
                        const options &opts)
{
    unsigned char bytes[26];
    unsigned char checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(&bytes[1], 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(&bytes[1], 24, checksum)) return;
#endif//VERIFY_CHECKSUM
    //unsigned int reg0, reg1;
    unsigned int reg2;
    //unsigned int reg3, reg4, reg5;
    //unpack_int(&bytes[1], reg0);
    //unpack_int(&bytes[5], reg1);
    unpack_int(&bytes[9], reg2);
    //unpack_int(&bytes[13], reg3);
    //unpack_int(&bytes[17], reg4);
    //unpack_int(&bytes[21], reg5);
    reg2 &= 0x9c003fff;
    reg2 |= (((opts.low_spur & 1) << 30) | ((opts.low_spur & 1) << 29) |
             ((opts.double_ref & 1) << 25) | ((opts.half_ref & 1) << 24) |
             ((opts.r & 0x03ff) << 14));
    // Write values to hardware
    bytes[0] = 0x00 | synth;
    pack_int(reg2, &bytes[9]);
    bytes[25] = generate_checksum(bytes, 25);
    s.write(bytes, 26);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//------------------//
// Reference Select //
//------------------//
bool
ValonSynth::get_ref_select(bool &e_not_i)
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
ValonSynth::set_ref_select(bool e_not_i)
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
ValonSynth::get_vco_range(enum ValonSynth::Synthesizer synth, vco_range &vcor)
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
ValonSynth::set_vco_range(enum ValonSynth::Synthesizer synth,
                          const vco_range &vcor)
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
ValonSynth::get_phase_lock(enum ValonSynth::Synthesizer synth, bool &locked)
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
    // ValonSynth A
    if(synth == ValonSynth::A) mask = 0x20;
    // ValonSynth B
    else mask = 0x10;
    locked = bytes & mask;
    return true;
}

//-------------------//
// ValonSynth Label //
//-------------------//
bool
ValonSynth::get_label(enum ValonSynth::Synthesizer synth, char *label)
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
ValonSynth::set_label(enum ValonSynth::Synthesizer synth, const char *label)
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
ValonSynth::flash()
{
    unsigned char bytes[2];
    bytes[0] = 0x40;
    bytes[1] = generate_checksum(bytes, 1);
    s.write(bytes, 2);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//------------------//
// EDPF Calculation //
//------------------//
float
ValonSynth::getEPDF(enum ValonSynth::Synthesizer synth)
{
    options opts;
    float reference = get_reference() / 1e6;
    get_options(synth, opts);

    if(opts.double_ref) reference *= 2.0;
    if(opts.half_ref) reference /= 2.0;
    if(opts.r > 1) reference /= opts.r;
    return reference;
}

//----------//
// Checksum //
//----------//
unsigned char
ValonSynth::generate_checksum(const unsigned char *bytes, int length)
{
    unsigned int sum = 0;
    for(int i = 0; i < length; ++i)
    {
        sum += bytes[i];
    }
    return (unsigned char)(sum % 256);
}

bool
ValonSynth::verify_checksum(const unsigned char *bytes, int length,
                            unsigned char checksum)
{
    return (generate_checksum(bytes, length) == checksum);
}

//-------------//
// Bit Packing //
//-------------//
void
ValonSynth::pack_freq_registers(const registers &regs, unsigned char *bytes)
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
ValonSynth::unpack_freq_registers(const unsigned char *bytes, registers &regs)
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
ValonSynth::pack_int(unsigned int num, unsigned char *bytes)
{
    bytes[0] = (num >> 24) & 0xff;
    bytes[1] = (num >> 16) & 0xff;
    bytes[2] = (num >> 8) & 0xff;
    bytes[3] = (num) & 0xff;
}

void
ValonSynth::pack_short(unsigned short num, unsigned char *bytes)
{
    bytes[0] = (num >> 8) & 0xff;
    bytes[1] = (num) & 0xff;
}

void
ValonSynth::unpack_int(const unsigned char *bytes, unsigned int &num)
{
    num = ((int(bytes[0]) << 24) + (int(bytes[1]) << 16) +
           (int(bytes[2]) << 8) + int(bytes[3]));
}

void
ValonSynth::unpack_short(const unsigned char *bytes, unsigned short &num)
{
    num = (int(bytes[0]) << 8) + int(bytes[1]);
}
