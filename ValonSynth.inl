// STL
// Boost
#include <boost/asio.hpp>

// Utility to calculate greatest common divisor
std::uint32_t gcd(std::uint32_t a, std::uint32_t b) {
    std::uint32_t r;
    while(b != 0) {
        r = a % b;
        a = b;
        b = r;
    }
    return a;
}

// Generate a checksum from a vector of boost buffers
template <typename buffer_type>
std::uint8_t generateChecksum(const std::vector<buffer_type> &data) {
    std::uint32_t sum = 0;
    for(auto &buffer : data) {
        std::uint8_t *bytes = boost::asio::buffer_cast<uint8_t*>(buffer);
        std::size_t n = boost::asio::buffer_size(buffer);
        for(std::size_t i = 0; i < n; ++i) {
            sum += bytes[i];
        }
    }
    return static_cast<std::uint8_t>(sum % 256);
}

template <typename buffer_type>
bool verifyChecksum(const std::vector<buffer_type> &data,
                    std::uint8_t checksum) {
    return (generateChecksum(data) == checksum);
}

template <typename INTERFACE>
template <typename ...Args>
Valon::Synth<INTERFACE>::Synth(Args&& ...args)
    : m_interface(std::make_unique<INTERFACE>(std::forward<Args>(args)...))
{}

template <typename INTERFACE>
Valon::Synth<INTERFACE>::Synth(std::unique_ptr<INTERFACE> interface)
    : m_interface(std::move(interface))
{}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::flashSettings() {
    std::uint8_t address = 0x40;
    writeWithAcknowledge(address);
}

template <typename INTERFACE>
float Valon::Synth<INTERFACE>::getFrequency(Valon::SynthID id) const {
    std::uint32_t ncount, frac, mod, dbf;
    std::tie(ncount, frac, mod, dbf) = readFrequencyRegisters(id);
    float epdf = getEPDF(id);
    return (ncount + float(frac) / mod) * epdf / dbf;
}

template <typename INTERFACE>
std::string Valon::Synth<INTERFACE>::getLabel(Valon::SynthID id) const {
    std::uint8_t address = 0x82 | static_cast<std::uint8_t>(id);
    char label[16];
    readWithChecksum(address, boost::asio::buffer(label, 16));
    return label;
}

template <typename INTERFACE>
Valon::detail::Options_t
Valon::Synth<INTERFACE>::getOptions(Valon::SynthID id) const {
    Register2_t reg2;
    std::tie(std::ignore, std::ignore, reg2,
             std::ignore, std::ignore, std::ignore) = getAllRegisters(id);
    bool double_ref = reg2.double_r;
    bool half_ref = reg2.half_r;
    std::uint32_t divider = reg2.r;
    bool low_spur = reg2.low_spur;
    std::make_tuple(double_ref, half_ref, divider, low_spur);
}

template <typename INTERFACE>
float Valon::Synth<INTERFACE>::getReferenceFrequency() const {
    std::uint8_t address = 0x81;
    std::uint32_t ref_frequency;
    readWithChecksum(address, ref_frequency);
    return ref_frequency;
}

template <typename INTERFACE>
std::int32_t Valon::Synth<INTERFACE>::getRfLevel(Valon::SynthID id) const {
    Register4_t reg4;
    std::tie(std::ignore, std::ignore, std::ignore,
             std::ignore, reg4, std::ignore) = getAllRegisters(id);
    std::int32_t rf_level = -4;
    switch(static_cast<std::uint32_t>(reg4.output_power)) {
    case 0: rf_level = -4; break;
    case 1: rf_level = -1; break;
    case 2: rf_level = 2; break;
    case 3: rf_level = 5; break;
    }
    return rf_level;
}

template <typename INTERFACE>
Valon::detail::VcoRange_t
Valon::Synth<INTERFACE>::getVcoRange(Valon::SynthID id) const {
    std::uint8_t address = 0x83 | static_cast<std::uint8_t>(id);
    std::uint16_t vco_low, vco_high;
    readWithChecksum(address, vco_low, vco_high);
    return std::make_tuple(vco_low, vco_high);
}

template <typename INTERFACE>
bool Valon::Synth<INTERFACE>::isPhaseLocked(Valon::SynthID id) const {
    std::uint8_t address = 0x86 | static_cast<std::uint8_t>(id);
    std::uint8_t phase_locked;
    readWithChecksum(address, phase_locked);
    // Mask off the correct bit to indicate the phase lock status
    std::uint8_t mask = (id == Valon::SynthID::A) ? 0x20 : 0x10;
    return phase_locked & mask;
}

template <typename INTERFACE>
bool Valon::Synth<INTERFACE>::isReferenceExternal() const {
    std::uint8_t address = 0x86;
    std::uint8_t ref_select;
    readWithChecksum(address, ref_select);
    return ref_select & 0x01;
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setFrequency(Valon::SynthID id, float megahertz,
                                           float channel_spacing) {
    std::uint32_t dbf = 1;
    std::uint16_t vco_low;
    std::tie(vco_low, std::ignore) = getVcoRange(id);
    while(((megahertz * dbf) <= vco_low) && (dbf < 16)) {
        dbf *= 2;
    }
    float vco = megahertz * dbf;
    float epdf = getEPDF(id);
    std::uint32_t ncount = static_cast<std::uint32_t>(vco / epdf);
    std::uint32_t frac = static_cast<std::uint32_t>(
        (vco - ncount * epdf) / channel_spacing + 0.5);
    std::uint32_t mod = static_cast<std::uint32_t>(epdf / channel_spacing + 0.5);
    // Reduce frac/mod to simplest fraction
    if((frac != 0) && (mod != 0)){
        std::uint32_t gcf = gcd(frac, mod);
        frac /= gcf;
        mod /= gcf;
    }
    else {
        frac = 0;
        mod = 1;
    }
    writeFrequencyRegisters(id, ncount, frac, mod, dbf);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setLabel(Valon::SynthID id,
                                       const std::string &label) {
    std::uint8_t address = 0x02 | static_cast<std::uint8_t>(id);
    std::string raw_label(label);
    raw_label.reserve(16);
    writeWithAcknowledge(address, boost::asio::buffer(raw_label.c_str(), 16));
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setOptions(Valon::SynthID id,
                                         bool double_ref, bool half_ref,
                                         std::uint32_t divider, bool low_spur) {
    Register0_t reg0;
    Register1_t reg1;
    Register2_t reg2;
    Register3_t reg3;
    Register4_t reg4;
    Register5_t reg5;
    std::tie(reg0, reg1, reg2, reg3, reg4, reg5) = getAllRegisters(id);
    reg2.double_r = double_ref;
    reg2.half_r = half_ref;
    reg2.r = divider;
    reg2.low_spur = low_spur ? 0x03 : 0x00;
    setAllRegisters(id, reg0, reg1, reg2, reg3, reg4, reg5);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setReferenceExternal(bool is_external) {
    std::uint8_t address = 0x06;
    std::uint8_t payload = is_external ? 0x01 : 0x00;
    writeWithAcknowledge(address, payload);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setReferenceFrequency(float hertz) {
    std::uint8_t address = 0x81;
    std::uint32_t ref_frequency = static_cast<std::uint32_t>(hertz);
    writeWithAcknowledge(address, ref_frequency);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setRfLevel(Valon::SynthID id, std::int32_t rf_level) {
    std::uint32_t raw_rfl;
    switch(rf_level) {
    case -4: raw_rfl = 0; break;
    case -1: raw_rfl = 1; break;
    case 2: raw_rfl = 2; break;
    case 5: raw_rfl = 3; break;
    default: ;// throw exception?
    }
    Register0_t reg0;
    Register1_t reg1;
    Register2_t reg2;
    Register3_t reg3;
    Register4_t reg4;
    Register5_t reg5;
    std::tie(reg0, reg1, reg2, reg3, reg4, reg5) = getAllRegisters(id);
    reg4.output_power = raw_rfl;
    setAllRegisters(id, reg0, reg1, reg2, reg3, reg4, reg5);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setVcoRange(Valon::SynthID id, std::uint16_t low, std::uint16_t high) {
    std::uint8_t address = 0x03 | static_cast<std::uint8_t>(id);
    writeWithAcknowledge(address, low, high);
}

template <typename INTERFACE>
float Valon::Synth<INTERFACE>::getEPDF(Valon::SynthID id) const {
    float reference = getReferenceFrequency() / 1e6;
    bool double_ref;
    bool half_ref;
    std::uint32_t divider;
    std::tie(double_ref, half_ref, divider, std::ignore) = getOptions(id);
    if(double_ref) {
        reference *= 2.0;
    }
    if(half_ref) {
        reference /= 2.0;
    }
    if(divider > 1) {
        reference /= divider;
    }
    return reference;
}

template <typename INTERFACE>
Valon::detail::Registers_t
Valon::Synth<INTERFACE>::getAllRegisters(Valon::SynthID id) const {
    std::uint8_t address = 0x80 | static_cast<std::uint8_t>(id);
    Register0_t reg0;
    Register1_t reg1;
    Register2_t reg2;
    Register3_t reg3;
    Register4_t reg4;
    Register5_t reg5;
    readWithChecksum(address, reg0.asbyte, reg1.asbyte, reg2.asbyte,
                     reg3.asbyte, reg4.asbyte, reg5.asbyte);
    return std::make_tuple(reg0, reg1, reg2, reg3, reg4, reg5);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::setAllRegisters(Valon::SynthID id,
                                              const Register0_t &reg0,
                                              const Register1_t &reg1,
                                              const Register2_t &reg2,
                                              const Register3_t &reg3,
                                              const Register4_t &reg4,
                                              const Register5_t &reg5) {
    std::uint8_t address = 0x00 | static_cast<std::uint8_t>(id);
    writeWithAcknowledge(address, reg0.asbyte, reg1.asbyte, reg2.asbyte,
                         reg3.asbyte, reg4.asbyte);
}

template <typename INTERFACE>
Valon::detail::FrequencyVars_t
Valon::Synth<INTERFACE>::readFrequencyRegisters(Valon::SynthID id) const {
    Register0_t reg0;
    Register1_t reg1;
    Register4_t reg4;
    std::tie(reg0, reg1, std::ignore, std::ignore, reg4, std::ignore) =
        getAllRegisters(id);
    std::uint32_t dbf = 1;
    switch(reg4.divider_select) {
    case 0: dbf = 1; break;
    case 1: dbf = 2; break;
    case 2: dbf = 4; break;
    case 3: dbf = 8; break;
    case 4: dbf = 16; break;
    }
    return std::make_tuple(reg0.ncount, reg0.frac, reg1.mod, dbf);
}

template <typename INTERFACE>
void Valon::Synth<INTERFACE>::writeFrequencyRegisters(Valon::SynthID id,
                                                      std::uint32_t ncount,
                                                      std::uint32_t frac,
                                                      std::uint32_t mod,
                                                      std::uint32_t dbf) {
    std::uint32_t raw_dbf = 0;
    switch(dbf) {
    case 1: raw_dbf = 0; break;
    case 2: raw_dbf = 1; break;
    case 4: raw_dbf = 2; break;
    case 8: raw_dbf = 3; break;
    case 16: raw_dbf = 4; break;
    }
    Register0_t reg0;
    Register1_t reg1;
    Register2_t reg2;
    Register3_t reg3;
    Register4_t reg4;
    Register5_t reg5;
    std::tie(reg0, reg1, reg2, reg3, reg4, reg5) = getAllRegisters(id);
    reg0.ncount = ncount;
    reg0.frac = frac;
    reg1.mod = mod;
    reg4.divider_select = raw_dbf;
    setAllRegisters(id, reg0, reg1, reg2, reg3, reg4, reg5);
}



template <typename buffer_type, typename T>
void buildBuffer(std::vector<buffer_type> &data, T variable) {
    data.push_back(boost::asio::buffer(variable));
}

// TODO: Figure out whether universal references are needed in these variadics
template <typename buffer_type, typename T, typename ...Ts>
void buildBuffer(std::vector<buffer_type> &data, T variable, Ts... ts) {
    data.push_back(boost::asio::buffer(variable));
    buildBuffer(data, ts...);
}

template <typename INTERFACE>
template <typename ...Ts>
void Valon::Synth<INTERFACE>::readWithChecksum(std::uint8_t address,
                                               Ts... ts) const {
    // Send the readback request
    boost::system::error_code ec;
    boost::asio::write(m_interface, boost::asio::buffer(&address, 1), ec);
    std::vector<boost::asio::mutable_buffer> data;
    buildBuffer(data, ts...);
    boost::asio::read(m_interface, data, ec);
    std::uint8_t checksum;
    boost::asio::read(m_interface, boost::asio::buffer(&checksum, 1), ec);
    if(ec) {
        // do something about the error
    }
    if(!verifyChecksum(data, checksum)) {
        // do something about the error
    }
}

template <typename INTERFACE>
template <typename ...Ts>
void Valon::Synth<INTERFACE>::writeWithAcknowledge(std::uint8_t address,
                                                   Ts... ts) {
    std::vector<boost::asio::const_buffer> data;
    buildBuffer(data, address, ts...);
    std::uint8_t checksum = generateChecksum(data);
    data.push_back(boost::asio::buffer(&checksum, 1));
    boost::system::error_code ec;
    boost::asio::write(m_interface, data, ec);
    std::uint8_t acknowledge = static_cast<std::uint8_t>(Valon::Response::NACK);
    boost::asio::read(m_interface, boost::asio::buffer(&acknowledge, 1), ec);
    if(ec) {
        // do something about the error
    }
    if(acknowledge != static_cast<std::uint8_t>(Valon::Response::ACK)) {
        // do something about the error
    }
}
