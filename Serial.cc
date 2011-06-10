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
//#    GBT Operations
//#    National Radio Astronomy Observatory
//#    P. O. Box 2
//#    Green Bank, WV 24944-0002 USA


#include "Serial.h"
#include <string.h>

#if defined(VXWORKS)
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#if defined(VXWORKS)
#include <iosLib.h>
#include <selectLib.h>
#endif


#if defined (SOLARIS) || defined (LINUX)
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#endif


Serial::Serial(const char *port) : the_serial_port(0),
                                   the_baud_rate(9600),
                                   the_parity(Serial::none),
                                   the_number_of_data_bits(8),
                                   the_number_of_stop_bits(1),
                                   the_hardware_flow_control_flag(0),
                                   the_software_flow_control_flag(0),
                                   the_input_mode(Serial::raw)
{
    if (open_serial_port(port) == 0)
    {
        set_parity(the_parity);
        set_baud_rate(the_baud_rate);
        set_data_bits(the_number_of_data_bits);
        set_stop_bits(the_number_of_stop_bits);
        set_hardware_flow_control(the_hardware_flow_control_flag);
        set_software_flow_control(the_software_flow_control_flag);
        set_input_mode(the_input_mode);
        set_other_flags();
    }
}


Serial::~Serial()
{
    close(the_serial_port);
}


int Serial::serial_write(const unsigned char *output_buffer,
                         const int &number_of_bytes)
{
    int bytes_written = 0;

    // Create list of file descriptors for writing.
    fd_set writeFds;

    // Set timeout for select call.
    timeval time_limit;
    time_limit.tv_usec = 200000;
    time_limit.tv_sec  = 0;

    FD_ZERO(&writeFds);
    FD_SET(the_serial_port, &writeFds);

    // Check for select timeout or error.
    int result = select(FD_SETSIZE, 0, &writeFds, 0, &time_limit);
    if (result > 0)
    {
        if (FD_ISSET(the_serial_port, &writeFds))
        {
            bytes_written = ::write(the_serial_port,
                                    #if defined (VXWORKS)
                                    reinterpret_cast<char *>(
                                    const_cast<unsigned char *>(output_buffer)),
                                    #else
                                    output_buffer,
                                    #endif
                                    number_of_bytes);
        }
    }

#if defined (SOLARIS) || defined (LINUX)
    tcdrain(the_serial_port);
#endif

#if defined (VXWORKS)
    result = ioctl(the_serial_port, FIOSYNC, 1);
#endif

    return (bytes_written);
}


int Serial::serial_read(unsigned char *input_buffer, const int &number_of_bytes, 
                        const int timeo_us)
{
    int bytes_received = 0;

    // Create list of file descriptors for reading.
    fd_set readFds;

    // Set timeout for select call.
    timeval time_limit;

    // Must read in a while loop because ::read may return between 1 and
    // number_of_bytes.
    while ((bytes_received < number_of_bytes))
    {
        FD_ZERO(&readFds);
        FD_SET(the_serial_port, &readFds);
        time_limit.tv_usec = timeo_us % 1000000;
        time_limit.tv_sec  = timeo_us / 1000000;

        // Check for select timeout or error.
        if (select(FD_SETSIZE, &readFds, 0, 0, &time_limit) <= 0)
        {
            break;
        }

        if (FD_ISSET(the_serial_port, &readFds) != 0)
        {
            int read_bytes = ::read(the_serial_port,
#if defined (VXWORKS)
                             reinterpret_cast<char *>(&input_buffer[bytes_received]),
#else
                             &input_buffer[bytes_received],
#endif
                             (number_of_bytes - bytes_received));
                 
            // was an error condition present?     
            if (read_bytes < 0)
            {
                // Flush input buffer.
                #if defined (SOLARIS) || defined (LINUX)
                ioctl(the_serial_port, TCFLSH, 0);
                #else
                ioctl(the_serial_port, FIORFLUSH, 0);
                #endif
                return(read_bytes);
            }
                 
            bytes_received += read_bytes;

            if (the_input_mode == Serial::canonical && bytes_received > 0)
            {
                // Bytes received may be less than number_of_bytes in
                // canonical mode because in this mode, we read until
                // a carriage-return/line-feed is discovered.
                //
                // Also, no flush is performed in this mode because the
                // input data might be CR/LF deliniated and we don't want
                // to miss any data.
                return (bytes_received);
            }
        }
    }

    return (bytes_received);
}


int Serial::open_serial_port(const char *port_name)
{
#if defined (SOLARIS) || defined (LINUX)
    the_serial_port = open(port_name, O_RDWR);
#else
    the_serial_port = open(port_name, O_RDWR, 0);
#endif

    if (the_serial_port < 0)
    {
        // TBF: Error message
        cerr << "Cannot open serial port" << endl;
        return (-1);
    }
    else
    {
        // Flush serial port.
#if defined (SOLARIS) || defined (LINUX)
        ioctl(the_serial_port, TCFLSH, 0);
#endif
#if defined(VXWORKS)
        ioctl(the_serial_port, FIOFLUSH, 0);
#endif
        return (0);
    }
}


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_parity(const Serial::parity_choices &parity)
{
    the_parity = parity;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set parity" << endl;
        return (-1);
    }

    switch (parity)
    {
        case Serial::odd:
            the_termios.c_cflag |= (PARODD | PARENB);      
            break;
        case Serial::even:
            the_termios.c_cflag |= PARENB;      
            the_termios.c_cflag &= ~(PARODD);      
            break;
        case Serial::none:
            the_termios.c_cflag &= ~PARENB;
            break;
        default:
            the_termios.c_cflag &= ~PARENB;
            the_parity = Serial::none;
            break;
    }

    // Wait until all output has been written.
    tcdrain(the_serial_port);

    if (tcsetattr(the_serial_port, TCSADRAIN, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set parity" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_parity(const Serial::parity_choices &)
{
    // Parity is always none.
    the_parity = Serial::none;

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_baud_rate(const int &baud_rate)
{
    the_baud_rate = baud_rate;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set baud rate" << endl;
        return (-1);
    }
 
    speed_t speed;

    switch (baud_rate)
    {
        case 1200:
            speed = B1200;
            break;
        case 2400:
            speed = B2400;
            break;
        case 4800:
            speed = B4800;
            break;
        case 9600:
            speed = B9600;
            break;
        case 19200:
            speed = B19200;
            break;
        case 38400:
            speed = B38400;
            break;
        case 57600:
            speed = B57600;
            break;
        case 115200:
            speed = B115200;
            break;
        default:
            the_baud_rate = 9600;
            speed = B9600;
            break;
    }

    cfsetispeed(&the_termios, speed);
    cfsetospeed(&the_termios, speed);

    // Wait until all output has been written.
    tcdrain(the_serial_port);

    if (tcsetattr(the_serial_port, TCSADRAIN, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set baud rate" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_baud_rate(const int &baud_rate)
{
    the_baud_rate = baud_rate;

    if (ioctl(the_serial_port, FIOBAUDRATE, the_baud_rate) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set baud rate" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_data_bits(const int &data_bits)
{
    the_number_of_data_bits = data_bits;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set data bits" << endl;
        return (-1);
    }

    the_termios.c_cflag &= ~CSIZE;

    switch (the_number_of_data_bits)
    {
        case 5:
            the_termios.c_cflag |= CS5;
            break;
        case 6:
            the_termios.c_cflag |= CS6;
            break;
        case 7:
            the_termios.c_cflag |= CS7;
            break;
        case 8:
            the_termios.c_cflag |= CS8;
            break;
        default:
            the_number_of_data_bits = 8;
            the_termios.c_cflag |= CS8;
            break;
    }

    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set data bits" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_data_bits(const int &)
{
    // The number of data bits is always 8.
    the_number_of_data_bits = 8;

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_stop_bits(const int &stop_bits)
{
    the_number_of_stop_bits = stop_bits;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set stop bits" << endl;
        return (-1);
    }

    switch (stop_bits)
    {
        case 2:
            the_termios.c_cflag |= CSTOPB;
            break;
        case 1:
            the_termios.c_cflag &= ~CSTOPB;
            break;
        default:
            the_number_of_stop_bits = 1;
            the_termios.c_cflag &= ~CSTOPB;
            break;
    }

    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set stop bits" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_stop_bits(const int &)
{
    // The number of stop bits is always 1.
    the_number_of_stop_bits = 1;

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_hardware_flow_control(const int &hardware_flow_control)
{
    the_hardware_flow_control_flag = hardware_flow_control;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set hardware flow control" << endl;
        return (-1);
    }

    if (the_hardware_flow_control_flag == 0)
    {
        the_termios.c_cflag &= ~CRTSCTS;
    }
    else
    {
        the_termios.c_cflag |= CRTSCTS;
    }

    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set hardware flow control" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_hardware_flow_control(const int &)
{
    // The hardware flow control is always disabled.
    the_hardware_flow_control_flag = 0;

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::update_software_flow_control(int const &software_flow_control)
{
    the_software_flow_control_flag = software_flow_control;

    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set software flow control" << endl;
        return (-1);
    }

    if (the_software_flow_control_flag == 0)
    {
        the_termios.c_iflag &= ~(IXON | IXOFF | IXANY);
    }
    else
    {
        the_termios.c_iflag |= (IXON | IXOFF | IXANY);
    }

    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set software flow control" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::update_software_flow_control(const int &software_flow_control)
{
    the_software_flow_control_flag = software_flow_control;

    if (the_software_flow_control_flag == 1)
    {
        if (ioctl(the_serial_port, FIOSETOPTIONS, OPT_TANDEM) < 0)
        {
            // TBF: Error message
            cerr << "Cannot set software flow control" << endl;
            return (-1);
        }
    }

    return (0);
}
#endif


int Serial::update_input_mode(const Serial::input_choices &input_mode)
{
    the_input_mode = input_mode;

    if (the_input_mode == Serial::raw)
    {
        return (set_raw_input_mode());
    }
    else
    {
        return (set_canonical_input_mode());
    }
}


#if defined (SOLARIS) || defined (LINUX)
int Serial::set_raw_input_mode()
{
    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set raw input mode" << endl;
        return (-1);
    }

    // local flags:-
    // Disable canonical mode (aka raw mode), disables signals, disable
    // echoing of input characters, disable echoing of erase character
    // as BS-SP-BS.
    the_termios.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOE);
    the_termios.c_iflag &= ~(INPCK | ISTRIP);
    
    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set raw input mode" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::set_raw_input_mode()
{
    if (ioctl(the_serial_port, FIOSETOPTIONS, OPT_RAW) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set raw input mode" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::set_canonical_input_mode()
{
    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set canonical input mode" << endl;
        return (-1);
    }

    // local flags:-
    // Enable canonical mode, enable echoing of input characters, enable
    // echoing of erase character as BS-SP-BS.
    the_termios.c_lflag |= (ICANON | ECHO | ECHOE);
    
    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set canonical input mode" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::set_canonical_input_mode()
{
    if (ioctl(the_serial_port, FIOSETOPTIONS, OPT_LINE) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set canonical input mode" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined (SOLARIS) || defined (LINUX)
int Serial::set_other_flags()
{
    struct termios the_termios;

    if (tcgetattr(the_serial_port, &the_termios) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set other flags" << endl;
        return (-1);
    }

    // local flags:-
    // Enable extended functions
    the_termios.c_lflag &= ~IEXTEN;

    // input flags:-
    // Send a SIGINT when a break condition is detected, map CR to NL, 
    the_termios.c_iflag &= ~(BRKINT | ICRNL);
    
    // control flags:-
    // Setting CLOCAL and CREAD ensures that the program will not become
    // the owner of the port and the serial interface driver will read
    // incoming data bytes.
    the_termios.c_cflag |= (CLOCAL | CREAD);
                        
    // output flags:-
    // Turn off the output processing
    the_termios.c_oflag &= ~(OPOST);
    
    // Set up 0 character minimum with no timeout.
    the_termios.c_cc[VMIN]  = 0;
    the_termios.c_cc[VTIME] = 0;

    if (tcsetattr(the_serial_port, TCSAFLUSH, &the_termios) != 0)
    {
        // TBF: Error message
        cerr << "Cannot set other flags" << endl;
        return (-1);
    }

    return (0);
}
#endif


#if defined(VXWORKS)
int Serial::set_other_flags()
{
    // Map CR to NL.
    if (ioctl(the_serial_port, FIOSETOPTIONS, OPT_CRMOD) < 0)
    {
        // TBF: Error message
        cerr << "Cannot set other flags" << endl;
        return (-1);
    }

    return (0);
}
#endif
