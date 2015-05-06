# Copyright (C) 2011 Associated Universities, Inc. Washington DC, USA.
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
# 
# Correspondence concerning GBT software should be addressed as follows:
#	GBT Operations
#	National Radio Astronomy Observatory
#	P. O. Box 2
#	Green Bank, WV 24944-0002 USA

"""
Provides a serial interface to the Valon 500x.
"""

# Python modules
import struct
# Third party modules
import serial
# Other
from valon_registers import *


__author__ = "Patrick Brandt"
__copyright__ = "Copyright 2011, Associated Universities, Inc."
__credits__ = ["Patrick Brandt, Stewart Rumley, Steven Stark"]
__license__ = "GPL"
#__version__ = "1.0"
__maintainer__ = "Patrick Brandt"


# Handy aliases
SYNTH_A = 0x00
SYNTH_B = 0x08

INT_REF = 0x00
EXT_REF = 0x01

ACK = 0x06
NACK = 0x15

class Synthesizer:
    def __init__(self, port):
        self.conn = serial.Serial(None, 9600, serial.EIGHTBITS,
                                  serial.PARITY_NONE, serial.STOPBITS_ONE)
        self.conn.setPort(port)

    def _generate_checksum(self, bytes):
        return chr(sum([ord(b) for b in bytes]) % 256)

    def _verify_checksum(self, bytes, checksum):
        return (self._generate_checksum(bytes) == checksum)

    def _write_freq_registers(self, synth, ncount, frac, mod, dbf):
        dbf_table = {1: 0, 2: 1, 4: 2, 8: 3, 16: 4}
        reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        reg0.ncount = ncount
        reg0.frac = frac
        reg1.mod = mod
        reg4.divider_select = dbf_table.get(dbf, 0)
        return self._set_all_registers(synth, reg1, reg2, reg3, reg4, reg5)

    def _read_freq_registers(self, synth):
        dbf_rev_table = {0: 1, 1: 2, 2: 4, 3: 8, 4: 16}
        reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        ncount = reg0.ncount
        frac = reg0.frac
        mod = reg1.mod
        dbf = dbf_rev_table.get(reg4.divider_select, 1)
        return ncount, frac, mod, dbf

    def _get_all_registers(self, synth):
        """Gets the values of ALL configuration registers on the sythesizer."""
        self.conn.open()
        bytes = struct.pack('>B', 0x80 | synth)
        self.conn.write(bytes)
        bytes = self.conn.read(24)
        checksum = self.conn.read(1)
        self.conn.close()
        # The only way to notify that this has failed is an exception
        if not self._verify_checksum(bytes, checksum):
            raise Exception("Checksum was not valid")
        r0 = reg0_t()
        r1 = reg1_t()
        r2 = reg2_t()
        r3 = reg3_t()
        r4 = reg4_t()
        r5 = reg5_t()
        (r0.asbyte, r1.asbyte, r2.asbyte,
         r3.asbyte, r4.asbyte, r5.asbyte) = struct.unpack('>IIIIII', bytes)
        return r0.reg, r1.reg, r2.reg, r3.reg, r4.reg, r5.reg

    def _set_all_registers(self, synth, reg0, reg1, reg2, reg3, reg4, reg5):
        """Sets the values of ALL configuration registers on the synthesizer.

        WARNING: This function will overwrite any existing settings in ALL
        registers.  It is intended to be used with 'get_all_registers'.
        Only use this if you know what you are doing!!!
        """
        r0 = reg0_t; r0.reg = reg0
        r1 = reg1_t; r1.reg = reg1
        r2 = reg2_t; r2.reg = reg2
        r3 = reg3_t; r3.reg = reg3
        r4 = reg4_t; r4.reg = reg4
        r5 = reg5_t; r5.reg = reg5
        bytes = struct.pack('>B6I', 0x00 | synth,
                            r0.asbyte, r1.asbyte, r2.asbyte,
                            r3.asbyte, r4.asbyte, r5.asbyte)
        checksum = self._generate_checksum(bytes)
        self.conn.open()
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def get_frequency(self, synth):
        """Returns the current output frequency for the selected synthesizer."""
        ncount, frac, mod, dbf = self._read_freq_registers(synth)
        EPDF = self._getEPDF(synth)
        return (ncount + float(frac) / mod) * EPDF / dbf

    def set_frequency(self, synth, freq, chan_spacing = 10.):
        """
        Sets the synthesizer to the desired frequency

        Sets to the closest possible frequency, depending on the channel spacing.
        Range is determined by the minimum and maximum VCO frequency.

        @param synth : synthesizer this command affects (0 for 1, 8 for 2).
        @type  synth : int

        @param freq : output frequency
        @type  freq : float

        @param chan_spacing : output frequency increment
        @type  chan_spacing : float

        @return: True if success (bool)
        """
        min, max = self.get_vco_range(synth)
        dbf = 1
        while (freq * dbf) <= min and dbf <= 16:
            dbf *= 2
        if dbf > 16:
            dbf = 16
        vco = freq * dbf
        EPDF = self._getEPDF(synth)
        ncount = int(vco / EPDF)
        frac = int((vco - ncount * float(EPDF)) / chan_spacing + 0.5)
        mod = int(EPDF / float(chan_spacing) + 0.5)
        if frac != 0 and mod != 0:
            while not (frac & 1) and not (mod & 1):
                frac /= 2
                mod /= 2
        else:
            frac = 0
            mod = 1
        return self._write_freq_registers(synth, ncount, frac, mod, dbf)

    def get_reference(self):
        """
        Get reference frequency in MHz
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x81)
        self.conn.write(bytes)
        bytes = self.conn.read(4)
        checksum = self.conn.read(1)
        self.conn.close()
        if not self._verify_checksum(bytes, checksum): return False
        freq = struct.unpack('>I', bytes)[0]
        return freq

    def set_reference(self, freq):
        """
        Set reference frequency in MHz

        @param freq : frequency in MHz
        @type  freq : float

        @return: True if success (bool)
        """
        self.conn.open()
        bytes = struct.pack('>BI', 0x01, freq)
        checksum = self._generate_checksum(bytes)
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def get_rf_level(self, synth):
        """
        Returns RF level in dBm

        @param synth : synthesizer address, 0 or 8
        @type  synth : int

        @return: dBm (int)
        """
        rfl_table = {0: -4, 1: -1, 2: 2, 3: 5}
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except:
            return False
        rfl = reg4.output_power
        rf_level = rfl_table.get(rfl)
        return rf_level

    def set_rf_level(self, synth, rf_level):
        """
        Set RF level

        @param synth : synthesizer address, 0 or 8
        @type  synth : int

        @param rf_level : RF power in dBm
        @type  rf_level : int

        @return: True if success (bool)
        """
        rfl_rev_table = {-4: 0, -1: 1, 2: 2, 5: 3}
        rfl = rfl_rev_table.get(rf_level)
        if(rfl is None): return False
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except:
            return False
        reg4.output_power = rfl
        return self._set_all_registers(synth, reg1, reg2, reg3, reg4, reg5)

    def get_options(self, synth):
        """
        Get options tuple:

        bool double:   if True, reference frequency is doubled
        bool half:     if True, reference frequency is halved
        int  r:        reference frequency divisor
        bool low_spur: if True, minimizes PLL spurs;
                       if False, minimizes phase noise
        double and half both True is same as both False.

        @param synth : synthesizer address

        @return: double (bool), half (bool), r (int), low_spur (bool)
        """
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except:
            return False
        return reg2.double_r, reg2.half_r, reg2.r, bool(reg2.low_spur)

    def set_options(self, synth, double = 0, half = 0, r = 1, low_spur = 0):
        """
        Set options.
        
        double and half both True is same as both False.

        @param synth : synthesizer base address
        @type  synth : int
        
        @param double : if 1, reference frequency is doubled; default 0
        @type  double : int
        
        @param half : if 1, reference frequency is halved; default 0
        @type  half : int
        
        @param r : reference frequency divisor; default 1
        @type  r : int
        
        @param low_spur : if 1, minimizes PLL spurs;
                          if 0, minimizes phase noise; default 0
        @type  low_spur : int

        @return: True if success (bool)
        """
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except:
            return False
        reg2.double_r = double
        reg2.half_r = half
        reg2.r = r
        reg2.low_spur = low_spur and 0x3 # Value must be either 0x0 or 0x3
        return self._set_all_registers(synth, reg1, reg2, reg3, reg4, reg5)

    def get_ref_select(self):
        """Returns the currently selected reference clock.

        Returns 1 if the external reference is selected, 0 otherwise.
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x86)
        self.conn.write(bytes)
        bytes = self.conn.read(1)
        checksum = self.conn.read(1)
        self.conn.close()
        if not self._verify_checksum(bytes, checksum): return False
        is_ext = struct.unpack('>B', bytes)[0]
        return is_ext & 1

    def set_ref_select(self, e_not_i = 1):
        """
        Selects either internal or external reference clock.

        @param e_not_i : 1 (external) or 0 (internal); default 1
        @type  e_not_i : int

        @return: True if success (bool)
        """
        self.conn.open()
        bytes = struct.pack('>BB', 0x06, e_not_i & 1)
        checksum = self._generate_checksum(bytes)
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def get_vco_range(self, synth):
        """
        Returns (min, max) VCO range tuple.

        @param synth : synthesizer base address
        @type  synth : int

        @return: min,max in MHz
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x83 | synth)
        self.conn.write(bytes)
        bytes = self.conn.read(4)
        checksum = self.conn.read(1)
        self.conn.close()
        if not self._verify_checksum(bytes, checksum): return False
        min, max = struct.unpack('>HH', bytes)
        return min, max

    def set_vco_range(self, synth, min, max):
        """
        Sets VCO range.

        @param synth : synthesizer base address
        @type  synth : int

        @param min : minimum VCO frequency
        @type  min : int

        @param max : maximum VCO frequency
        @type  max : int

        @return: True if success (bool)
        """
        self.conn.open()
        bytes = struct.pack('>BHH', 0x03 | synth, min, max)
        checksum = self._generate_checksum(bytes)
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def get_phase_lock(self, synth):
        """
        Get phase lock status

        @param synth : synthesizer base address
        @type  synth : int

        @return: True if locked (bool)
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x86 | synth)
        self.conn.write(bytes)
        bytes = self.conn.read(1)
        checksum = self.conn.read(1)
        self.conn.close()
        if not self._verify_checksum(bytes, checksum): return False
        mask = (synth << 1) or 0x20
        lock = struct.unpack('>B', bytes)[0] & mask
        return lock > 0

    def get_label(self, synth):
        """
        Get synthesizer label or name

        @param synth : synthesizer base address
        @type  synth : int

        @return: str
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x82 | synth)
        self.conn.write(bytes)
        bytes = self.conn.read(16)
        checksum = self.conn.read(1)
        self.conn.close()
        if not self._verify_checksum(bytes, checksum): return False
        return bytes

    def set_label(self, synth, label):
        """
        Set synthesizer label or name

        @param synth : synthesizer base address
        @type  synth : int

        @param label : up to 16 bytes of text
        @type  label : str
        
        @return: True if success (bool)
        """
        self.conn.open()
        bytes = struct.pack('>B16s', 0x02 | synth, label)
        checksum = self._generate_checksum(bytes)
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def flash(self):
        """
        Flash current settings for both synthesizers into non-volatile memory.

        @return: True if success (bool)
        """
        self.conn.open()
        bytes = struct.pack('>B', 0x40)
        checksum = self._generate_checksum(bytes)
        self.conn.write(bytes + checksum)
        bytes = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', bytes)[0]
        return ack == ACK

    def _getEPDF(self, synth):
        """
        Returns effective phase detector frequency.

        This is the reference frequency with options applied.
        """
        reference = self.get_reference() / 1e6
        double, half, r, low_spur = self.get_options(synth)
        if(double): reference *= 2.0
        if(half):   reference /= 2.0
        if(r > 1):  reference /= r
        return reference
