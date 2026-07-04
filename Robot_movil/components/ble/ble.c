#include "ble.h"
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"

#define TAG "BLE"
#define DEVICE_NAME "ESP32_BLE"
#define CHAR_MAX_LEN 20

static const uint8_t SERVICE_UUID[16] = {
    0x30,0xa9,0x28,0xf6,0x91,0x60,0xc0,0xbb,
    0x54,0x42,0x9b,0xd7,0x9e,0x66,0x2a,0x9b
};

static const uint8_t CHAR_UUID[16] = {
    0x1e,0x00,0xc3,0xe9,0x55,0xfd,0x5a,0x8a,
    0x79,0x44,0x67,0xb2,0x44,0x43,0xe1,0xb6
};

static uint16_t service_handle;
static uint16_t char_handle;

static uint8_t char_value[CHAR_MAX_LEN];
static uint16_t char_len = 0;

static bool new_value = false;

static esp_attr_value_t char_val = {
    .attr_max_len = CHAR_MAX_LEN,
    .attr_len = 0,
    .attr_value = char_value,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .flag = ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    if (event == ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT) {
        esp_ble_gap_start_advertising(&adv_params);
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    switch (event) {

    case ESP_GATTS_REG_EVT: {
        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);

        esp_gatt_srvc_id_t service_id = {
            .is_primary = true,
            .id.inst_id = 0,
            .id.uuid.len = ESP_UUID_LEN_128,
        };
        memcpy(service_id.id.uuid.uuid.uuid128, SERVICE_UUID, 16);

        esp_ble_gatts_create_service(gatts_if, &service_id, 4);
        break;
    }

    case ESP_GATTS_CREATE_EVT: {
        service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(service_handle);

        esp_bt_uuid_t char_uuid = {.len = ESP_UUID_LEN_128};
        memcpy(char_uuid.uuid.uuid128, CHAR_UUID, 16);

        esp_ble_gatts_add_char(
            service_handle,
            &char_uuid,
            ESP_GATT_PERM_WRITE,
            ESP_GATT_CHAR_PROP_BIT_WRITE,
            &char_val,
            NULL
        );
        break;
    }

    case ESP_GATTS_ADD_CHAR_EVT:
        char_handle = param->add_char.attr_handle;
        break;

    case ESP_GATTS_WRITE_EVT:
        if (!param->write.is_prep) {
            uint16_t len = param->write.len;
            if (len > CHAR_MAX_LEN) len = CHAR_MAX_LEN;

            memcpy(char_value, param->write.value, len);
            char_len = len;
            new_value = true;

            esp_gatt_rsp_t rsp = {0};
            rsp.attr_value.handle = param->write.handle;
            rsp.attr_value.len = 0;

            esp_ble_gatts_send_response(
                gatts_if,
                param->write.conn_id,
                param->write.trans_id,
                ESP_GATT_OK,
                &rsp
            );
        }
        break;

    default:
        break;
    }
}

void ble_init(void)
{
    nvs_flash_init();
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    esp_bluedroid_init();
    esp_bluedroid_enable();

    esp_ble_gap_register_callback(gap_event_handler);
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gatts_app_register(0);
}

void ble_get_last_value(char *out, uint16_t max_len)
{
    uint16_t len = char_len;
    if (len > max_len) len = max_len;
    memcpy(out, char_value, len);
    out[len] = 0;
}

bool ble_has_new_value(void)
{
    return new_value;
}

void ble_clear_new_value_flag(void)
{
    new_value = false;
}





