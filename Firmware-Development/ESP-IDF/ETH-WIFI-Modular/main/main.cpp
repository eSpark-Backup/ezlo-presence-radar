#include "W5500.hpp"

W5500* w5500ETH;

extern "C" void app_main(void)
{
    w5500ETH = new W5500;
    w5500ETH->setup();
    w5500ETH->start();
}