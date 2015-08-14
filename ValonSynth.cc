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
    std::uint32_t ncount, frac, mod, dbf;
    bool success = read_freq_registers(synth, ncount, frac, mod, dbf);
    float EPDF = getEPDF(synth);
    frequency = (ncount + float(frac) / mod) * EPDF / dbf;
    return success;
}

bool
ValonSynth::set_frequency(enum ValonSynth::Synthesizer synth, float frequency,
                          float chan_spacing)
{
    vco_range vcor;
    std::uint32_t dbf = 1;
    bool success = get_vco_range(synth, vcor);
    while(((frequency * dbf) <= vcor.min) && (dbf <= 16))
    {
        dbf *= 2;
    }
    if(dbf > 16)
    {
        dbf = 16;
    }
    float vco = frequency * dbf;
    float EPDF = getEPDF(synth);
    std::uint32_t ncount = std::uint32_t(vco / EPDF);
    std::uint32_t frac = std::uint32_t((vco - ncount * EPDF) / chan_spacing + 0.5);
    std::uint32_t mod = std::uint32_t(EPDF / chan_spacing + 0.5);
    // Reduce frac/mod to simplest fraction
    if((frac != 0) && (mod != 0))
    {
        while(!(frac & 1) && !(mod & 1))
        {
            frac /= 2;
            mod /= 2;
        }
    }
    else
    {
        frac = 0;
        mod = 1;
    }
    return success && write_freq_registers(synth, ncount, frac, mod, dbf);
}

//---------------------//
// Reference Frequency //
//---------------------//
bool
ValonSynth::get_reference(std::uint32_t &frequency)
{
    std::uint8_t bytes[4];
    std::uint8_t checksum;
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
ValonSynth::set_reference(std::uint32_t frequency)
{
    std::uint8_t bytes[6];
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
ValonSynth::get_rf_level(enum ValonSynth::Synthesizer synth, std::int32_t &rf_level)
{
    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;
    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);
    switch(static_cast<std::uint32_t>(r4.output_power))
    {
    case 0: rf_level = -4; break;
    case 1: rf_level = -1; break;
    case 2: rf_level =  2; break;
    case 3: rf_level =  5; break;
    }
    return success;
}

bool
ValonSynth::set_rf_level(enum ValonSynth::Synthesizer synth, int32_t rf_level)
{
    std::uint32_t rfl;
    switch(rf_level)
    {
    case -4: rfl = 0; break;
    case -1: rfl = 1; break;
    case 2:  rfl = 2; break;
    case 5:  rfl = 3; break;
    default: return false;
    }
    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;
    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);
    r4.output_power = rfl;
    return success && set_all_registers(synth, r0, r1, r2, r3, r4, r5);
}

//---------------------//
// ValonSynth Options //
//---------------------//
bool
ValonSynth::get_options(enum ValonSynth::Synthesizer synth, options &opts)
{
    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;
    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);
    opts.double_ref = r2.double_r;
    opts.half_ref   = r2.half_r;
    opts.r          = r2.r;
    opts.low_spur   = bool(r2.low_spur);
    return success;
}

bool
ValonSynth::set_options(enum ValonSynth::Synthesizer synth,
                        const options &opts)
{
    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;
    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);
    r2.double_r = opts.double_ref;
    r2.half_r   = opts.half_ref;
    r2.r        = opts.r;
    // Value must be either 0x0 or 0x3
    r2.low_spur = opts.low_spur ? 3 : 0;
    return success && set_all_registers(synth, r0, r1, r2, r3, r4, r5);
}

//------------------//
// Reference Select //
//------------------//
bool
ValonSynth::get_ref_select(bool &e_not_i)
{
    std::uint8_t bytes;
    std::uint8_t checksum;
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
    std::uint8_t bytes[3];
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
    std::uint8_t bytes[4];
    std::uint8_t checksum;
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
    std::uint8_t bytes[6];
    bytes[0] = 0x03 | synth;
    pack_short(vcor.min, &bytes[1]);
    pack_short(vcor.max, &bytes[3]);
    bytes[5] = generate_checksum(bytes, 5);
    s.write(bytes, 6);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

//------------//
// Phase Lock //
//------------//
bool
ValonSynth::get_phase_lock(enum ValonSynth::Synthesizer synth, bool &locked)
{
    std::uint8_t bytes;
    std::uint8_t checksum;
    bytes = 0x86 | synth;
    s.write(&bytes, 1);
    s.read(&bytes, 1);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(&bytes, 1, checksum)) return false;
#endif//VERIFY_CHECKSUM
    std::uint32_t mask;
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
    std::uint8_t bytes[16];
    std::uint8_t checksum;
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
    std::uint8_t bytes[18];
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
    std::uint8_t bytes[2];
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
std::uint8_t
ValonSynth::generate_checksum(const std::uint8_t *bytes, std::size_t length)
{
    std::uint32_t sum = 0;
    for(std::size_t i = 0; i < length; ++i)
    {
        sum += bytes[i];
    }
    return (uint8_t)(sum % 256);
}

bool
ValonSynth::verify_checksum(const std::uint8_t *bytes, std::size_t length,
                            std::uint8_t checksum)
{
    return (generate_checksum(bytes, length) == checksum);
}

//-------------//
// Bit Packing //
//-------------//
bool ValonSynth::write_freq_registers(enum ValonSynth::Synthesizer synth,
                                      std::uint32_t ncount, std::uint32_t frac,
                                      std::uint32_t mod, std::uint32_t dbf) {
    std::uint32_t raw_dbf;
    switch(dbf)
    {
    case  1: raw_dbf = 0; break;
    case  2: raw_dbf = 1; break;
    case  4: raw_dbf = 2; break;
    case  8: raw_dbf = 3; break;
    case 16: raw_dbf = 4; break;
    }

    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;

    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);

    r0.ncount = ncount;
    r0.frac = frac;
    r1.mod = mod;
    r4.divider_select = raw_dbf;

    return success && set_all_registers(synth, r0, r1, r2, r3, r4, r5);
}

bool ValonSynth::read_freq_registers(enum ValonSynth::Synthesizer synth,
                                     std::uint32_t &ncount, std::uint32_t &frac,
                                     std::uint32_t &mod, std::uint32_t &dbf) {
    reg0_t r0;
    reg1_t r1;
    reg2_t r2;
    reg3_t r3;
    reg4_t r4;
    reg5_t r5;

    bool success = get_all_registers(synth, r0, r1, r2, r3, r4, r5);

    ncount = r0.ncount;
    frac = r0.frac;
    mod = r1.mod;
    switch(static_cast<std::uint32_t>(r4.divider_select))
    {
    case  0: dbf = 1; break;
    case  1: dbf = 2; break;
    case  2: dbf = 4; break;
    case  3: dbf = 8; break;
    case  4: dbf = 16; break;
    default: dbf = 1;
    }

    return success;
}

bool ValonSynth::get_all_registers(enum ValonSynth::Synthesizer synth,
                       reg0_t &r0, reg1_t &r1,
                       reg2_t &r2, reg3_t &r3,
                       reg4_t &r4, reg5_t &r5) {
    std::uint8_t bytes[24];
    std::uint8_t checksum;
    bytes[0] = 0x80 | synth;
    s.write(bytes, 1);
    s.read(bytes, 24);
    s.read(&checksum, 1);
#ifdef VERIFY_CHECKSUM
    if(!verify_checksum(bytes, 24, checksum)) return false;
#endif//VERIFY_CHECKSUM
    unpack_int(&bytes[ 0], r0.asbyte);
    unpack_int(&bytes[ 4], r1.asbyte);
    unpack_int(&bytes[ 8], r2.asbyte);
    unpack_int(&bytes[12], r3.asbyte);
    unpack_int(&bytes[16], r4.asbyte);
    unpack_int(&bytes[20], r5.asbyte);
    return true;
}

bool ValonSynth::set_all_registers(enum ValonSynth::Synthesizer synth,
                                   const reg0_t &r0, const reg1_t &r1,
                                   const reg2_t &r2, const reg3_t &r3,
                                   const reg4_t &r4, const reg5_t &r5) {
    std::uint8_t bytes[26];
    bytes[0] = 0x00 | synth;
    pack_int(r0.asbyte, &bytes[0]);
    pack_int(r1.asbyte, &bytes[4]);
    pack_int(r2.asbyte, &bytes[8]);
    pack_int(r3.asbyte, &bytes[12]);
    pack_int(r4.asbyte, &bytes[16]);
    pack_int(r5.asbyte, &bytes[20]);
    bytes[25] = generate_checksum(bytes, 25);
    s.write(bytes, 26);
    s.read(bytes, 1);
    return bytes[0] == ACK;
}

void
ValonSynth::pack_int(std::uint32_t num, std::uint8_t *bytes)
{
    bytes[0] = (num >> 24) & 0xff;
    bytes[1] = (num >> 16) & 0xff;
    bytes[2] = (num >> 8) & 0xff;
    bytes[3] = (num) & 0xff;
}

void
ValonSynth::pack_short(std::uint16_t num, std::uint8_t *bytes)
{
    bytes[0] = (num >> 8) & 0xff;
    bytes[1] = (num) & 0xff;
}

void
ValonSynth::unpack_int(const std::uint8_t *bytes, std::uint32_t &num)
{
    num = ((std::uint32_t(bytes[0]) << 24) + (std::uint32_t(bytes[1]) << 16) +
           (std::uint32_t(bytes[2]) <<  8) + (std::uint32_t(bytes[3])));
}

void
ValonSynth::unpack_short(const std::uint8_t *bytes, std::uint16_t &num)
{
    num = (std::uint16_t(bytes[0]) << 8) + std::uint16_t(bytes[1]);
}
