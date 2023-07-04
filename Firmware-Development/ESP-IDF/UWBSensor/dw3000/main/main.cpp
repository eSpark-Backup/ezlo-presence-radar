#include <stdio.h>
#include "driver/spi_master.h"
#include "dw3000.h"

#define SPI_MOSI_UWB 7
#define SPI_MISO_UWB 6
#define SPI_SCK_UWB 8
#define SPI_CS_UWB 9
// connection pins
const uint8_t PIN_RST = 38; // reset pin
const uint8_t PIN_IRQ = 48; //= 34; // irq pin
const uint8_t PIN_SS = SPI_CS_UWB; // spi select pin

extern "C" void app_main(void)
{
  spiBegin(SPI_MOSI_UWB, SPI_MISO_UWB, SPI_SCK_UWB, PIN_IRQ, PIN_RST);
  spiSelect(PIN_SS);

  delay(2); // Time needed for DW3000 to start up (transition from INIT_RC to IDLE_RC, or could wait for SPIRDY event)
    
  /* Reads and validate device ID returns DWT_ERROR if it does not match expected else DWT_SUCCESS */
  while (1)
  {
    int32_t ret = dwt_check_dev_id();
    if (ret == DWT_SUCCESS)
    printf("DEVICE ID READ SUCCESS\n");
    else 
    printf("ERROR %d\n",ret);
    delay(1000);
  }

}
