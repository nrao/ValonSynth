import ctypes

class register0_t(ctypes.LittleEndianStructure):
    _fields_ = [
        ('control', ctypes.c_uint32,  3),
        ('frac',    ctypes.c_uint32, 12),
        ('ncount',  ctypes.c_uint32, 16),
        ('',        ctypes.c_uint32,  1)
        ]

class register1_t(ctypes.LittleEndianStructure):
    _fields_ = [
        ('control',   ctypes.c_uint32,  3),
        ('mod',       ctypes.c_uint32, 12),
        ('phase',     ctypes.c_uint32, 12),
        ('prescaler', ctypes.c_uint32,  1),
        ('',          ctypes.c_uint32,  4),
        ]

class register2_t(ctypes.LittleEndianStructure):
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

class register3_t(ctypes.LittleEndianStructure):
    _fields_ = [
        ('control',        ctypes.c_uint32,  3),
        ('clock_div',      ctypes.c_uint32, 12),
        ('clock_div_mode', ctypes.c_uint32,  2),
        ('',               ctypes.c_uint32,  1),
        ('csr',            ctypes.c_uint32,  1),
        ('',               ctypes.c_uint32, 13),
        ]

class register4_t(ctypes.LittleEndianStructure):
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

class register5_t(ctypes.LittleEndianStructure):
    _fields_ = [
        ('control',     ctypes.c_uint32,  3),
        ('',            ctypes.c_uint32, 19),
        ('ld_pin_mode', ctypes.c_uint32,  2),
        ('',            ctypes.c_uint32,  8),
        ]

class reg0_t(ctypes.Union):
    _fields_ = [
        ('reg', register0_t),
        ('asbyte', ctypes.c_uint32)
        ]

class reg1_t(ctypes.Union):
    _fields_ = [
        ('reg', register1_t),
        ('asbyte', ctypes.c_uint32)
        ]

class reg2_t(ctypes.Union):
    _fields_ = [
        ('reg', register2_t),
        ('asbyte', ctypes.c_uint32)
        ]

class reg3_t(ctypes.Union):
    _fields_ = [
        ('reg', register3_t),
        ('asbyte', ctypes.c_uint32)
        ]

class reg4_t(ctypes.Union):
    _fields_ = [
        ('reg', register4_t),
        ('asbyte', ctypes.c_uint32)
        ]

class reg5_t(ctypes.Union):
    _fields_ = [
        ('reg', register5_t),
        ('asbyte', ctypes.c_uint32)
        ]
