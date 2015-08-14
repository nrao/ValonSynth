#ifndef VALONREGISTERS_H
#define VALONREGISTERS_H

#include "BitField.h"

union reg0_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0,  3> control;
    BitField<std::uint32_t,  3, 12> frac;
    BitField<std::uint32_t, 15, 16> ncount;
  //BitField<std::uint32_t, 31,  1> reserved;
};

union reg1_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0,  3> control;
    BitField<std::uint32_t,  3, 12> mod;
    BitField<std::uint32_t, 15, 12> phase;
    BitField<std::uint32_t, 27,  1> prescaler;
  //BitField<std::uint32_t, 28,  4> reserved;
};

union reg2_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0,  3> control;
    BitField<std::uint32_t,  3,  1> counter_reset;
    BitField<std::uint32_t,  4,  1> cp_three_state;
    BitField<std::uint32_t,  5,  1> pd;
    BitField<std::uint32_t,  6,  1> pd_polarity;
    BitField<std::uint32_t,  7,  1> ldp;
    BitField<std::uint32_t,  8,  1> ldf;
    BitField<std::uint32_t,  9,  4> charge_pump;
    BitField<std::uint32_t, 13,  1> double_buffer;
    BitField<std::uint32_t, 14, 10> r;
    BitField<std::uint32_t, 24,  1> half_r;
    BitField<std::uint32_t, 25,  1> double_r;
    BitField<std::uint32_t, 26,  3> muxout;
    BitField<std::uint32_t, 29,  2> low_spur;
  //BitField<std::uint32_t, 31,  1> reserved;
};

union reg3_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0,  3> control;
    BitField<std::uint32_t,  3, 12> clock_div;
    BitField<std::uint32_t, 15,  2> clock_div_mode;
  //BitField<std::uint32_t, 17,  1> reserved;
    BitField<std::uint32_t, 18,  1> csr;
  //BitField<std::uint32_t, 19, 13> reserved;
};

union reg4_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0, 3> control;
    BitField<std::uint32_t,  3, 2> output_power;
    BitField<std::uint32_t,  5, 1> rf_output_enable;
    BitField<std::uint32_t,  6, 2> aux_output_power;
    BitField<std::uint32_t,  8, 1> aux_output_enable;
    BitField<std::uint32_t,  9, 1> aux_output_select;
    BitField<std::uint32_t, 10, 1> mtld;
    BitField<std::uint32_t, 11, 1> vco_power_down;
    BitField<std::uint32_t, 12, 8> band_select_clock_div;
    BitField<std::uint32_t, 20, 3> divider_select;
    BitField<std::uint32_t, 23, 1> feedback_select;
  //BitField<std::uint32_t, 24, 8> reserved;
};

union reg5_t {
    std::uint32_t asbyte;

    BitField<std::uint32_t,  0,  3> control;
  //BitField<std::uint32_t,  3, 19> reserved;
    BitField<std::uint32_t, 22,  2> ld_pin_mode;
  //BitField<std::uint32_t, 24,  8> reserved;
};

#endif//VALONREGISTERS_H
