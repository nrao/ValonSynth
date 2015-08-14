struct register0_t {
    uint32_t control :  3;
    uint32_t frac    : 12;
    uint32_t ncount  : 16;
    uint32_t         :  1;
};

struct register1_t {
    uint32_t control   :  3;
    uint32_t mod       : 12;
    uint32_t phase     : 12;
    uint32_t prescaler :  1;
    uint32_t           :  4;
};

struct register2_t {
    uint32_t : ;
    uint32_t : ;
    uint32_t : ;
    uint32_t : ;
'control',        ctypes.c_uint32,  3),
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
}

struct register3_t {

}

struct register4_t {

}

struct register5_t {

}
