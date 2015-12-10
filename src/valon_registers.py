"""
Union-like structures to represent registers.
Intended to make bit-packing easier to grok.
"""

import ctypes

class Register0(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 0."
    _fields_ = [
        ('control', ctypes.c_uint32,  3),
        ('frac',    ctypes.c_uint32, 12),
        ('ncount',  ctypes.c_uint32, 16),
        ('',        ctypes.c_uint32,  1)
        ]

class Register1(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 1."
    _fields_ = [
        ('control',   ctypes.c_uint32,  3),
        ('mod',       ctypes.c_uint32, 12),
        ('phase',     ctypes.c_uint32, 12),
        ('prescaler', ctypes.c_uint32,  1),
        ('',          ctypes.c_uint32,  4),
        ]

class Register2(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 2."
    _fields_ = [
        ('control',        ctypes.c_uint32,  3),
        ('counter_reset',  ctypes.c_uint32,  1),
        ('cp_three_state', ctypes.c_uint32,  1),
        ('pd',             ctypes.c_uint32,  1),
        ('pd_polarity',    ctypes.c_uint32,  1),
        ('ldp',            ctypes.c_uint32,  1),
        ('ldf',            ctypes.c_uint32,  1),
        ('charge_pump',    ctypes.c_uint32,  4),
        ('double_buff',    ctypes.c_uint32,  1),
        ('r',              ctypes.c_uint32, 10),
        ('half_r',         ctypes.c_uint32,  1),
        ('double_r',       ctypes.c_uint32,  1),
        ('muxout',         ctypes.c_uint32,  3),
        ('low_spur',       ctypes.c_uint32,  2),
        ('',               ctypes.c_uint32,  1),
        ]

class Register3(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 3."
    _fields_ = [
        ('control',        ctypes.c_uint32,  3),
        ('clock_div',      ctypes.c_uint32, 12),
        ('clock_div_mode', ctypes.c_uint32,  2),
        ('',               ctypes.c_uint32,  1),
        ('csr',            ctypes.c_uint32,  1),
        ('',               ctypes.c_uint32, 13),
        ]

class Register4(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 4."
    _fields_ = [
        ('control',               ctypes.c_uint32, 3),
        ('output_power',          ctypes.c_uint32, 2),
        ('rf_output_enable',      ctypes.c_uint32, 1),
        ('aux_output_power',      ctypes.c_uint32, 2),
        ('aux_output_enable',     ctypes.c_uint32, 1),
        ('aux_output_select',     ctypes.c_uint32, 1),
        ('mtld',                  ctypes.c_uint32, 1),
        ('vco_power_down',        ctypes.c_uint32, 1),
        ('band_select_clock_div', ctypes.c_uint32, 8),
        ('divider_select',        ctypes.c_uint32, 3),
        ('feedback_select',       ctypes.c_uint32, 1),
        ('',                      ctypes.c_uint32, 8),
        ]

class Register5(ctypes.LittleEndianStructure):
    "Structure to hold fields of the Valon register 5."
    _fields_ = [
        ('control',     ctypes.c_uint32,  3),
        ('',            ctypes.c_uint32, 19),
        ('ld_pin_mode', ctypes.c_uint32,  2),
        ('',            ctypes.c_uint32,  8),
        ]

class Reg0(ctypes.Union):
    "Union to alias Register0 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register0),
        ('asbyte', ctypes.c_uint32)
        ]

class Reg1(ctypes.Union):
    "Union to alias Register1 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register1),
        ('asbyte', ctypes.c_uint32)
        ]

class Reg2(ctypes.Union):
    "Union to alias Register2 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register2),
        ('asbyte', ctypes.c_uint32)
        ]

class Reg3(ctypes.Union):
    "Union to alias Register3 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register3),
        ('asbyte', ctypes.c_uint32)
        ]

class Reg4(ctypes.Union):
    "Union to alias Register4 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register4),
        ('asbyte', ctypes.c_uint32)
        ]

class Reg5(ctypes.Union):
    "Union to alias Register5 type as an endian-aware integer."
    _fields_ = [
        ('reg', Register5),
        ('asbyte', ctypes.c_uint32)
        ]
