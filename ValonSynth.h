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

/**
 * Interface to a Valon 5007 dual synthesizer.
 * 
 * The reference signal, reference select, and flash commands are shared between
 * synthesizers. All other settings are independent, so a synthesizer must be
 * specified when getting and setting values. Frequency values are specified in
 * MegaHertz (MHz) unless otherwise noted.
 * 
 * \section calculations Calculations
 * 
 * The output frequency of the synthesizer is controlled by a number of
 * parameters.  The relationship between these parameters may be expressed by the
 * following equations. \f$EPDF\f$ is the Effective Phase Detector Frequency and is
 * the reference frequency(?) after applying the relevant options. (doubler,
 * halver and divider)
 * 
 * \f[
 *      1 \le dbf=2^n \le 16
 * \f]
 * \f[
 *      vco = frequency\times dbf
 * \f]
 * \f[
 *      ncount = \lfloor\frac{vco}{EPDF}\rfloor
 * \f]
 * \f[
 *      frac = \lfloor\frac{vco-ncount\times EPDF}{channel_spacing}\rfloor
 * \f]
 * \f[
 *      mod = \lfloor\frac{EPDF}{channel_spacing}+0.5\rfloor
 * \f]
 * 
 * \f$frac\f$ and \f$mod\f$ are a ratio, and can be reduced to the simplest
 * fraction after the calculations above. To compute the output frequency, use
 * the following equation.
 * 
 * \f[
 *      frequency = (ncount+\frac{frac}{mod})\times\frac{EPDF}{dbf}
 * \f]
 **/
class ValonSynth
{
public:
    /**
     * Uniquely identifies which of the board's synthesizers is being
     * referenced.
     **/
    enum Synthesizer { A = 0x00, B = 0x08 };
    
    /**
     * Holds various options used to control the operation of a synthesizer.
     **/
    struct options
    {
        /**
         * The synthesizer is operating in "low spur" mode. When not running in
         * this mode it is in "low noise" mode.
         **/
        bool low_spur;
        
        /**
         * The reference frequency doubler is active.
         **/
        bool double_ref;
        
        /**
         * The reference frequency halver is active.
         **/
        bool half_ref;
        
        /**
         * The reference frequency divider value;
         **/
        unsigned int r;
    };

    /**
     * Holds the range of the VCO.
     **/
    struct vco_range
    {
        /**
         * Minimum frequency the VCO is capable of producing.
         **/
        unsigned short min;
        
        /**
         * Maximum frequency the VCO is capable of producing.
         **/
        unsigned short max;
    };

    /**
     * Constructor.
     * @param[in] port The filename of the serial port device node.
     **/
    ValonSynth(const char *port);

    /**
     * \name Methods relating to output frequency
     * \{
     **/
    
    /**
     * Read the current settings from the synthesizer.
     * @param[in] synth The synthesizer to be read.
     * @return Frequency in MHz.
     **/
    float get_frequency(enum Synthesizer synth);
    
    /**
     * Read the current settings from the synthesizer.
     * @param[in] synth The synthesizer to be read.
     * @param[out] frequency Receives the frequency in MHz.
     * @return True on succesful completion.
     **/
    bool get_frequency(enum Synthesizer synth, float &frequency);
    
    /**
     * Set the synthesizer to the desired frequency, or best approximation based
     * on channel spacing. See the section on \ref calculations.
     * @param[in] synth The synthesizer to be set.
     * @param[in] frequency The desired output frequency in MHz. The range is
     *                      determined by the minimum and maximum VCO frequency.
     * @param[in] Chan_spacing The "resolution" of the synthesizer.
     **/
    bool set_frequency(enum Synthesizer synth, float frequency,
                        float chan_spacing = 10.0f);

    /**
     * \}
     * \name Methods relating to the reference frequency
     * \{
     **/
    
    /**
     * Read the current reference frequency. This is shared between the two
     * channels.
     * @return The reference frequency in Hz.
     **/
    unsigned int get_reference();
    
    /**
     * Read the current reference frequency. This is shared between the two
     * channels.
     * @param[out] reference Receives the reference frequency in Hz.
     * @return True on successful completion.
     **/
    bool get_reference(unsigned int &reference);
    
    /**
     * Set the synthesizer reference frequency. This does not appear to change
     * the actual reference frequency of the synthesizer for either internal or
     * external reference modes!?
     * @param reference The new reference frequency in Hz.
     * @return True on successful completion.
     **/
    bool set_reference(unsigned int reference);

    /**
     * \}
     * \name Members relating to the synthesizer options
     * 
     * Note that although the reference frequency is shared these options are
     * specific to a particular synthesizer.
     * \{
     **/
    
    /**
     * Read the current options for a synthesizer.
     * @param[in] synth The synthesizer to be read.
     * @param[out] opts Receives the options.
     * @return True on successful completion.
     **/
    bool get_options(enum Synthesizer synth, struct options &opts);
    
    /**
     * Set the options for a synthesizer.
     * @param[in] synth The synthesizer to be set.
     * @param[in] opts Structure holding the new options.
     * @return True on successful completion.
     **/
    bool set_options(enum Synthesizer synth, const struct options &opts);

    /**
     * \}
     * \name Methods relating to the reference source
     * \{
     **/
    
    /**
     * Read the current reference source.
     * @return True if external, false if internal.
     **/
    bool get_ref_select();
    
    /**
     * Read the current reference source.
     * @param[out] e_not_i Receives the refernce source. True if external,
     *                     false if internal.
     * @return True on successful completion.
     **/
    bool get_ref_select(bool &e_not_i);
    
    /**
     * Set the reference source.
     * @param[in] e_not_i True for external, false for internal.
     * @return True on successful completion.
     **/
    bool set_ref_select(bool e_not_i);

    /**
     * \}
     * \name Methods relating to the voltage controlled oscillator
     * \{
     **/
    
    /**
     * Read the current range of the VCO.
     * @param[in] synth The synthesizer to be read.
     * @param[out] vcor Receives the current VCO extent.
     * @return True on successful completion.
     **/
    bool get_vco_range(enum Synthesizer synth, vco_range &vcor);
    
    /**
     * Set the range of the VCO. This affects the allowable frequency range of
     * the output. See \ref calculations for details.
     * @param[in] synth The synthesizer to be read.
     * @param[in] vcor The extent to set.
     * @return True on successful completion.
     **/
    bool set_vco_range(enum Synthesizer synth, const vco_range &vcor);

    /**
     * \}
     * \name Methods relating to phase lock.
     * \{
     **/
    
    /**
     * Read the current state of phase lock.
     * @param[in] synth The synthesizer to be read.
     * @return True if the synthesizer is phase locked.
     **/
    bool get_phase_lock(enum Synthesizer synth);

    /**
     * Read the current state of the given synthesizers phase lock.
     * @param[in] synth The synthesizer to be read.
     * @param[out] locked Receives true if the synthesizer is phase locked.
     * @return True on successful completion.
     **/
    bool get_phase_lock(enum Synthesizer synth, bool &locked);

    /**
     * \}
     * \name Methods relating to synthesizer labels.
     * \{
     **/
    
    /**
     * Read the current label of the specified synthesizer.
     * @param[in] synth The synthesizer to be read.
     * @param[out] label Receives the label. The character buffer must be
     *                   allocated and large enough to accept the label.
     * @return True on successful completion.
     **/
    bool get_label(enum Synthesizer synth, char *label);
    
    /**
     * Set the label of the specified synthesizer.
     * @param[in] synth The synthesizer to be read.
     * @param[out] label The new label.
     * @return True on successful completion.
     **/
    bool set_label(enum Synthesizer synth, const char *label);

    /**
     * \}
     **/
    
    /**
     * Copies all current settings for both synthesizers to non-volatile flash
     * memory.
     * @return True on successful completion.
     **/
    bool flash();

private:
    enum { ACK  = 0x06,
           NACK = 0x15 };

    struct registers
    {
        unsigned int ncount;
        unsigned int frac;
        unsigned int mod;
        unsigned int dbf;
    };

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
    
    Serial s;
};

inline float
ValonSynth::get_frequency(enum Synthesizer synth)
{
    float frequency;
    get_frequency(synth, frequency);
    return frequency;
}

inline unsigned int
ValonSynth::get_reference()
{
    unsigned int frequency;
    get_reference(frequency);
    return frequency;
}

inline bool
ValonSynth::get_ref_select()
{
    bool e_not_i;
    get_ref_select(e_not_i);
    return e_not_i;
}

#endif//SYNTHESIZER_H
