#ifndef VALONREGISTERS_H
#define VALONREGISTERS_H

#include "BitField.h"

union reg0_t {
    uint32_t asbyte;

    BitField<uint32_t,  0,  3> control;
    BitField<uint32_t,  3, 12> frac;
    BitField<uint32_t, 15, 16> ncount;
  //BitField<uint32_t, 31, 1> reserved;
};

union reg1_t {
    uint32_t asbyte;

    BitField<uint32_t,  0,  3> control;
    BitField<uint32_t,  3, 12> mod;
    BitField<uint32_t, 15, 12> phase;
    BitField<uint32_t, 27,  1> prescaler;
  //BitField<uint32_t, 28,  4> reserved;
};

union reg2_t {
    uint32_t asbyte;

    BitField<uint32_t,  0,  3> control;
    BitField<uint32_t,  3,  1> counter_reset;
    BitField<uint32_t,  4,  1> cp_three_state;
    BitField<uint32_t,  5,  1> pd;
    BitField<uint32_t,  6,  1> pd_polarity;
    BitField<uint32_t,  7,  1> ldp;
    BitField<uint32_t,  8,  1> ldf;
    BitField<uint32_t,  9,  4> charge_pump;
    BitField<uint32_t, 13,  1> double_buffer;
    BitField<uint32_t, 14, 10> r;
    BitField<uint32_t, 24,  1> half_r;
    BitField<uint32_t, 25,  1> double_r;
    BitField<uint32_t, 26,  3> muxout;
    BitField<uint32_t, 29,  2> low_spur;
  //BitField<uint32_t, 31,  1> reserved;
};

union reg3_t {
    uint32_t asbyte;

    BitField<uint32_t,  0,  3> control;
    BitField<uint32_t,  3, 12> clock_div;
    BitField<uint32_t, 15,  2> clock_div_mode;
  //BitField<uint32_t, 17, 1> reserved;
    BitField<uint32_t, 18,  1> csr;
  //BitField<uint32_t, 19, 13> reserved;
};

union reg4_t {
    uint32_t asbyte;

    BitField<uint32_t,  0, 3> control;
    BitField<uint32_t,  3, 2> output_power;
    BitField<uint32_t,  5, 1> rf_output_enable;
    BitField<uint32_t,  6, 2> aux_output_power;
    BitField<uint32_t,  8, 1> aux_output_enable;
    BitField<uint32_t,  9, 1> aux_output_select;
    BitField<uint32_t, 10, 1> mtld;
    BitField<uint32_t, 11, 1> vco_power_down;
    BitField<uint32_t, 12, 8> band_select_clock_div;
    BitField<uint32_t, 20, 3> divider_select;
    BitField<uint32_t, 23, 1> feedback_select;
  //BitField<uint32_t, 24, 8> reserved;
};

union reg5_t {
    uint32_t asbyte;

    BitField<uint32_t,  0,  3> control;
  //BitField<uint32_t,  3, 19> reserved;
    BitField<uint32_t, 22,  2> ld_pin_mode;
  //BitField<uint32_t, 24,  8> reserved;
};

#endif//VALONREGISTERS_H
