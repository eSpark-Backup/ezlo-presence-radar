#include "wifinetwork.hpp"

#define TAG "WIFI"
static EventGroupHandle_t wifiEventGroup;
static int retryNum = 0;

/*
    All the functions are called from the ESPIDF base library, which have a different naming convention.
*/

bool WIFINETWORK::connect()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(WIFI_TAG, "ESP_WIFI_MODE_STA");
    if(setup(this->ssid, this->password))
    {
        ESP_LOGI(TAG,"WIFI CONNECTED SUCCESSFULLY");
        return true;
    }
    else
    {
        return false;
    }
}

static void wifiEventHandler(void *arg, esp_event_base_t eventBase, int32_t eventID, void *eventData) 
{
    if (eventBase == WIFI_EVENT && eventID == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (eventBase == WIFI_EVENT &&eventID == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if (retryNum < EXAMPLE_ESP_MAXIMUM_RETRY) 
        {
            esp_wifi_connect();
            retryNum++;
            ESP_LOGI(WIFI_TAG, "retry to connect to the AP");
        } 
        else 
        {
            xEventGroupSetBits(wifiEventGroup, WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_TAG, "connect to the AP fail");
    } 
    else if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)eventData;
        ESP_LOGI(WIFI_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retryNum = 0;
        xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
    }
}

bool WIFINETWORK::setup(const char* ssid, const char* password) {
    wifiEventGroup = xEventGroupCreate();

    // ESP_ERROR_CHECK(esp_netif_init());

    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifiNetif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, NULL, &instance_got_ip));
    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));
   
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg = {true, false};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WIFI_TAG, "wifiInit finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
    * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
    * bits are set by wifiEventHandler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup,WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we
    * can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT) 
    {
        ESP_LOGW(WIFI_TAG, "connected to ap SSID: %s || password: %s", ssid, password);
        /* The event will not be processed after unregister */
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        vEventGroupDelete(wifiEventGroup);
        return true;
    } 
    else if (bits & WIFI_FAIL_BIT) 
    {
        ESP_LOGE(WIFI_TAG, "Failed to connect to SSID: %s || password: %s", ssid, password);
        /* The event will not be processed after unregister */
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        vEventGroupDelete(wifiEventGroup);
        return false;
    } 
    else 
    {
        ESP_LOGE(WIFI_TAG, "UNEXPECTED EVENT");
        /* The event will not be processed after unregister */
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
        ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
        vEventGroupDelete(wifiEventGroup);
        return false;
    }

}

void WIFINETWORK::disconnect()
{
    if( wifiNetif != NULL){
    esp_wifi_stop();
    esp_netif_destroy(wifiNetif);
    // free(wifiNetif);
    wifiNetif = NULL;}
}