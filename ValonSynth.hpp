#ifndef VALONSYNTH_HPP
#define VALONSYNTH_HPP

// STL
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
// ValonSynth
#include "ValonRegisters.hpp"

namespace Valon {

enum class Response : std::uint8_t { ACK = 0x06, NACK = 0x15 };
enum class SynthID : std::uint8_t { A = 0x00, B = 0x08 };

namespace detail {
// These types are meant to be unpacked using std::tie,
// thus why they are hidden.
using FrequencyVars_t = std::tuple<std::uint32_t/*ncount*/,
                                   std::uint32_t/*frac*/,
                                   std::uint32_t/*mod*/,
                                   std::uint32_t/*dbf*/>;
using Options_t = std::tuple<bool/*double_ref*/, bool/*half_ref*/,
                             std::uint32_t/*divider*/, bool/*low_spur*/>;
using VcoRange_t = std::tuple<std::uint16_t/*low*/, std::uint16_t/*high*/>;
using Registers_t = std::tuple<Register0_t, Register1_t, Register2_t,
                               Register3_t, Register4_t, Register5_t>;

} // namespace detail

template <typename INTERFACE>
class Synth {
public:
    template <typename ...Args>
    Synth(Args&& ...args);
    Synth(std::unique_ptr<INTERFACE> interface);
    Synth() = delete;
    ~Synth() = default;

    void flashSettings();

    float getFrequency(SynthID id) const;
    std::string getLabel(SynthID id) const;
    detail::Options_t getOptions(SynthID id) const;
    float getReferenceFrequency() const;
    std::int32_t getRfLevel(SynthID id) const;
    detail::VcoRange_t getVcoRange(SynthID id) const;
    bool isPhaseLocked(SynthID id) const;
    bool isReferenceExternal() const;

    void setFrequency(SynthID id, float megahertz,
                      float channel_spacing = 10.0f);
    void setLabel(SynthID id, const std::string &label);
    void setOptions(SynthID id, bool double_ref, bool half_ref,
                    std::uint32_t divider, bool low_spur);
    void setReferenceExternal(bool is_external);
    void setReferenceFrequency(float hertz);
    void setRfLevel(SynthID id, std::int32_t rf_level);
    void setVcoRange(SynthID id, std::uint16_t low, std::uint16_t high);

protected:
    // Get the effective phase detector frequency
    float getEPDF(SynthID id) const;

    // High level utilities to get/set the frequency registers
    detail::FrequencyVars_t readFrequencyRegisters(SynthID id) const;
    void writeFrequencyRegisters(SynthID id,
                                 std::uint32_t ncount, std::uint32_t frac,
                                 std::uint32_t mod, std::uint32_t dbf);

    // Mid level utilities for controlling configuration registers directly
    detail::Registers_t getAllRegisters(SynthID id) const;
    void setAllRegisters(SynthID id,
                         const Register0_t &reg0, const Register1_t &reg1,
                         const Register2_t &reg2, const Register3_t &reg3,
                         const Register4_t &reg4, const Register5_t &reg5);

    // Lowest level utility to read and write values to the Valon
    template <typename ...Ts>
    void readWithChecksum(std::uint8_t address, Ts... ts) const;
    template <typename ...Ts>
    void writeWithAcknowledge(std::uint8_t address, Ts... ts);

private:
    mutable std::unique_ptr<INTERFACE> m_interface;
};

// Example usage:
//using SerialSynth = Synth<boost::asio::serial_port>;
//using EthernetSynth = Synth<boost::asio::ip::tcp::socket>;

} // namespace Valon

#include "ValonSynth.inl"

#endif//VALONSYNTH_HPP
