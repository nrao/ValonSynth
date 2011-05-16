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
//#	GBT Operations
//#	National Radio Astronomy Observatory
//#	P. O. Box 2
//#	Green Bank, WV 24944-0002 USA

#ifndef SYNTHESIZER_H
#define SYNTHESIZER_H

// Serial class must support
// * read(unsigned char*, int)
// * write(unsigned char*, int)
//class Serial;
#include "Serial.h"

struct registers
{
    unsigned int ncount;
    unsigned int frac;
    unsigned int mod;
    unsigned int dbf;
};

struct options
{
    // Sets the synthesizer in "low noise" or "low spur" mode
    // NOTE: When not in low spur mode, the synthesizer operates in low noise
    // mode and vice versa.
    bool low_spur;
    // Doubles the input reference frequency
    bool double_ref;
    // Halves the input reference frequency
    bool half_ref;
    // Divides the input reference frequency
    unsigned int r;
};

struct vco_range
{
    unsigned short min;
    unsigned short max;
};

class Synthesizer
{
public:
    enum { A = 0x00,
           B = 0x08,
           ACK  = 0x06,
           NACK = 0x15 };

    Synthesizer(const char *port);

    // Output frequency
    float get_frequency(unsigned char synth);
    bool  get_frequency(unsigned char synth, float &frequency);
    bool  set_frequency(unsigned char synth, float frequency,
                        float chan_spacing = 10.0f);

    // Reference frequency
    unsigned int  get_reference();
    bool get_reference(unsigned int &reference);
    bool set_reference(unsigned int reference);

    // Synthesizer options
    bool get_options(unsigned char synth, options &opts);
    bool set_options(unsigned char synth, const options &opts);

    // Reference select (internal/external)
    bool get_ref_select();
    bool get_ref_select(bool &e_not_i);
    // E_NOT_I = true means that the synthesizer will use external reference
    bool set_ref_select(bool e_not_i);

    // VCO range
    bool get_vco_range(unsigned char synth, vco_range &vcor);
    bool set_vco_range(unsigned char synth, const vco_range &vcor);

    // Phase lock
    bool get_phase_lock(unsigned char synth);
    bool get_phase_lock(unsigned char synth, bool &locked);

    // Synthesizer label
    bool get_label(unsigned char synth, char *label);
    bool set_label(unsigned char synth, const char *label);

    // Flash volatile settings
    bool flash();

protected:
    // Checksum
    unsigned char generate_checksum(const unsigned char*, int);
    bool verify_checksum(const unsigned char*, int, unsigned char);

    // Register formatting
    void pack_freq_registers(const registers &regs, unsigned char *bytes);
    void unpack_freq_registers(const unsigned char *bytes, registers &regs);

    void pack_int(unsigned int num, unsigned char *bytes);
    void pack_short(unsigned short num, unsigned char *bytes);
    void unpack_int(const unsigned char *bytes, unsigned int &num);
    void unpack_short(const unsigned char *bytes, unsigned short &num);

private:
    Serial s;
};

inline float
Synthesizer::get_frequency(unsigned char synth)
{
    float frequency;
    get_frequency(synth, frequency);
    return frequency;
}

inline unsigned int
Synthesizer::get_reference()
{
    unsigned int frequency;
    get_reference(frequency);
    return frequency;
}

inline bool
Synthesizer::get_ref_select()
{
    bool e_not_i;
    get_ref_select(e_not_i);
    return e_not_i;
}

#endif//SYNTHESIZER_H
