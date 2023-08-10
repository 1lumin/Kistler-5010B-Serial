#include <boost/asio.hpp>
#include <iostream>
#include <sys/ioctl.h>

int main() {
    boost::asio::io_context io_context(1);
    boost::asio::serial_port port(io_context, "/dev/tty.usbserial-144340");

    port.set_option(boost::asio::serial_port_base::baud_rate(9'600));
    port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port.set_option(boost::asio::serial_port_base::character_size(8));
    port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));

    int fd = port.native_handle();

    // we are responsible for powering the serial receptor
    int rts_key = TIOCM_RTS;
    if (ioctl(fd, TIOCMBIC, &rts_key) < 0) { // -V
        printf("ioctl failed and returned errno %s \n",strerror(errno));
    }

    int dtr_key = TIOCM_DTR;
    if (ioctl(fd, TIOCMBIS, &dtr_key) < 0) { // +V
        printf("ioctl failed and returned errno %s \n",strerror(errno));
    }

    // wait for the receptor to wake up
    usleep(1000);
    
    auto send = [&](const std::string_view msg){
        boost::asio::write(port, boost::asio::buffer(msg.data(), msg.size()));
    };

    auto query = [&](const std::string_view msg){
        send(msg);

        char c;
        std::string output;

        while(true) {
            boost::asio::read(port, boost::asio::buffer(&c, 1));
            if (c == '\r' || c == '\n')
                break;
            else
                output.push_back(c);
        }
        
        return output;
    };

    // send any valid command to turn on remote mode and consume error
    //  (first command always seems to result in 'command error' which
    //    may be due to bad data from adaptor while setting rts/dts)
    query("SN\r");
    usleep(1000);

    // finally ready to transact
    std::cout << query("SN\r") << "\n";
    std::cout << query("IDN?\r") << "\n";
}
