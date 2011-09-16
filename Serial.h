//# Copyright (C) 2011 Associated Universities, Inc. Washington DC, USA.
//# 
//# This program is free software; you can redistribute it and/or modify
//# it under the terms of the GNU General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or
//# (at your option) any later version.
//# 
//# This program is distributed in the hope that it will be useful, but
//# WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//# General Public License for more details.
//# 
//# You should have received a copy of the GNU General Public License
//# along with this program; if not, write to the Free Software
//# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//# 
//# Correspondence concerning GBT software should be addressed as follows:
//#	GBT Operations
//#	National Radio Astronomy Observatory
//#	P. O. Box 2
//#	Green Bank, WV 24944-0002 USA


#ifndef YGOR_SERIAL_H
#define YGOR_SERIAL_H


// <summary>
// This class provides a vehicle for serial communication on the vxWorks, 
// Linux, and Solaris platforms.
// </summary>

// <prerequisite>
// <li> The configuration of the serial port that you wish to use (i.e. 
//      baud rate, parity, port name, etc). 
// </li>
// </prerequisite>

// <etymology>
// This class uses the serial port(s) on your computer for communication.
// </etymology>

// <synopsis>
// This class provides a portable way to use the serial port for 
// program communication.  The two main member functions that enable this
// communication are read and write.  Other member functions are provided
// to configure the serial port as needed.  The default serial port
// configuration is: 9600 baud, 8 data bits, 1 stop bit, no parity, no
// hardware/software flow control, raw input mode.
//
// The valid choices for baud rate are:
// <ul>
// <li> 1200   </li>
// <li> 2400   </li>
// <li> 4800   </li>
// <li> 9600   </li>
// <li> 19200  </li>
// <li> 38400  </li>
// <li> 57600  </li>
// <li> 115200 </li>
// </ul>
//
// The valid choices for the number of data bits are 8 and 7.
//
// The valid choices for the number of stop bits are 1 and 2.
//
// The valid choices for parity are odd, even, and none.  These are
// defined via the enumeration parity_choices below.
//
// The valid choices for input mode are raw and canonical.  These are
// defined via the enumeration input_choices below.  Raw mode processes
// the characters as they are typed.  Canonical mode handles the 
// characters on a line by line basis - i.e. it waits for the '/n'
// character before reading/writing.
// </synopsis>

// <motivation>
// To contain all serial communication needs across multiple platforms 
// within a single class.
// </motivation>


class Serial
{
public:
    enum parity_choices {odd, even, none};
    enum input_choices  {raw, canonical};

    // Default configuration is 9600 baud, 8N1, no hardware or software
    // flow control, and raw input.  Call the member functions below to
    // modify the default configuration.
    // <group>
    explicit Serial(const char *port);
    virtual ~Serial();
    // </group>

    // These member functions follow the Template Method which prefers
    // to make the interface nonvirtual.  Instead, the implementation is
    // virtual and private (see private area below).
    //
    // <group>
    //
    // The write member function requires a buffer of characters which
    // will be sent over the serial port as well as the number of
    // characters from the buffer to send.  The read member function
    // requires a buffer to hold the read characters and the number of
    // bytes to retrieve from the serial port.  Note that if canonical
    // mode is selected, read only returns the characters read when a
    // carriage-return/line-feed is encountered.  Both member functions
    // return a value of -1 to indicate an error, otherwise the number
    // of bytes written/read are returned.
    int write(const unsigned char *output_buffer, const int &number_of_bytes);
    int read(unsigned char *input_buffer, const int &number_of_bytes,
             const int timeout_usec = 200000);

    // set_parity accepts the parity_choices listed above and defaults to
    // none if the parameter passed in doesn't correspond to any of the
    // choices.  Returns 0 on success, -1 on failure.
    int set_parity(const Serial::parity_choices &parity);

    // set_baud_rate accepts 1200, 2400, 4800, 9600, 19200, 38400, 57600,
    // 115200, and defaults to 9600 if the baud rate passed in doesn't
    // correspond to one of these choices.  Returns 0 on success, -1 on
    // failure.
    int set_baud_rate(const int &baud_rate);

    // set_data_bits accepts either 7 or 8 and defaults to 8 if the input
    // parameter is something other than 7 or 8.  Returns 0 on success,
    // -1 on failure.
    int set_data_bits(const int &data_bits);

    // set_stop_bits accepts either 1 or 2 and defaults to 1 if the input
    // parameter is something other than 1 or 2.  Returns 0 on success,
    // -1 on failure.
    int set_stop_bits(const int &stop_bits);

    // set_hardware_flow_control accepts either 0 = No hardware flow 
    // control or 1 = Hardware flow control.  If the input parameter is
    // neither 0 nor 1, hardware flow control is disabled.  Returns 0 on
    // success, -1 on failure.
    int set_hardware_flow_control(const int &hardware_flow_control);

    // set_software_flow_control accepts either 0 = No software flow
    // control or 1 = Software flow control.  If the input parameter is
    // neither 0 nor 1, software flow control is disabled.  Returns 0 on
    // success, -1 on failure.
    int set_software_flow_control(const int &software_flow_control);

    // set_input_mode accepts the input_choices listed above and defaults
    // to raw if the parameter passed in doesn't correspond to any of the
    // choices Raw mode retrieves 1 or more characters per read until the
    // requested number of bytes are retrieved.  In canonical mode, all
    // input characters are saved until a '/n' character is input; then
    // the entire line of characters, including '/n', are sent/received.
    // Returns 0 on success, -1 on failure.
    int set_input_mode(const Serial::input_choices &input_mode);
    // </group>

    bool is_open();

private:
    // Forbidden operations
    // <group>
    Serial(const Serial&);
    Serial& operator=(const Serial&);
    // </group>

    // These data members contain all necessary knowledge about this port.
    // <group>
    int the_serial_port;
    int the_baud_rate;
    parity_choices the_parity;
    int the_number_of_data_bits;
    int the_number_of_stop_bits;
    int the_hardware_flow_control_flag;
    int the_software_flow_control_flag;
    input_choices the_input_mode;
    // </group>

    // These member functions set up parameters for the serial port over 
    // which the user of this class has no control.
    // <group>
    int open_serial_port(const char *port_name);
    int set_raw_input_mode();
    int set_canonical_input_mode();
    int set_other_flags();
    // </group>

    // These member functions are the work-horses behind the public
    // interface  defined above.  This separation of the interface from
    // the implementation will allow easy modification for future needs.
    // <group>
    virtual int serial_write(const unsigned char *output_buffer,
                             const int &number_of_bytes);
    virtual int serial_read(unsigned char *input_buffer,
                            const int &number_of_bytes, 
                            const int timeout_usec = 200000);
    virtual int update_parity(const parity_choices &parity);
    virtual int update_baud_rate(const int &baud_rate);
    virtual int update_data_bits(const int &number_of_data_bits);
    virtual int update_stop_bits(const int &number_of_stop_bits);
    virtual int update_hardware_flow_control(const int
                                             &hardware_flow_control);
    virtual int update_software_flow_control(const int
                                             &software_flow_control);
    virtual int update_input_mode(const input_choices &input_mode);
    // </group>
};


inline int Serial::write(const unsigned char *output_buffer,
                         const int &number_of_bytes)
{
    return (serial_write(output_buffer, number_of_bytes));
}


inline int Serial::read(unsigned char *input_buffer, const int &number_of_bytes, 
                        const int tmo_usec)
{
    return (serial_read(input_buffer, number_of_bytes, tmo_usec));
}


inline int Serial::set_parity(const parity_choices &parity)
{
    return (update_parity(parity));
}


inline int Serial::set_baud_rate(const int &baud_rate)
{
    return (update_baud_rate(baud_rate));
}


inline int Serial::set_data_bits(const int &data_bits)
{
    return (update_data_bits(data_bits));
}


inline int Serial::set_stop_bits(const int &stop_bits)
{
    return (update_stop_bits(stop_bits));
}


inline int Serial::set_hardware_flow_control(const int
                                             &hardware_flow_control)
{
    return (update_hardware_flow_control(hardware_flow_control));
}


inline int Serial::set_software_flow_control(const int
                                             &software_flow_control)
{
    return (update_software_flow_control(software_flow_control));
}


inline int Serial::set_input_mode(const input_choices &input_mode)
{
    return (update_input_mode(input_mode));
}

inline bool Serial::is_open()
{
    if (the_serial_port < 0)
        return false;
    else
        return true;
}

#endif
