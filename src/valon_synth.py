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
from valon_registers import Reg0, Reg1, Reg2, Reg3, Reg4, Reg5


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

def _generate_checksum(data):
    "Generate a checksum for the data provided."
    return chr(sum([ord(b) for b in data]) % 256)

def _verify_checksum(data, checksum):
    "Verify a checksum for the data provided."
    return (_generate_checksum(data) == checksum)

class Synthesizer:
    """A simple interface to the Valon 500x synthesizer."""
    def __init__(self, port):
        self.conn = serial.Serial(None, 9600, serial.EIGHTBITS,
                                  serial.PARITY_NONE, serial.STOPBITS_ONE)
        self.conn.setPort(port)

    def _write_freq_registers(self, synth, ncount, frac, mod, dbf):
        "Write register values responsible for setting the frequency."
        dbf_table = {1: 0, 2: 1, 4: 2, 8: 3, 16: 4}
        reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        reg0.ncount = ncount
        reg0.frac = frac
        reg1.mod = mod
        reg4.divider_select = dbf_table.get(dbf, 0)
        return self._set_all_registers(synth, reg0, reg1, reg2,
                                       reg3, reg4, reg5)

    def _read_freq_registers(self, synth):
        "Read register values responsible for setting the frequency."
        dbf_rev_table = {0: 1, 1: 2, 2: 4, 3: 8, 4: 16}
        reg0, reg1, _, _, reg4, _ = self._get_all_registers(synth)
        ncount = reg0.ncount
        frac = reg0.frac
        mod = reg1.mod
        dbf = dbf_rev_table.get(reg4.divider_select, 1)
        return ncount, frac, mod, dbf

    def _get_all_registers(self, synth):
        """Gets the values of ALL configuration registers on the sythesizer."""
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        self.conn.close()
        # The only way to notify that this has failed is an exception
        if not _verify_checksum(data, checksum):
            raise ValueError("Checksum was not valid")
        reg0 = Reg0()
        reg1 = Reg1()
        reg2 = Reg2()
        reg3 = Reg3()
        reg4 = Reg4()
        reg5 = Reg5()
        (reg0.asbyte, reg1.asbyte, reg2.asbyte,
         reg3.asbyte, reg4.asbyte, reg5.asbyte) = struct.unpack('>IIIIII', data)
        return reg0.reg, reg1.reg, reg2.reg, reg3.reg, reg4.reg, reg5.reg

    def _set_all_registers(self, synth, register0, register1, register2,
                           register3, register4, register5):
        """Sets the values of ALL configuration registers on the synthesizer.

        WARNING: This function will overwrite any existing settings in ALL
        registers.  It is intended to be used with 'get_all_registers'.
        Only use this if you know what you are doing!!!
        """
        reg0 = Reg0()
        reg1 = Reg1()
        reg2 = Reg2()
        reg3 = Reg3()
        reg4 = Reg4()
        reg5 = Reg5()
        reg0.reg = register0
        reg1.reg = register1
        reg2.reg = register2
        reg3.reg = register3
        reg4.reg = register4
        reg5.reg = register5
        data = struct.pack('>B6I', 0x00 | synth,
                            reg0.asbyte, reg1.asbyte, reg2.asbyte,
                            reg3.asbyte, reg4.asbyte, reg5.asbyte)
        checksum = _generate_checksum(data)
        self.conn.open()
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

    def get_frequency(self, synth):
        """Returns the current output frequency for the selected synthesizer."""
        ncount, frac, mod, dbf = self._read_freq_registers(synth)
        epdf = self._get_epdf(synth)
        return (ncount + float(frac) / mod) * epdf / dbf

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
        low, _ = self.get_vco_range(synth)
        dbf = 1
        while (freq * dbf) <= low and dbf <= 16:
            dbf *= 2
        if dbf > 16:
            dbf = 16
        vco = freq * dbf
        epdf = self._get_epdf(synth)
        ncount = int(vco / epdf)
        frac = int((vco - ncount * float(epdf)) / chan_spacing + 0.5)
        mod = int(epdf / float(chan_spacing) + 0.5)
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
        data = struct.pack('>B', 0x81)
        self.conn.write(data)
        data = self.conn.read(4)
        checksum = self.conn.read(1)
        self.conn.close()
        if not _verify_checksum(data, checksum):
            return False
        freq = struct.unpack('>I', data)[0]
        return freq

    def set_reference(self, freq):
        """
        Set reference frequency in MHz

        @param freq : frequency in MHz
        @type  freq : float

        @return: True if success (bool)
        """
        self.conn.open()
        data = struct.pack('>BI', 0x01, freq)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
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
            _, _, _, _, reg4, _ = self._get_all_registers(synth)
        except ValueError:
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
        if(rfl is None):
            return False
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except ValueError:
            return False
        reg4.output_power = rfl
        return self._set_all_registers(synth, reg0, reg1, reg2,
                                       reg3, reg4, reg5)

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
            _, _, reg2, _, _, _ = self._get_all_registers(synth)
        except ValueError:
            return False
        return reg2.double_r, reg2.half_r, reg2.r, bool(reg2.low_spur)

    def set_options(self, synth, double = 0, half = 0, divider = 1,
                    low_spur = 0):
        """
        Set options.

        double and half both True is same as both False.

        @param synth : synthesizer base address
        @type  synth : int

        @param double : if 1, reference frequency is doubled; default 0
        @type  double : int

        @param half : if 1, reference frequency is halved; default 0
        @type  half : int

        @param divider : reference frequency divisor; default 1
        @type  divider : int

        @param low_spur : if 1, minimizes PLL spurs;
                          if 0, minimizes phase noise; default 0
        @type  low_spur : int

        @return: True if success (bool)
        """
        try:
            reg0, reg1, reg2, reg3, reg4, reg5 = self._get_all_registers(synth)
        except ValueError:
            return False
        reg2.double_r = double
        reg2.half_r = half
        reg2.r = divider
        reg2.low_spur = low_spur and 0x3 # Value must be either 0x0 or 0x3
        return self._set_all_registers(synth, reg0, reg1, reg2, reg3, reg4, reg5)

    def get_ref_select(self):
        """Returns the currently selected reference clock.

        Returns 1 if the external reference is selected, 0 otherwise.
        """
        self.conn.open()
        data = struct.pack('>B', 0x86)
        self.conn.write(data)
        data = self.conn.read(1)
        checksum = self.conn.read(1)
        self.conn.close()
        if not _verify_checksum(data, checksum):
            return False
        is_ext = struct.unpack('>B', data)[0]
        return is_ext & 1

    def set_ref_select(self, e_not_i = 1):
        """
        Selects either internal or external reference clock.

        @param e_not_i : 1 (external) or 0 (internal); default 1
        @type  e_not_i : int

        @return: True if success (bool)
        """
        self.conn.open()
        data = struct.pack('>BB', 0x06, e_not_i & 1)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

    def get_vco_range(self, synth):
        """
        Returns (min, max) VCO range tuple.

        @param synth : synthesizer base address
        @type  synth : int

        @return: min,max in MHz
        """
        self.conn.open()
        data = struct.pack('>B', 0x83 | synth)
        self.conn.write(data)
        data = self.conn.read(4)
        checksum = self.conn.read(1)
        self.conn.close()
        if not _verify_checksum(data, checksum):
            return False
        low, high = struct.unpack('>HH', data)
        return low, high

    def set_vco_range(self, synth, low, high):
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
        data = struct.pack('>BHH', 0x03 | synth, low, high)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

    def get_phase_lock(self, synth):
        """
        Get phase lock status

        @param synth : synthesizer base address
        @type  synth : int

        @return: True if locked (bool)
        """
        self.conn.open()
        data = struct.pack('>B', 0x86 | synth)
        self.conn.write(data)
        data = self.conn.read(1)
        checksum = self.conn.read(1)
        self.conn.close()
        if not _verify_checksum(data, checksum):
            return False
        mask = (synth << 1) or 0x20
        lock = struct.unpack('>B', data)[0] & mask
        return lock > 0

    def get_label(self, synth):
        """
        Get synthesizer label or name

        @param synth : synthesizer base address
        @type  synth : int

        @return: str
        """
        self.conn.open()
        data = struct.pack('>B', 0x82 | synth)
        self.conn.write(data)
        data = self.conn.read(16)
        checksum = self.conn.read(1)
        self.conn.close()
        if not _verify_checksum(data, checksum):
            return False
        return data

    def set_label(self, synth, label):
        """
        Set synthesizer label or name

        @param synth : synthesizer base address
        @type  synth : int

        @param label : up to 16 data of text
        @type  label : str
        
        @return: True if success (bool)
        """
        self.conn.open()
        data = struct.pack('>B16s', 0x02 | synth, label)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

    def flash(self):
        """
        Flash current settings for both synthesizers into non-volatile memory.

        @return: True if success (bool)
        """
        self.conn.open()
        data = struct.pack('>B', 0x40)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

    def _get_epdf(self, synth):
        """
        Returns effective phase detector frequency.

        This is the reference frequency with options applied.
        """
        reference = self.get_reference() / 1e6
        double, half, divider, _ = self.get_options(synth)
        if(double):
            reference *= 2.0
        if(half):
            reference /= 2.0
        if(divider > 1):
            reference /= divider
        return reference
