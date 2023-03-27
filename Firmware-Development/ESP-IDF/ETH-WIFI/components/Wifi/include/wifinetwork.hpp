#ifndef WIFINETWORK_HPP
#define WIFINETWORK_HPP

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_err.h"

#define WIFI_SSID "eSpark Wifi"
#define WIFI_PW "47356880"

#define EXAMPLE_ESP_MAXIMUM_RETRY 5
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_TAG "WIFINETWORK"

class WIFINETWORK
{
    private:
    char* ssid;
    char* password;
    esp_netif_t * wifiNetif;

    public:
    WIFINETWORK(char* ssid, char* pass) : ssid(ssid) , password(pass){}
    bool connect();
    bool setup(const char* ssid, const char* password);
    void disconnect();
};

static void wifiEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventID, void *eventData);

#endif