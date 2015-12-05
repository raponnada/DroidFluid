#include <pcap.h>
#include "switch/Switch.hh"

int main( int argc, char* argv[])
{
    std::vector<std::string> ports;
    ports.push_back("wlan0");

    Switch *sw = new Switch(0, "127.0.0.1", 6653, ports);

    sw->start();
    wait_for_sigint();
    sw->stop();

    delete sw;
    return 0;
}
