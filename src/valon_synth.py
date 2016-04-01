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

def _pack_freq_registers(ncount, frac, mod, dbf, old_data):
    "Do bit packing for the frequency setting registers."
    dbf_table = {1: 0, 2: 1, 4: 2, 8: 3, 16: 4}
    reg0, reg1, reg2, reg3, reg4, reg5 = struct.unpack('>IIIIII', old_data)
    reg0 &= 0x80000007
    reg0 |= ((ncount & 0xffff) << 15) | ((frac & 0x0fff) << 3)
    reg1 &= 0xffff8007
    reg1 |= (mod & 0x0fff) << 3
    reg4 &= 0xff8fffff
    reg4 |= (dbf_table.get(dbf, 0)) << 20
    return struct.pack('>IIIIII', reg0, reg1, reg2, reg3, reg4, reg5)

def _unpack_freq_registers(data):
    "Do bit unpacking for the frequency setting registers."
    dbf_rev_table = {0: 1, 1: 2, 2: 4, 3: 8, 4: 16}
    reg0, reg1, _, _, reg4, _ = struct.unpack('>IIIIII', data)
    ncount = (reg0 >> 15) & 0xffff
    frac = (reg0 >> 3) & 0x0fff
    mod = (reg1 >> 3) & 0x0fff
    dbf = dbf_rev_table.get((reg4 >> 20) & 0x07, 1)
    return ncount, frac, mod, dbf

class Synthesizer:
    """A simple interface to the Valon 500x synthesizer."""
    def __init__(self, port):
        self.conn = serial.Serial(None, 9600, serial.EIGHTBITS,
                                  serial.PARITY_NONE, serial.STOPBITS_ONE)
        self.conn.setPort(port)

    def get_frequency(self, synth):
        """
        Returns the current output frequency for the selected synthesizer.

        @param synth : synthesizer this command affects (0 for 1, 8 for 2).
        @type  synth : int

        @return: the frequency in MHz (float)
        """
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        self.conn.close()
        #_verify_checksum(data, checksum)
        ncount, frac, mod, dbf = _unpack_freq_registers(data)
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
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        old_data = self.conn.read(24)
        checksum = self.conn.read(1)
        #_verify_checksum(old_data, checksum)
        data = struct.pack('>B24s', 0x00 | synth,
                           _pack_freq_registers(ncount, frac, mod,
                                                dbf, old_data))
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

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
        #_verify_checksum(data, checksum)
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
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        self.conn.close()
        #_verify_checksum(data, checksum)
        _, _, _, _, reg4, _ = struct.unpack('>IIIIII', data)
        rfl = (reg4 >> 3) & 0x03
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
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        #_verify_checksum(data, checksum)
        reg0, reg1, reg2, reg3, reg4, reg5 = struct.unpack('>IIIIII', data)
        reg4 &= 0xffffffe7
        reg4 |= (rfl & 0x03) << 3
        data = struct.pack('>BIIIIII', 0x00 | synth,
                            reg0, reg1, reg2, reg3, reg4, reg5)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

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
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        self.conn.close()
        #_verify_checksum(data, checksum)
        _, _, reg2, _, _, _ = struct.unpack('>IIIIII', data)
        low_spur = ((reg2 >> 30) & 1) & ((reg2 >> 29) & 1)
        double = (reg2 >> 25) & 1
        half = (reg2 >> 24) & 1
        divider = (reg2 >> 14) & 0x03ff
        return double, half, divider, low_spur

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
        self.conn.open()
        data = struct.pack('>B', 0x80 | synth)
        self.conn.write(data)
        data = self.conn.read(24)
        checksum = self.conn.read(1)
        #_verify_checksum(data, checksum)
        reg0, reg1, reg2, reg3, reg4, reg5 = struct.unpack('>IIIIII', data)
        reg2 &= 0x9c003fff
        reg2 |= (((low_spur & 1) << 30) | ((low_spur & 1) << 29) |
                 ((double & 1) << 25) | ((half & 1) << 24) |
                 ((divider & 0x03ff) << 14))
        data = struct.pack('>BIIIIII', 0x00 | synth,
                            reg0, reg1, reg2, reg3, reg4, reg5)
        checksum = _generate_checksum(data)
        self.conn.write(data + checksum)
        data = self.conn.read(1)
        self.conn.close()
        ack = struct.unpack('>B', data)[0]
        return ack == ACK

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
        #_verify_checksum(data, checksum)
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
        #_verify_checksum(data, checksum)
        return struct.unpack('>HH', data)

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
        #_verify_checksum(data, checksum)
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
        #_verify_checksum(data, checksum)
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
