#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-fpermissive"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "bt.h"
#include "bta_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"

#include "sdkconfig.h"

#pragma GCC diagnostic pop

/* this function will be invoked to handle incomming events */
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

#define LED                          2
#define GATTS_SERVICE_UUID_TEST_LED   0xAABB
#define GATTS_CHAR_UUID_LED_CTRL      0xAA01
#define GATTS_CHAR_UUID_TEMP_NOTI     0xBB01
#define GATTS_NUM_HANDLES     8

#define TEST_DEVICE_NAME            "ESP_GATTS_IOTSHARING"
/* maximum value of a characteristic */
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0xFF

/* profile info */
#define PROFILE_ON_APP_ID 0
/* characteristic ids for led ctrl and temp noti */
#define CHAR_NUM 2
#define CHARACTERISTIC_LED_CTRL_ID    0
#define CHARACTERISTIC_TEMP_NOTI_ID   1

/* value range of a attribute (characteristic) */
uint8_t attr_str[] = {0x00};
esp_attr_value_t gatts_attr_val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(attr_str),
    .attr_value   = attr_str,
};
/* custom uuid */
static uint8_t service_uuid128[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xAB, 0xCD, 0x00, 0x00,
};

static esp_ble_adv_data_t test_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 16,
    .p_service_uuid = service_uuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

//this variable holds advertising parameters
esp_ble_adv_params_t test_adv_params;

//this structure holds the information of characteristic
struct gatts_characteristic_inst{
    esp_bt_uuid_t char_uuid;
    esp_bt_uuid_t descr_uuid;
    uint16_t char_handle;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
};

//this structure holds the information of current BLE connection
struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    struct gatts_characteristic_inst chars[CHAR_NUM];
};

//this variable holds the information of current BLE connection
static struct gatts_profile_inst test_profile;

/* 
This callback will be invoked when GAP advertising events come.
Refer GAP BLE callback event type here: 
https://github.com/espressif/esp-idf/blob/master/components/bt/bluedroid/api/include/esp_gap_ble_api.h
*/
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
  case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
   esp_ble_gap_start_advertising(&test_adv_params);
   break;
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
   esp_ble_gap_start_advertising(&test_adv_params);
   break;
  case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
   esp_ble_gap_start_advertising(&test_adv_params);
   break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
   //advertising start complete event to indicate advertising start successfully or failed
   if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
    Serial.printf("\nAdvertising start failed\n");
   }
   break;
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
   if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
    Serial.printf("\nAdvertising stop failed\n");
   }
   else {
    Serial.printf("\nStop adv successfully\n");
   }
   break;
  default:
   break;
    }
}

//process BLE write event ESP_GATTS_WRITE_EVT
void process_write_event_env(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param){
    /* check char handle and set LED */
    if(test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].char_handle == param->write.handle){
        if(param->write.len == 1){
            uint8_t state = param->write.value[0];
            digitalWrite(LED, state);
        }
    }
    /* send response if any */
    if (param->write.need_rsp){
        Serial.printf("respond");
        esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        if (response_err != ESP_OK){
            Serial.printf("\nSend response error\n");
        }
    }
}

/*
This callback will will be invoked when GATT BLE events come.
Refer GATT Server callback function events here: 
https://github.com/espressif/esp-idf/blob/master/components/bt/bluedroid/api/include/esp_gatts_api.h
*/
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
    switch (event) {
    //When register application id, the event comes
    case ESP_GATTS_REG_EVT:{
        Serial.printf("\nREGISTER_APP_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        test_profile.service_id.is_primary = true;
        test_profile.service_id.id.inst_id = 0x00;
        test_profile.service_id.id.uuid.len = ESP_UUID_LEN_16;
        test_profile.service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_LED;
  //after finishing registering, the ESP_GATTS_REG_EVT event comes, we start the next step is creating service
        esp_ble_gatts_create_service(gatts_if, &test_profile.service_id, GATTS_NUM_HANDLES);
        break;
    }
    case ESP_GATTS_READ_EVT: {
        Serial.printf("\nESP_GATTS_READ_EVT\n");
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 14;
        rsp.attr_value.value[0] = 105;
        rsp.attr_value.value[1] = 111;
        rsp.attr_value.value[2] = 116;
        rsp.attr_value.value[3] = 115;
        rsp.attr_value.value[4] = 104;
        rsp.attr_value.value[5] = 97;
        rsp.attr_value.value[6] = 114;
        rsp.attr_value.value[7] = 105;
        rsp.attr_value.value[8] = 110;
        rsp.attr_value.value[9] = 103;
        rsp.attr_value.value[10] = 46;
        rsp.attr_value.value[11] = 99;
        rsp.attr_value.value[12] = 111;
        rsp.attr_value.value[13] = 109;
  /* 
  When central device send READ request to GATT server, the ESP_GATTS_READ_EVT comes 
  This always responds "iotsharing.com" string 
  */
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
 /* 
 When central device send WRITE request to GATT server, the ESP_GATTS_WRITE_EVT comes 
 Invoking process_write_event_env() to process and send response if any
 */
    case ESP_GATTS_WRITE_EVT: {
        Serial.printf("\nESP_GATTS_WRITE_EVT\n");
        process_write_event_env(gatts_if, param);
        break;
    }
    // When create service complete, the event comes
    case ESP_GATTS_CREATE_EVT:{
        Serial.printf("\nstatus %d, service_handle %x, service id %x\n", param->create.status, param->create.service_handle, param->create.service_id.id.uuid.uuid.uuid16);
        // store service handle and add characteristics
        test_profile.service_handle = param->create.service_handle;
        // LED controller characteristic
        esp_ble_gatts_add_char(test_profile.service_handle, &test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].char_uuid,
                               test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].perm,
                               test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].property,
                               &gatts_attr_val, NULL);
        // temperature monitoring characteristic
        esp_ble_gatts_add_char(test_profile.service_handle, 
                               &test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].char_uuid,
                               test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].perm,
                               test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].property,
                               &gatts_attr_val, NULL);
    //and start service
        esp_ble_gatts_start_service(test_profile.service_handle);
        break;
    }
 //When add characteristic complete, the event comes
    case ESP_GATTS_ADD_CHAR_EVT: {
        Serial.printf("\nADD_CHAR_EVT, status %d,  attr_handle %x, service_handle %x, char uuid %x\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle, param->add_char.char_uuid.uuid.uuid16);
        /* store characteristic handles for later usage */
        if(param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_LED_CTRL){
            test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].char_handle = param->add_char.attr_handle;
        }else if(param->add_char.char_uuid.uuid.uuid16 == GATTS_CHAR_UUID_TEMP_NOTI){
            test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].char_handle = param->add_char.attr_handle;
        }   
  //if having characteristic description calling esp_ble_gatts_add_char_descr() here 
        break;
    }
 //When add descriptor complete, the event comes
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:{
        Serial.printf("\nESP_GATTS_ADD_CHAR_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        break;
    }
    /* when disconneting, send advertising information again */
    case ESP_GATTS_DISCONNECT_EVT:{
        esp_ble_gap_start_advertising(&test_adv_params);
        break;
    }
    // When gatt client connect, the event comes
    case ESP_GATTS_CONNECT_EVT: {
        Serial.printf("\nESP_GATTS_CONNECT_EVT\n");
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x50;    // max_int = 0x50*1.25ms = 100ms
        conn_params.min_int = 0x30;    // min_int = 0x30*1.25ms = 60ms
        conn_params.timeout = 1000;    // timeout = 1000*10ms = 10000ms
        test_profile.conn_id = param->connect.conn_id;
        //start sent the update connection parameters to the peer device.
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for the profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            test_profile.gatts_if = gatts_if;
        } else {
            Serial.printf("\nReg app failed, app_id %04x, status %d\n",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    /* here call each profile's callback */
    if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            gatts_if == test_profile.gatts_if) {
        if (test_profile.gatts_cb) {
            test_profile.gatts_cb(event, gatts_if, param);
        }
    }
}

void setup(){
    Serial.begin(115200);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);
    /* initialize advertising info */
    test_adv_params.adv_int_min        = 0x20;
    test_adv_params.adv_int_max        = 0x40;
    test_adv_params.adv_type           = ADV_TYPE_IND;
    test_adv_params.own_addr_type      = BLE_ADDR_TYPE_PUBLIC;
    test_adv_params.channel_map        = ADV_CHNL_ALL;
    test_adv_params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;
    /* initialize profile and characteristic permission and property*/
    test_profile.gatts_cb = gatts_profile_event_handler;
    test_profile.gatts_if = ESP_GATT_IF_NONE;
    test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].char_uuid.len = ESP_UUID_LEN_16;
    test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_LED_CTRL;
    test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
    test_profile.chars[CHARACTERISTIC_LED_CTRL_ID].property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
    test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].char_uuid.len = ESP_UUID_LEN_16;
    test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_TEMP_NOTI;
    test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE;
    test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
    
    esp_err_t ret;
    /* initialize BLE and bluedroid */
    btStart();
    ret = esp_bluedroid_init();
    if (ret) {
        Serial.printf("\n%s init bluetooth failed\n", __func__);
        return;
    }
    ret = esp_bluedroid_enable();
    if (ret) {
        Serial.printf("\n%s enable bluetooth failed\n", __func__);
        return;
    }
    // set BLE name and broadcast advertising info so that the world can see you
    esp_ble_gap_set_device_name(TEST_DEVICE_NAME);
    esp_ble_gap_config_adv_data(&test_adv_data);
    // register callbacks to handle events of GAp and GATT
    esp_ble_gatts_register_callback(gatts_event_handler);
    esp_ble_gap_register_callback(gap_event_handler);
    // register profiles with app id
    esp_ble_gatts_app_register(CHARACTERISTIC_LED_CTRL_ID);
}

long lastMsg = 0;
//send temperature value to registered notification client every 5 seconds via GATT notification
void loop(){
    long now = millis();
    if (now - lastMsg > 5000) {
        lastMsg = now;
        uint8_t temp = random(0, 50);
        esp_ble_gatts_send_indicate(test_profile.gatts_if, 
                                    test_profile.conn_id, 
                                    test_profile.chars[CHARACTERISTIC_TEMP_NOTI_ID].char_handle,
                                    sizeof(temp), &temp, false);
    }
}