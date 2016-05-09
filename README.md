# Summary
The Valon 5007 board houses two synthesizers.  The reference signal, reference select, and flash commands are shared between synthesizers.  All other settings are independent, so a synthesizer must be specified when getting and setting values.  Frequency values are specified in MegaHertz (MHz) unless otherwise noted.

# Installation
## Python
The python code works with the standard distutils/setuptools installer.  To install the library, download the code and extract it.

    $ cd path/to/code
    $ python setup.py build
    $ python setup.py install --prefix=path/to/install

## C++
Included with the C++ code is a simple makefile that produces statically-linked (.a) and dynamically-linked (.so) libraries.  Installing these to the proper directory must be done manually.

    $ cd path/to/code
    $ make
    $ cp ValonSynth.a path/to/install
    $ cp ValonSynth.so path/to/install

# Commands
Text in square brackets [ ], unless otherwise noted, denote a C++ version of the function which fills a value or structure rather than returning a new object.  All C++ functions which use this convention return a boolean (true/false) indicating success or failure of the function’s task.

## get_frequency(unsigned char synthesizer[, float &frequency])
### Description:
Reads the current settings from the synthesizer and returns the frequency in MHz.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++)

## set_frequency(unsigned char synthesizer, float frequency, float channel_spacing)
### Description:
Sets the synthesizer to the desired frequency, or closest possible frequency (depending on the channel spacing).

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).
* frequency (float) – Specifies the desired output frequency of the synthesizer.  Range is determined by the minimum and maximum VCO frequency.  See Calculations section for more details.
* channel_spacing (float) [optional] – Specifies the “resolution” of the synthesizer.  See Calculations section for more details.

## get_reference()
### Description:
Reads the current settings from the synthesizer and returns the reference frequency in Hertz (Hz).  This setting is shared between synthesizers.

### Arguments:
None.

## set_reference(float frequency)
### Description:
Sets the synthesizer reference register to the desired frequency in Hertz (Hz).  This does not appear to change the actual reference frequency of the synthesizer for either internal or external reference modes (?).

### Arguments:
* frequency (float) – Specifies the desired reference frequency in Hz.

## get_rf_level(unsigned char synthesizer[, int &rf_level])
### Description:
Reads the current settings from the synthesizer and returns the RF level in dBm. Valid settings are -4, -1, 2, and 5 dBm.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).

## set_rf_level(unsigned char synthesizer, int rf_level)
### Description:
Sets the synthesizer RF level. Valid settings are -4, -1, 2, and 5; other settings cause this command to fail.

### Arguments
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).
* rf_level (int) - Specifies the desired output level of the synthesizer

## get_options(unsigned char synthesizer[, struct options &opts])
### Description:
Reads the current settings from the synthesizer and returns the options fields.  The Python version returns the following fields as a tuple.  The C++ version returns a structure which contains the following fields.

* double_ref – Doubles the input reference frequency.
* half_ref – Halves the input reference frequency.
* r – Divides the input reference frequency by an integer.
* low_spur – Sets the synthesizer to operate in either “low noise” or “low spur” mode.
Note that the reference frequency options are applied to a single synthesizer, even though the base reference frequency input is shared.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).

## set_options(unsigned char synthesizer[, bool double, bool half, int r, bool low_spur])
### Description:
Sets the desired options on the selected synthesizer.  This is the Python version of set_options.

### Arguments:
All arguments are optional, and may be specified either positionally or by keyword.

* double_ref (bool) – Doubles the input reference frequency.
* half_ref (bool) – Halves the input reference frequency.
* r (int) – Divides the input reference frequency by an integer.
* low_spur (bool) – Sets the synthesizer to operate in either “low noise” or “low spur” mode.

## set_options(unsigned char synth, struct options &opts)
### Description:
This is the C++ version of set_options.

### Arguments:
* opts (struct options) – Structure containing double_ref, half_ref, r, and low_spur described in the Python version of set_options.

## get_ref_select([bool &e_not_i])
### Description:
Reads the current settings from the synthesizer and returns the state of the internal/external reference select.

### Arguments:
None.

## set_ref_select(bool e_not_i)
### Description:
Sets the synthesizer reference select to use either the internal reference source or an external reference source.

### Arguments:
* e_not_i (bool) – If true, uses an external reference source; if false, uses the internal reference source.

## get_vco_range(unsigned char synthesizer[, struct vco_range &vcor])
### Description:
Reads the current synthesizer settings and returns the VCO range settings.  The Python version returns a tuple of the form (minimum, maximum).  The C++ version fills a structure containing min and max fields.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).

## set_vco_range(unsigned char synthesizer, short min, short max)
### Description:
Sets the min and max allowable VCO frequencies. These values are used to calculate the allowable frequency range for the output frequency, see Calculations for details.  This is the Python version of set_vco_range.

### Arguments:
* min (short) – The minimum allowable VCO frequency.
* max (short) – The maximum allowable VCO frequency.

## set_vco_range(unsigned char synthesizer, struct vco_range &vcor)
### Description:
Sets the minimum and maximum allowable VCO frequencies. This is the  C++ version of set_vco_range.

### Arguments:
* vcor (struct vco_range) – Contains the min and max fields, respectively, as described above in the Python version of set_vco_range.

## get_phase_lock(unsigned char synthesizer)
### Description:
Returns a bool indicating whether the specified synthesizer is phase-locked to the input reference.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).

## get_synthesizer_label(unsigned char synthesizer[, char *label])
### Description:
Retrieves the current label for the specified synthesizer.  The C++ version requires that the input pointer be valid and point to currently allocated memory of the correct size.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).

## set_label(unsigned char synthesizer, char *label)
### Description:
Sets the label for the selected synthesizer.

### Arguments:
* synthesizer (unsigned char) – Specifies which synthesizer this command affects.  Values are aliased for convenience: SYNTH_A and SYNTH_B (Python) or ValonSynth::A and ValonSynth::B (C++).
* label (char*) - The new synthesizer label.

## flash()
### Description:
Flashes all current settings for both synthesizers into non-volatile memory.
### Arguments:
None.

#Calculations
In order to set the output frequency of the synthesizer, several calculations are done using the settings of the synthesizer.  EPDF stands for Effective Phase Detector Frequency, which is the reference frequency after applying the relevant options (double_ref, half_ref, r).

`1 <= dbf = 2^n <= 16`

`vco = frequency * dbf`

`ncount = floor(vco / EPDF)`

`frac = floor((vco - ncount * EPDF) / channel_spacing)`

`mod = floor((EPDF / channel_spacing) + 0.5)`

`frac` and `mod` are a ratio, and can be reduced to the simplest fraction after the calculations above.  To compute the output frequency, use the following equation.

`frequency = (ncount + frac / mod) * EPDF / dbf`
