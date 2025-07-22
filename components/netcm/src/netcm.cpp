#include "netcm_priv.hpp"

#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_blufi.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"


#define STA_RETRY_MAX_CNT   2       // failure_retry_cnt


NetcmCallback *netcm_callback = NULL;


static uint8_t gl_mac_addr[6];
static char gl_mac_str[18];

static TaskHandle_t netcm_task_handle = NULL;
static QueueHandle_t sta_async_conn_queue = NULL;

static wifi_config_t gl_sta_config;
static bool gl_sta_is_connected = false;
static bool gl_sta_is_connecting = false;
static sta_conn_result_t gl_sta_conn_result;

static uint8_t gl_sta_ssid[32];
static int gl_sta_ssid_len = 0;

static uint8_t gl_sta_password[64];
static int gl_sta_password_len = 0;

static bool gl_sta_is_got_ip = false;
static char gl_sta_local_ip_str[16];

static uint16_t gl_sta_ap_num = 0;
static wifi_ap_record_t *gl_ap_list = NULL;

static bool gl_ble_is_connected = false;
static bool gl_ble_ap_scan_is_running = false;
static esp_blufi_extra_info_t gl_ble_conn_result = {};

static void sta_set_ssid(const char *str) {
    strncpy((char*)gl_sta_config.sta.ssid, str, strlen(str) + 1);
}


static void sta_set_pwd(const char *str) {
    strncpy((char*)gl_sta_config.sta.password, str, strlen(str) + 1);
}


static void sta_clear_config(void) {
    bzero(&gl_sta_config, sizeof(gl_sta_config));
}


static void sta_update_config(void) {
    gl_sta_config.sta.failure_retry_cnt = STA_RETRY_MAX_CNT;
    esp_wifi_set_config(WIFI_IF_STA, &gl_sta_config);
}


void set_wifi_ssid_pwd(const char *ssid, const char *password) {
    if (NULL == ssid || NULL == password) {
        return;
    }
    sta_clear_config();
    sta_set_ssid(ssid);
    sta_set_pwd(password);
    sta_update_config();
}


static void blufi_record_wifi_conn_info(uint8_t reason) {
    bzero(&gl_ble_conn_result, sizeof(esp_blufi_extra_info_t));
    if (gl_sta_is_connecting) {
        gl_ble_conn_result.sta_max_conn_retry_set = true;
        gl_ble_conn_result.sta_max_conn_retry = STA_RETRY_MAX_CNT;
    } else {
        gl_ble_conn_result.sta_conn_rssi_set = true;
        gl_ble_conn_result.sta_conn_rssi = Netcm::sta_get_rssi();
        gl_ble_conn_result.sta_conn_end_reason_set = true;
        gl_ble_conn_result.sta_conn_end_reason = reason;
    }
}


void Netcm::ip_event_handler(void* arg, esp_event_base_t event_base,
                           int32_t event_id, void* event_data) {
    if (event_base != IP_EVENT) {
        ESP_LOGE(TAG, "Received unexpected event base: %s", event_base);
        return;
    }

    switch (event_id) {
        case IP_EVENT_STA_GOT_IP: {
            ip_event_got_ip_t* evt_data = (ip_event_got_ip_t*)event_data;
            if (!evt_data) {
                ESP_LOGE(TAG, "Null event data for IP_EVENT_STA_GOT_IP");
                break;
            }
            ESP_LOGI(TAG, "local ip:" IPSTR, IP2STR(&evt_data->ip_info.ip));
            // Safely format IP string
            int written = snprintf(gl_sta_local_ip_str, sizeof(gl_sta_local_ip_str), 
                                 IPSTR, IP2STR(&evt_data->ip_info.ip));
            if (written <= 0 || written >= sizeof(gl_sta_local_ip_str)) {
                ESP_LOGE(TAG, "Failed to format IP address");
                break;
            }

            gl_sta_is_got_ip = true;

            /* blufi report connection success with IP */
            if (gl_ble_is_connected) {
                blufi_record_wifi_conn_info(255);
                esp_err_t ret = esp_blufi_send_wifi_conn_report(
                    WIFI_MODE_STA, 
                    ESP_BLUFI_STA_CONN_SUCCESS, 
                    gl_sta_ap_num, 
                    &gl_ble_conn_result
                );
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send BLE connection report (0x%x)", ret);
                }
            }

            if (netcm_callback) {
                netcm_callback->on_netif_got_ip(gl_sta_local_ip_str);
            }
            break;
        }

        case IP_EVENT_STA_LOST_IP: {
            ESP_LOGI(TAG, "lost ip");
            gl_sta_is_got_ip = false;
            if (netcm_callback) {
                netcm_callback->on_netif_lost_ip();
            }
            break;
        }

        default:
            ESP_LOGW(TAG, "Unhandled IP event ID");
            break;
    }
}


void Netcm::wifi_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    if (event_base != WIFI_EVENT) {
        ESP_LOGE(TAG, "Received unexpected event base: %s", event_base);
        return;
    }

    switch (event_id) {
        case WIFI_EVENT_SCAN_DONE: {
            wifi_event_sta_scan_done_t *evt_data = (wifi_event_sta_scan_done_t*)event_data;
            if (!evt_data) {
                ESP_LOGE(TAG, "Null event data for WIFI_EVENT_SCAN_DONE");
                break;
            }
            ESP_LOGI(TAG, "Scan done with %d found networks.", evt_data->number);
            if (gl_ap_list) {
                free(gl_ap_list);
                gl_ap_list = nullptr;
            }

            // Get new scan results
            gl_ap_list = netcm_sta_scan_get_result(&gl_sta_ap_num);
            if (!gl_ap_list || gl_sta_ap_num <= 0) {
                ESP_LOGW(TAG, "No valid APs found in scan");
                break;
            }

            // Handle BLE AP scan if needed
            if (gl_ble_ap_scan_is_running && gl_ble_is_connected) {
                esp_blufi_ap_record_t *blufi_ap_list = (esp_blufi_ap_record_t *)malloc(gl_sta_ap_num * sizeof(esp_blufi_ap_record_t));
                if (!blufi_ap_list) {
                    ESP_LOGE(TAG, "Failed to allocate memory for BLE AP list");
                    break;
                }

                memset(blufi_ap_list, 0x0, gl_sta_ap_num * sizeof(esp_blufi_ap_record_t));
                for (int i = 0; i < gl_sta_ap_num; ++i) {
                    blufi_ap_list[i].rssi = gl_ap_list[i].rssi;
                    memcpy(blufi_ap_list[i].ssid, gl_ap_list[i].ssid, sizeof(gl_ap_list[i].ssid));
                }

                esp_err_t ret = esp_blufi_send_wifi_list(gl_sta_ap_num, blufi_ap_list);
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send WiFi list via BLE (0x%x)", ret);
                }

                free(blufi_ap_list);
                gl_ble_ap_scan_is_running = false;
            }

            ESP_LOGI(TAG, "STA scan done, found %d APs", gl_sta_ap_num);
            if (netcm_callback) {
                netcm_callback->on_sta_scan_done(gl_ap_list, gl_sta_ap_num);
            }
            break;
        }

        case WIFI_EVENT_STA_START: {
            ESP_LOGI(TAG, "WiFi started");
            if (esp_wifi_get_config(WIFI_IF_STA, &gl_sta_config) == ESP_OK) {
                memcpy(gl_sta_ssid, gl_sta_config.sta.ssid, sizeof(gl_sta_ssid));
                gl_sta_ssid_len = strnlen((const char*)gl_sta_config.sta.ssid, sizeof(gl_sta_config.sta.ssid)) + 1;
                gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
            }
            break;
        }

        case WIFI_EVENT_STA_STOP: {
            ESP_LOGI(TAG, "WiFi stopped");
            break;
        }

        case WIFI_EVENT_STA_CONNECTED: {
            wifi_event_sta_connected_t *evt_data = (wifi_event_sta_connected_t*)event_data;
            if (!evt_data) {
                ESP_LOGE(TAG, "Null event data for WIFI_EVENT_STA_CONNECTED");
                break;
            }

            ESP_LOGI(TAG, "Connected to SSID: %.*s, channel: %d", 
            sizeof(evt_data->ssid), evt_data->ssid, evt_data->channel);

            gl_sta_is_connected = true;
            gl_sta_is_connecting = false;
            gl_sta_conn_result.set_sta_connected(evt_data);

            if (netcm_callback) {
                netcm_callback->on_sta_connected(evt_data->ssid, evt_data->bssid);
            }
            break;
        }

        case WIFI_EVENT_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t *evt_data = (wifi_event_sta_disconnected_t*)event_data;
            if (!evt_data) {
                ESP_LOGE(TAG, "Null event data for WIFI_EVENT_STA_DISCONNECTED");
                break;
            }

            ESP_LOGI(TAG, "Disconnected from SSID: %.*s, reason: %d", 
            sizeof(evt_data->ssid), evt_data->ssid, evt_data->reason);

            gl_sta_is_connected = false;
            gl_sta_is_connecting = false;
            gl_sta_conn_result.set_sta_disconnected(evt_data);

            if (netcm_callback) {
                netcm_callback->on_sta_disconnected(evt_data->ssid, evt_data->rssi, evt_data->reason);
            }
            break;
        }

        default:
            ESP_LOGW(TAG, "Unhandled WiFi event ID");
            break;
    }
}
/* blufi */


static uint8_t blufi_service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,
};


static esp_ble_adv_data_t blufi_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, //slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, //slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = blufi_service_uuid128,
    .flag = 0x6,
};


static void _esp_blufi_adv_start() {
    esp_ble_gap_set_device_name("BLUFI_KX_DEVICE");
    esp_ble_gap_config_adv_data(&blufi_adv_data);
}


void Netcm::blufi_event_callback(esp_blufi_cb_event_t event, esp_blufi_cb_param_t *param) {
    switch (event) {
        case ESP_BLUFI_EVENT_INIT_FINISH:
            ESP_LOGI(TAG, "BLUFI init finish");
            // esp_blufi_adv_start();
            _esp_blufi_adv_start();
            break;

        case ESP_BLUFI_EVENT_DEINIT_FINISH:
            ESP_LOGI(TAG, "BLUFI deinit finish");
            break;

        case ESP_BLUFI_EVENT_BLE_CONNECT:
            ESP_LOGI(TAG, "BLUFI ble connect");
            gl_ble_is_connected = true;
            esp_blufi_adv_stop();
            break;

        case ESP_BLUFI_EVENT_BLE_DISCONNECT:
            ESP_LOGI(TAG, "BLUFI ble disconnect");
            gl_ble_is_connected = false;
            // esp_blufi_adv_start();
            _esp_blufi_adv_start();
            break;

        case ESP_BLUFI_EVENT_REQ_CONNECT_TO_AP:
            sta_async_connect((const char*)gl_sta_ssid, (const char*)gl_sta_password);
            break;

        case ESP_BLUFI_EVENT_REQ_DISCONNECT_FROM_AP:
            ESP_LOGI(TAG, "blufi request disconnect from ap");
            esp_wifi_disconnect();
            break;

        case ESP_BLUFI_EVENT_RECV_SLAVE_DISCONNECT_BLE:
            ESP_LOGI(TAG, "blufi close a gatt connection");
            esp_blufi_disconnect();
            break;

        case ESP_BLUFI_EVENT_REPORT_ERROR:
            ESP_LOGE(TAG, "BLUFI report error, error code %d\n", param->report_error.state);
            esp_blufi_send_error_info(param->report_error.state);
            break;

        case ESP_BLUFI_EVENT_GET_WIFI_STATUS:
            if (gl_sta_is_connected) {
                esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, gl_sta_is_got_ip ? ESP_BLUFI_STA_CONN_SUCCESS : ESP_BLUFI_STA_NO_IP, gl_sta_ap_num, &gl_ble_conn_result);
            }
            else {
                if (gl_sta_is_connecting) {
                    esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONNECTING, gl_sta_ap_num, &gl_ble_conn_result);
                }
                else {
                    esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, gl_sta_ap_num, &gl_ble_conn_result);
                }
            }
            break;

        case ESP_BLUFI_EVENT_RECV_STA_BSSID:
            break;

        case ESP_BLUFI_EVENT_RECV_STA_SSID:
            strncpy((char *)gl_sta_ssid, (char *)param->sta_ssid.ssid, param->sta_ssid.ssid_len);
            gl_sta_ssid[param->sta_ssid.ssid_len] = '\0';
            gl_sta_ssid_len = param->sta_ssid.ssid_len;
            ESP_LOGI(TAG, "blufi recv ssid %s" ,gl_sta_ssid);
            break;

        case ESP_BLUFI_EVENT_RECV_STA_PASSWD:
            strncpy((char *)gl_sta_password, (char *)param->sta_passwd.passwd, param->sta_passwd.passwd_len);
            gl_sta_password[param->sta_passwd.passwd_len] = '\0';
            gl_sta_password_len = param->sta_passwd.passwd_len;
            ESP_LOGI(TAG, "blufi recv password %s" ,gl_sta_password);
            break;
        
        case ESP_BLUFI_EVENT_GET_WIFI_LIST:
            ESP_LOGI(TAG, "blufi get wifi list");
            gl_ble_ap_scan_is_running = true;
            break;

        case ESP_BLUFI_EVENT_RECV_CUSTOM_DATA:
            ESP_LOGI(TAG, "Recv Custom Data %" PRIu32 "\n", param->custom_data.data_len);
            ESP_LOG_BUFFER_HEX("Custom Data", param->custom_data.data, param->custom_data.data_len);
            break;
        
        case ESP_BLUFI_EVENT_RECV_USERNAME:
            /* Not handle currently */
            break;
        case ESP_BLUFI_EVENT_RECV_CA_CERT:
            /* Not handle currently */
            break;
        case ESP_BLUFI_EVENT_RECV_CLIENT_CERT:
            /* Not handle currently */
            break;
        case ESP_BLUFI_EVENT_RECV_SERVER_CERT:
            /* Not handle currently */
            break;
        case ESP_BLUFI_EVENT_RECV_CLIENT_PRIV_KEY:
            /* Not handle currently */
            break;;
        case ESP_BLUFI_EVENT_RECV_SERVER_PRIV_KEY:
            /* Not handle currently */
            break;

        default:
            break;
    }
}


void Netcm::netcm_init() {
    sta_async_conn_queue = xQueueCreate(10, sizeof(sta_auth_info_t*));

    /* ip */
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL) );

    /* station */
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();
    esp_netif_set_default_netif(esp_netif_sta);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    /* mac */
    esp_efuse_mac_get_default(gl_mac_addr);
    sprintf(gl_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
        gl_mac_addr[0], gl_mac_addr[1], gl_mac_addr[2], gl_mac_addr[3], gl_mac_addr[4], gl_mac_addr[5]);
}


/* station */


void Netcm::sta_async_connect(const char *ssid, const char *password) {
    sta_auth_info_t *auth_info = (sta_auth_info_t*)malloc(sizeof(sta_auth_info_t));
    memset(auth_info, 0x0, sizeof(sta_auth_info_t));
    strncpy(auth_info->ssid, ssid, strlen(ssid) + 1);
    strncpy(auth_info->password, password, strlen(password) + 1);
    xQueueSend(sta_async_conn_queue, &auth_info, portMAX_DELAY);
}


const char *Netcm::sta_get_ssid() {
    return (const char*)gl_sta_config.sta.ssid;
}


bool Netcm::sta_is_connected() {
    return gl_sta_is_connected;
}


bool Netcm::sta_is_connecting(){
    return gl_sta_is_connecting;
}


uint8_t *Netcm::sta_get_mac_addr() {
    return gl_mac_addr;
}

const char *Netcm::sta_get_mac_str() {
    return gl_mac_str;
}


int32_t Netcm::sta_get_rssi() {
    int rssi;
    if (esp_wifi_sta_get_rssi(&rssi) == ESP_OK) {
        return rssi;
    }
    return -128;
}


/* ip */


bool Netcm::ip_is_got() {
    return gl_sta_is_got_ip;
}


const char *Netcm::ip_get_local_addr_str() {
    return gl_sta_local_ip_str;
}


/* task */


void Netcm::register_callback(NetcmCallback *callback) {
    if (NULL == netcm_callback) {
        netcm_callback = callback;
    }
}


void Netcm::loop_start(int priority, int core_id) {
    xTaskCreatePinnedToCore(loop_forever, "netcm", 8192, NULL, priority, &netcm_task_handle, core_id);
}


void Netcm::loop_forever(void *args) {
    sta_auth_info_t *sta_auth_info = NULL;
    TickType_t _last_heart_beat_tick = 0;

    for (;;) {
        /* 主动连接 */
        if (xQueueReceive(sta_async_conn_queue, &sta_auth_info, 0) == pdTRUE) {
            if (gl_sta_is_connected) {
                esp_wifi_disconnect();
            }
            ESP_LOGI(TAG, "receive sta_auth_info: %s, %s", sta_auth_info->ssid, sta_auth_info->password);
            set_wifi_ssid_pwd(sta_auth_info->ssid, sta_auth_info->password);
            free(sta_auth_info);

            /* 清除缓存 */
            gl_sta_conn_result.clear();

            /* 开始连接 */
            gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);

            gl_ble_conn_result.sta_ssid = (uint8_t*)sta_get_ssid();
            gl_ble_conn_result.sta_ssid_len = strlen(sta_get_ssid()) + 1;

            if (gl_sta_is_connecting) {
                netcm_callback->on_sta_start_connect((uint8_t*)sta_get_ssid());
                vTaskDelay(2000 / portTICK_PERIOD_MS);

                /* blufi report 正在连接 */
                blufi_record_wifi_conn_info(255);
                esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONNECTING, gl_sta_ap_num, &gl_ble_conn_result);

                // 等待连接反馈
                ESP_LOGI(TAG, "wait for geting connection result...");
                while (gl_sta_conn_result.is_empty()) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                /* 连接结束 */
                gl_sta_is_connecting = false;

                /* 连接结果反馈 */
                if (gl_sta_conn_result.is_connected) {
                    ESP_LOGI(TAG, "sta connect success");

                    netcm_callback->on_sta_connect_success(gl_sta_conn_result.sta_connected_event);
                } else {
                    ESP_LOGI(TAG, "sta connect failed %d", gl_sta_conn_result.sta_disconnected_event->reason);

                    /* blufi report 连接失败 */
                    if (gl_ble_is_connected) {
                        blufi_record_wifi_conn_info(gl_sta_conn_result.sta_disconnected_event->reason);
                        esp_blufi_send_wifi_conn_report(WIFI_MODE_STA, ESP_BLUFI_STA_CONN_FAIL, gl_sta_ap_num, &gl_ble_conn_result);
                    }

                    netcm_callback->on_sta_connect_failed(gl_sta_conn_result.sta_disconnected_event);

                    /* 等待1s后重试 */
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }
        }

        if (gl_ble_ap_scan_is_running && (!gl_sta_is_connecting)) {
            netcm_sta_scan_all(true);
        }

        /* 心跳 */
        if (xTaskGetTickCount() - _last_heart_beat_tick > pdMS_TO_TICKS(30 * 1000)) {
            _last_heart_beat_tick = xTaskGetTickCount();

            if (!gl_ble_is_connected) {
                if (!gl_sta_is_connected && !gl_sta_is_connecting) {
                    gl_sta_is_connecting = (esp_wifi_connect() == ESP_OK);
                }

                if (NULL != netcm_callback) {
                    netcm_callback->on_netcm_heart_handle();
                }
            }
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
