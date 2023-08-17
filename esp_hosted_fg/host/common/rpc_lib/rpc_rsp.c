// Copyright 2015-2022 Espressif Systems (Shanghai) PTE LTD
/* SPDX-License-Identifier: GPL-2.0 OR Apache-2.0 */

#include "rpc_core.h"
#include "rpc_api.h"
#include "adapter.h"
#include "esp_log.h"

DEFINE_LOG_TAG(rpc_rsp);

/* RPC response is result of remote function invokation at slave from host
 * The response will contain the return values of the RPC procedure
 * Return values typically will be simple integer return value of rpc call
 * for simple procedures. For function call with return value as a parameter,
 * RPC will contain full structure returned for that parameter and wrapper
 * level above will return these in expected pointer
 *
 * Responses will typically have two levels:
 * 1. protobuf level response received
 * 2. parse the response so that Ctrl_cmd_t app structure will be populated
 * or parsed from protobuf level response.
 *
 * For new RPC request, add up switch case for your message
 * For altogether new RPC function addition, please check
 * esp_hosted_fg/common/proto/esp_hosted_config.proto as a start point
 */

#define RPC_ERR_IN_RESP(msGparaM)                                             \
    if (rpc_msg->msGparaM->resp) {                                            \
        ESP_LOGE(TAG, "Failure resp/event: possibly precondition not met\n");    \
        goto fail_parse_rpc_msg;                                              \
    }


#define RPC_RSP_COPY_BYTES(dst,src) {                                         \
    if (src.data && src.len) {                                                \
        g_h.funcs->_h_memcpy(dst, src.data, src.len);                         \
    }                                                                         \
}

/* This will copy rpc response from `Rpc` into
 * application structure `ctrl_cmd_t`
 * This function is called after protobuf decoding is successful
 **/
int rpc_parse_rsp(Rpc *rpc_msg, ctrl_cmd_t *app_resp)
{
	uint16_t i = 0;

	/* 1. Check non NULL */
	if (!rpc_msg || !app_resp) {
		ESP_LOGE(TAG, "NULL rpc resp or NULL App Resp\n");
		goto fail_parse_rpc_msg;
	}

	/* 2. update basic fields */
	app_resp->msg_type = RPC_TYPE__Resp;
	app_resp->msg_id = rpc_msg->msg_id;
	ESP_LOGD(TAG, "========>> Recvd Resp [0x%x] (for RPC_Req [0x%x])\n",
			app_resp->msg_id, (app_resp->msg_id - RPC_ID__Resp_Base + RPC_ID__Req_Base));

	/* 3. parse Rpc into ctrl_cmd_t */
	switch (rpc_msg->msg_id) {

	case RPC_ID__Resp_GetMACAddress : {
		RPC_FAIL_ON_NULL(resp_get_mac_address);
		RPC_FAIL_ON_NULL(resp_get_mac_address->mac.data);
		RPC_ERR_IN_RESP(resp_get_mac_address);

		RPC_RSP_COPY_BYTES(app_resp->u.wifi_mac.mac, rpc_msg->resp_get_mac_address->mac);
		break;
	} case RPC_ID__Resp_SetMacAddress : {
		RPC_FAIL_ON_NULL(resp_set_mac_address);
		RPC_ERR_IN_RESP(resp_set_mac_address);
		break;
	} case RPC_ID__Resp_GetWifiMode : {
		RPC_FAIL_ON_NULL(resp_get_wifi_mode);
		RPC_ERR_IN_RESP(resp_get_wifi_mode);

		app_resp->u.wifi_mode.mode = rpc_msg->resp_get_wifi_mode->mode;
		break;
	} case RPC_ID__Resp_SetWifiMode : {
		RPC_FAIL_ON_NULL(resp_set_wifi_mode);
		RPC_ERR_IN_RESP(resp_set_wifi_mode);
		break;
#if 0
	} case RPC_ID__Resp_GetAPScanList : {
		RpcRespScanResult *rp = rpc_msg->resp_scan_ap_list;
		wifi_scan_ap_list_t *ap = &app_resp->u.wifi_scan_ap_list;
		wifi_scanlist_t *list = NULL;

		RPC_FAIL_ON_NULL(resp_scan_ap_list);
		RPC_ERR_IN_RESP(resp_scan_ap_list);

		ap->number = rp->count;
		if (rp->count) {

			RPC_FAIL_ON_NULL_PRINT(ap->number,"No APs available");
			list = (wifi_scanlist_t *)g_h.funcs->_h_calloc(ap->number,
					sizeof(wifi_scanlist_t));
			RPC_FAIL_ON_NULL_PRINT(list, "Malloc Failed");
		}

		for (i=0; i<rp->count; i++) {

			if (rp->entries[i]->ssid.len)
				g_h.funcs->_h_memcpy(list[i].ssid, (char *)rp->entries[i]->ssid.data,
					rp->entries[i]->ssid.len);

			if (rp->entries[i]->bssid.len)
				g_h.funcs->_h_memcpy(list[i].bssid, (char *)rp->entries[i]->bssid.data,
					rp->entries[i]->bssid.len);

			list[i].channel = rp->entries[i]->chnl;
			list[i].rssi = rp->entries[i]->rssi;
			list[i].encryption_mode = rp->entries[i]->sec_prot;
		}

		ap->out_list = list;
		/* Note allocation, to be freed later by app */
		app_resp->app_free_buff_func = g_h.funcs->_h_free;
		app_resp->app_free_buff_hdl = list;
		break;
	} case RPC_ID__Resp_GetAPConfig : {
		RPC_FAIL_ON_NULL(resp_get_ap_config);
		hosted_ap_config_t *p = &app_resp->u.hosted_ap_config;

		app_resp->resp_event_status = rpc_msg->resp_get_ap_config->resp;

		switch (rpc_msg->resp_get_ap_config->resp) {

			case RPC_ERR_NOT_CONNECTED:
				strncpy(p->status, NOT_CONNECTED_STR, STATUS_LENGTH);
				p->status[STATUS_LENGTH-1] = '\0';
				ESP_LOGI(TAG, "Station is not connected to AP \n");
				goto fail_parse_rpc_msg2;
				break;

			case SUCCESS:
				strncpy(p->status, SUCCESS_STR, STATUS_LENGTH);
				p->status[STATUS_LENGTH-1] = '\0';
				if (rpc_msg->resp_get_ap_config->ssid.data) {
					strncpy((char *)p->ssid,
							(char *)rpc_msg->resp_get_ap_config->ssid.data,
							MAX_SSID_LENGTH-1);
					p->ssid[MAX_SSID_LENGTH-1] ='\0';
				}
				if (rpc_msg->resp_get_ap_config->bssid.data) {
					uint8_t len_l = 0;

					len_l = min(rpc_msg->resp_get_ap_config->bssid.len,
							MAX_MAC_STR_LEN-1);
					strncpy((char *)p->bssid,
							(char *)rpc_msg->resp_get_ap_config->bssid.data,
							len_l);
					p->bssid[len_l] = '\0';
				}

				p->channel = rpc_msg->resp_get_ap_config->chnl;
				p->rssi = rpc_msg->resp_get_ap_config->rssi;
				p->encryption_mode = rpc_msg->resp_get_ap_config->sec_prot;
				break;

			case FAILURE:
			default:
				/* intentional fall-through */
				strncpy(p->status, FAILURE_STR, STATUS_LENGTH);
				p->status[STATUS_LENGTH-1] = '\0';
				ESP_LOGE(TAG, "Failed to get AP config \n");
				goto fail_parse_rpc_msg2;
				break;
		}
		break;
	} case RPC_ID__Resp_ConnectAP : {
		uint8_t len_l = 0;
		RPC_FAIL_ON_NULL(resp_connect_ap);

		app_resp->resp_event_status = rpc_msg->resp_connect_ap->resp;

		switch(rpc_msg->resp_connect_ap->resp) {
			case RPC_ERR_INVALID_PASSWORD:
				ESP_LOGE(TAG, "Invalid password for SSID\n");
				goto fail_parse_rpc_msg2;
				break;
			case RPC_ERR_NO_AP_FOUND:
				ESP_LOGE(TAG, "SSID: not found/connectable\n");
				goto fail_parse_rpc_msg2;
				break;
			case SUCCESS:
				RPC_FAIL_ON_NULL(resp_connect_ap->mac.data);
				RPC_ERR_IN_RESP(resp_connect_ap);
				break;
			default:
				RPC_ERR_IN_RESP(resp_connect_ap);
				ESP_LOGE(TAG, "Connect AP failed\n");
				goto fail_parse_rpc_msg2;
				break;
		}
		len_l = min(rpc_msg->resp_connect_ap->mac.len, MAX_MAC_STR_LEN-1);
		strncpy(app_resp->u.hosted_ap_config.out_mac,
				(char *)rpc_msg->resp_connect_ap->mac.data, len_l);
		app_resp->u.hosted_ap_config.out_mac[len_l] = '\0';
		break;
	} case RPC_ID__Resp_DisconnectAP : {
		RPC_FAIL_ON_NULL(resp_disconnect_ap);
		RPC_ERR_IN_RESP(resp_disconnect_ap);
		break;
	} case RPC_ID__Resp_GetSoftAPConfig : {
		RPC_FAIL_ON_NULL(resp_get_softap_config);
		RPC_ERR_IN_RESP(resp_get_softap_config);

		if (rpc_msg->resp_get_softap_config->ssid.data) {
			uint16_t len = rpc_msg->resp_get_softap_config->ssid.len;
			uint8_t *data = rpc_msg->resp_get_softap_config->ssid.data;
			uint8_t *app_str = app_resp->u.wifi_softap_config.ssid;

			g_h.funcs->_h_memcpy(app_str, data, len);
			if (len<MAX_SSID_LENGTH)
				app_str[len] = '\0';
			else
				app_str[MAX_SSID_LENGTH-1] = '\0';
		}

		if (rpc_msg->resp_get_softap_config->pwd.data) {
			g_h.funcs->_h_memcpy(app_resp->u.wifi_softap_config.pwd,
					rpc_msg->resp_get_softap_config->pwd.data,
					rpc_msg->resp_get_softap_config->pwd.len);
			app_resp->u.wifi_softap_config.pwd[MAX_PWD_LENGTH-1] = '\0';
		}

		app_resp->u.wifi_softap_config.channel =
			rpc_msg->resp_get_softap_config->chnl;
		app_resp->u.wifi_softap_config.encryption_mode =
			rpc_msg->resp_get_softap_config->sec_prot;
		app_resp->u.wifi_softap_config.max_connections =
			rpc_msg->resp_get_softap_config->max_conn;
		app_resp->u.wifi_softap_config.ssid_hidden =
			rpc_msg->resp_get_softap_config->ssid_hidden;
		app_resp->u.wifi_softap_config.bandwidth =
			rpc_msg->resp_get_softap_config->bw;

		break;
	} case RPC_ID__Resp_SetSoftAPVendorSpecificIE : {
		RPC_FAIL_ON_NULL(resp_set_softap_vendor_specific_ie);
		RPC_ERR_IN_RESP(resp_set_softap_vendor_specific_ie);
		break;
	} case RPC_ID__Resp_StartSoftAP : {
		uint8_t len_l = 0;
		RPC_FAIL_ON_NULL(resp_start_softap);
		RPC_ERR_IN_RESP(resp_start_softap);
		RPC_FAIL_ON_NULL(resp_start_softap->mac.data);

		len_l = min(rpc_msg->resp_connect_ap->mac.len, MAX_MAC_STR_LEN-1);
		strncpy(app_resp->u.wifi_softap_config.out_mac,
				(char *)rpc_msg->resp_connect_ap->mac.data, len_l);
		app_resp->u.wifi_softap_config.out_mac[len_l] = '\0';
		break;
	} case RPC_ID__Resp_GetSoftAPConnectedSTAList : {
		wifi_softap_conn_sta_list_t *ap = &app_resp->u.wifi_softap_con_sta;
		wifi_connected_stations_list_t *list = ap->out_list;
		RpcRespSoftAPConnectedSTA *rp =
			rpc_msg->resp_softap_connected_stas_list;

		RPC_ERR_IN_RESP(resp_softap_connected_stas_list);

		ap->count = rp->num;
		RPC_FAIL_ON_NULL_PRINT(ap->count,"No Stations connected");
		if(ap->count) {
			RPC_FAIL_ON_NULL(resp_softap_connected_stas_list);
			list = (wifi_connected_stations_list_t *)g_h.funcs->_h_calloc(
					ap->count, sizeof(wifi_connected_stations_list_t));
			RPC_FAIL_ON_NULL_PRINT(list, "Malloc Failed");
		}

		for (i=0; i<ap->count; i++) {
			g_h.funcs->_h_memcpy(list[i].bssid, (char *)rp->stations[i]->mac.data,
					rp->stations[i]->mac.len);
			list[i].rssi = rp->stations[i]->rssi;
		}
		app_resp->u.wifi_softap_con_sta.out_list = list;

		/* Note allocation, to be freed later by app */
		app_resp->app_free_buff_func = g_h.funcs->_h_free;
		app_resp->app_free_buff_hdl = list;

		break;
	} case RPC_ID__Resp_StopSoftAP : {
		RPC_FAIL_ON_NULL(resp_stop_softap);
		RPC_ERR_IN_RESP(resp_stop_softap);
		break;
#endif
	} case RPC_ID__Resp_WifiSetPs: {
		RPC_FAIL_ON_NULL(resp_wifi_set_ps);
		RPC_ERR_IN_RESP(resp_wifi_set_ps);
		break;
	} case RPC_ID__Resp_WifiGetPs : {
		RPC_FAIL_ON_NULL(resp_wifi_get_ps);
		RPC_ERR_IN_RESP(resp_wifi_get_ps);
		app_resp->u.wifi_ps.ps_mode = rpc_msg->resp_wifi_get_ps->mode;
		break;
	} case RPC_ID__Resp_OTABegin : {
		RPC_FAIL_ON_NULL(resp_ota_begin);
		RPC_ERR_IN_RESP(resp_ota_begin);
		if (rpc_msg->resp_ota_begin->resp) {
			ESP_LOGE(TAG, "OTA Begin Failed\n");
			goto fail_parse_rpc_msg;
		}
		break;
	} case RPC_ID__Resp_OTAWrite : {
		RPC_FAIL_ON_NULL(resp_ota_write);
		RPC_ERR_IN_RESP(resp_ota_write);
		if (rpc_msg->resp_ota_write->resp) {
			ESP_LOGE(TAG, "OTA write failed\n");
			goto fail_parse_rpc_msg;
		}
		break;
	} case RPC_ID__Resp_OTAEnd: {
		RPC_FAIL_ON_NULL(resp_ota_end);
		if (rpc_msg->resp_ota_end->resp) {
			ESP_LOGE(TAG, "OTA write failed\n");
			goto fail_parse_rpc_msg;
		}
		break;
	} case RPC_ID__Resp_WifiSetMaxTxPower: {
		RPC_FAIL_ON_NULL(req_set_wifi_max_tx_power);
		switch (rpc_msg->resp_set_wifi_max_tx_power->resp)
		{
			case FAILURE:
				ESP_LOGE(TAG, "Failed to set max tx power\n");
				goto fail_parse_rpc_msg;
				break;
			case SUCCESS:
				break;
			case RPC_ERR_OUT_OF_RANGE:
				ESP_LOGE(TAG, "Power is OutOfRange. Check api doc for reference\n");
				goto fail_parse_rpc_msg;
				break;
			default:
				ESP_LOGE(TAG, "unexpected response\n");
				goto fail_parse_rpc_msg;
				break;
		}
		break;
	} case RPC_ID__Resp_WifiGetMaxTxPower: {
		RPC_FAIL_ON_NULL(resp_get_wifi_curr_tx_power);
		RPC_ERR_IN_RESP(resp_get_wifi_curr_tx_power);
		app_resp->u.wifi_tx_power.power =
			rpc_msg->resp_get_wifi_curr_tx_power->wifi_curr_tx_power;
		break;
	} case RPC_ID__Resp_ConfigHeartbeat: {
		RPC_FAIL_ON_NULL(resp_config_heartbeat);
		RPC_ERR_IN_RESP(resp_config_heartbeat);
		break;
	} case RPC_ID__Resp_WifiInit: {
		RPC_FAIL_ON_NULL(resp_wifi_init);
		RPC_ERR_IN_RESP(resp_wifi_init);
		break;
	} case RPC_ID__Resp_WifiDeinit: {
		RPC_FAIL_ON_NULL(resp_wifi_deinit);
		RPC_ERR_IN_RESP(resp_wifi_deinit);
		break;
	} case RPC_ID__Resp_WifiStart: {
		RPC_FAIL_ON_NULL(resp_wifi_start);
		RPC_ERR_IN_RESP(resp_wifi_start);
		break;
	} case RPC_ID__Resp_WifiStop: {
		RPC_FAIL_ON_NULL(resp_wifi_stop);
		RPC_ERR_IN_RESP(resp_wifi_stop);
		break;
	} case RPC_ID__Resp_WifiConnect: {
		RPC_FAIL_ON_NULL(resp_wifi_connect);
		RPC_ERR_IN_RESP(resp_wifi_connect);
		break;
	} case RPC_ID__Resp_WifiDisconnect: {
		RPC_FAIL_ON_NULL(resp_wifi_disconnect);
		RPC_ERR_IN_RESP(resp_wifi_disconnect);
		break;
    } case RPC_ID__Resp_WifiSetConfig: {
		RPC_FAIL_ON_NULL(resp_wifi_set_config);
		RPC_ERR_IN_RESP(resp_wifi_set_config);
		break;
    } case RPC_ID__Resp_WifiGetConfig: {
		RPC_FAIL_ON_NULL(resp_wifi_set_config);
		RPC_ERR_IN_RESP(resp_wifi_set_config);

		app_resp->u.wifi_config.iface = rpc_msg->resp_wifi_get_config->iface;

		switch (app_resp->u.wifi_config.iface) {

		case WIFI_IF_STA: {
			wifi_sta_config_t * p_a_sta = &(app_resp->u.wifi_config.u.sta);
			WifiStaConfig * p_c_sta = rpc_msg->resp_wifi_get_config->cfg->sta;
			RPC_RSP_COPY_BYTES(p_a_sta->ssid, p_c_sta->ssid);
			RPC_RSP_COPY_BYTES(p_a_sta->password, p_c_sta->password);
			p_a_sta->scan_method = p_c_sta->scan_method;
			p_a_sta->bssid_set = p_c_sta->bssid_set;

			if (p_a_sta->bssid_set)
				RPC_RSP_COPY_BYTES(p_a_sta->bssid, p_c_sta->bssid);

			p_a_sta->channel = p_c_sta->channel;
			p_a_sta->listen_interval = p_c_sta->listen_interval;
			p_a_sta->sort_method = p_c_sta->sort_method;
			p_a_sta->threshold.rssi = p_c_sta->threshold->rssi;
			p_a_sta->threshold.authmode = p_c_sta->threshold->authmode;
			//p_a_sta->ssid_hidden = p_c_sta->ssid_hidden;
			//p_a_sta->max_connections = p_c_sta->max_connections;
			p_a_sta->pmf_cfg.capable = p_c_sta->pmf_cfg->capable;
			p_a_sta->pmf_cfg.required = p_c_sta->pmf_cfg->required;

			p_a_sta->rm_enabled = H_GET_BIT(STA_RM_ENABLED_BIT, p_c_sta->bitmask);
			p_a_sta->btm_enabled = H_GET_BIT(STA_BTM_ENABLED_BIT, p_c_sta->bitmask);
			p_a_sta->mbo_enabled = H_GET_BIT(STA_MBO_ENABLED_BIT, p_c_sta->bitmask);
			p_a_sta->ft_enabled = H_GET_BIT(STA_FT_ENABLED_BIT, p_c_sta->bitmask);
			p_a_sta->owe_enabled = H_GET_BIT(STA_OWE_ENABLED_BIT, p_c_sta->bitmask);
			p_a_sta->transition_disable = H_GET_BIT(STA_TRASITION_DISABLED_BIT, p_c_sta->bitmask);
			p_a_sta->reserved = WIFI_CONFIG_STA_GET_RESERVED_VAL(p_c_sta->bitmask);

			p_a_sta->sae_pwe_h2e = p_c_sta->sae_pwe_h2e;
			p_a_sta->failure_retry_cnt = p_c_sta->failure_retry_cnt;
			break;
		}
		case WIFI_IF_AP: {
			wifi_ap_config_t * p_a_ap = &(app_resp->u.wifi_config.u.ap);
			WifiApConfig * p_c_ap = rpc_msg->resp_wifi_get_config->cfg->ap;

			RPC_RSP_COPY_BYTES(p_a_ap->ssid, p_c_ap->ssid);
			RPC_RSP_COPY_BYTES(p_a_ap->password, p_c_ap->password);
			p_a_ap->ssid_len = p_c_ap->ssid_len;
			p_a_ap->channel = p_c_ap->channel;
			p_a_ap->authmode = p_c_ap->authmode;
			p_a_ap->ssid_hidden = p_c_ap->ssid_hidden;
			p_a_ap->max_connection = p_c_ap->max_connection;
			p_a_ap->beacon_interval = p_c_ap->beacon_interval;
			p_a_ap->pairwise_cipher = p_c_ap->pairwise_cipher;
			p_a_ap->ftm_responder = p_c_ap->ftm_responder;
			p_a_ap->pmf_cfg.capable = p_c_ap->pmf_cfg->capable;
			p_a_ap->pmf_cfg.required = p_c_ap->pmf_cfg->required;
			break;
		}
		default:
            ESP_LOGE(TAG, "Unsupported WiFi interface[%u]\n", app_resp->u.wifi_config.iface);
		} //switch

		break;

    } case RPC_ID__Resp_WifiScanStart: {
		RPC_FAIL_ON_NULL(resp_wifi_scan_start);
		RPC_ERR_IN_RESP(resp_wifi_scan_start);
		break;
    } case RPC_ID__Resp_WifiScanStop: {
		RPC_FAIL_ON_NULL(resp_wifi_scan_stop);
		RPC_ERR_IN_RESP(resp_wifi_scan_stop);
		break;
    } case RPC_ID__Resp_WifiScanGetApNum: {
		wifi_scan_ap_list_t *p_a = &(app_resp->u.wifi_scan_ap_list);
		RPC_FAIL_ON_NULL(resp_wifi_scan_get_ap_num);
		RPC_ERR_IN_RESP(resp_wifi_scan_get_ap_num);

		p_a->number = rpc_msg->resp_wifi_scan_get_ap_num->number;
		break;
    } case RPC_ID__Resp_WifiScanGetApRecords: {
		wifi_scan_ap_list_t *p_a = &(app_resp->u.wifi_scan_ap_list);
		wifi_ap_record_t *list = NULL;
		WifiApRecord **p_c_list = NULL;

		RPC_FAIL_ON_NULL(resp_wifi_scan_get_ap_records);
		RPC_ERR_IN_RESP(resp_wifi_scan_get_ap_records);
		p_c_list = rpc_msg->resp_wifi_scan_get_ap_records->ap_records;

		p_a->number = rpc_msg->resp_wifi_scan_get_ap_records->number;

		if (!p_a->number) {
			ESP_LOGI(TAG, "No AP found\n\r");
			goto fail_parse_rpc_msg2;
		}
		ESP_LOGI(TAG, "Num AP records: %u\n\r",
				app_resp->u.wifi_scan_ap_list.number);

		RPC_FAIL_ON_NULL(resp_wifi_scan_get_ap_records->ap_records);

		list = (wifi_ap_record_t*)g_h.funcs->_h_calloc(p_a->number,
				sizeof(wifi_ap_record_t));
		p_a->out_list = list;

		RPC_FAIL_ON_NULL_PRINT(list, "Malloc Failed");

		app_resp->app_free_buff_func = g_h.funcs->_h_free;
		app_resp->app_free_buff_hdl = list;

		ESP_LOGI(TAG, "Number of available APs is %d\n\r", p_a->number);
		for (i=0; i<p_a->number; i++) {

			WifiCountry *p_c_cntry = p_c_list[i]->country;
			wifi_country_t *p_a_cntry = &list[i].country;

			ESP_LOGI(TAG, "\n\nap_record[%u]:\n", i+1);
			ESP_LOGI(TAG, "ssid len: %u\n", p_c_list[i]->ssid.len);
			RPC_RSP_COPY_BYTES(list[i].ssid, p_c_list[i]->ssid);
			RPC_RSP_COPY_BYTES(list[i].bssid, p_c_list[i]->bssid);
			list[i].primary = p_c_list[i]->primary;
			list[i].second = p_c_list[i]->second;
			list[i].rssi = p_c_list[i]->rssi;
			list[i].authmode = p_c_list[i]->authmode;
			list[i].pairwise_cipher = p_c_list[i]->pairwise_cipher;
			list[i].group_cipher = p_c_list[i]->group_cipher;
			list[i].ant = p_c_list[i]->ant;
			//list-> = p_c_list->;
			//list-> = p_c_list->;
			list[i].phy_11b       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11b_BIT, p_c_list[i]->bitmask);
			list[i].phy_11g       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11g_BIT, p_c_list[i]->bitmask);
			list[i].phy_11n       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11n_BIT, p_c_list[i]->bitmask);
			list[i].phy_lr        = H_GET_BIT(WIFI_SCAN_AP_REC_phy_lr_BIT, p_c_list[i]->bitmask);
			list[i].wps           = H_GET_BIT(WIFI_SCAN_AP_REC_wps_BIT, p_c_list[i]->bitmask);
			list[i].ftm_responder = H_GET_BIT(WIFI_SCAN_AP_REC_ftm_responder_BIT, p_c_list[i]->bitmask);
			list[i].ftm_initiator = H_GET_BIT(WIFI_SCAN_AP_REC_ftm_initiator_BIT, p_c_list[i]->bitmask);
			list[i].reserved      = WIFI_SCAN_AP_GET_RESERVED_VAL(p_c_list[i]->bitmask);

			RPC_RSP_COPY_BYTES(p_a_cntry->cc, p_c_cntry->cc);
			p_a_cntry->schan = p_c_cntry->schan;
			p_a_cntry->nchan = p_c_cntry->nchan;
			p_a_cntry->max_tx_power = p_c_cntry->max_tx_power;
			p_a_cntry->policy = p_c_cntry->policy;


			ESP_LOGI(TAG, "Ssid: %s\nBssid: "MACSTR"\nPrimary: %u\nSecond: %u\nRssi: %d\nAuthmode: %u\nPairwiseCipher: %u\nGroupcipher: %u\nAnt: %u\nBitmask:11b:%u g:%u n:%u lr:%u wps:%u ftm_resp:%u ftm_ini:%u res: %u\n",
					list[i].ssid, MAC2STR(list[i].bssid),
					list[i].primary, list[i].second,
					list[i].rssi, list[i].authmode,
					list[i].pairwise_cipher, list[i].group_cipher,
					list[i].ant, list[i].phy_11b, list[i].phy_11g,
					list[i].phy_11n, list[i].phy_lr,
					list[i].wps, list[i].ftm_responder,
					list[i].ftm_initiator, list[i].reserved
					);
			ESP_LOGI(TAG, "Country:\n  cc:%s schan: %u nchan: %u max_tx_pow: %d policy: %u\n",
					p_a_cntry->cc, p_a_cntry->schan, p_a_cntry->nchan,
					p_a_cntry->max_tx_power,p_a_cntry->policy);

			//p_a_sta->rm_enabled = H_GET_BIT(STA_RM_ENABLED_BIT, p_c_sta->bitmask);
		}
		break;
    } case RPC_ID__Resp_WifiStaGetApInfo: {
		WifiApRecord *p_c = NULL;
		wifi_ap_record_t *ap_info = NULL;
		wifi_scan_ap_list_t *p_a = &(app_resp->u.wifi_scan_ap_list);

		RPC_FAIL_ON_NULL(resp_wifi_sta_get_ap_info);
		RPC_ERR_IN_RESP(resp_wifi_sta_get_ap_info);
		p_c = rpc_msg->resp_wifi_sta_get_ap_info->ap_records;

		p_a->number = 1;

		RPC_FAIL_ON_NULL(resp_wifi_sta_get_ap_info->ap_records);

		ap_info = (wifi_ap_record_t*)g_h.funcs->_h_calloc(p_a->number,
				sizeof(wifi_ap_record_t));
		p_a->out_list = ap_info;

		RPC_FAIL_ON_NULL_PRINT(ap_info, "Malloc Failed");

		app_resp->app_free_buff_func = g_h.funcs->_h_free;
		app_resp->app_free_buff_hdl = ap_info;

		{
			WifiCountry *p_c_cntry = p_c->country;
			wifi_country_t *p_a_cntry = &ap_info->country;

			ESP_LOGI(TAG, "\n\nap_info\n");
			ESP_LOGI(TAG,"ssid len: %u\n", p_c->ssid.len);
			RPC_RSP_COPY_BYTES(ap_info->ssid, p_c->ssid);
			RPC_RSP_COPY_BYTES(ap_info->bssid, p_c->bssid);
			ap_info->primary = p_c->primary;
			ap_info->second = p_c->second;
			ap_info->rssi = p_c->rssi;
			ap_info->authmode = p_c->authmode;
			ap_info->pairwise_cipher = p_c->pairwise_cipher;
			ap_info->group_cipher = p_c->group_cipher;
			ap_info->ant = p_c->ant;
			//list-> = p_c_list->;
			//list-> = p_c_list->;
			ap_info->phy_11b       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11b_BIT, p_c->bitmask);
			ap_info->phy_11g       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11g_BIT, p_c->bitmask);
			ap_info->phy_11n       = H_GET_BIT(WIFI_SCAN_AP_REC_phy_11n_BIT, p_c->bitmask);
			ap_info->phy_lr        = H_GET_BIT(WIFI_SCAN_AP_REC_phy_lr_BIT, p_c->bitmask);
			ap_info->wps           = H_GET_BIT(WIFI_SCAN_AP_REC_wps_BIT, p_c->bitmask);
			ap_info->ftm_responder = H_GET_BIT(WIFI_SCAN_AP_REC_ftm_responder_BIT, p_c->bitmask);
			ap_info->ftm_initiator = H_GET_BIT(WIFI_SCAN_AP_REC_ftm_initiator_BIT, p_c->bitmask);
			ap_info->reserved      = WIFI_SCAN_AP_GET_RESERVED_VAL(p_c->bitmask);

			RPC_RSP_COPY_BYTES(p_a_cntry->cc, p_c_cntry->cc);
			p_a_cntry->schan = p_c_cntry->schan;
			p_a_cntry->nchan = p_c_cntry->nchan;
			p_a_cntry->max_tx_power = p_c_cntry->max_tx_power;
			p_a_cntry->policy = p_c_cntry->policy;

			/*ESP_LOGE(TAG, "AP info: ssid \"%s\" bssid \"%s\" rssi \"%d\" channel \"%d\" auth mode \"%d\" \n\r",\
			  ap_info->ssid, ap_info->bssid, ap_info->rssi,
			  ap_info->channel, ap_info->authmode);*/

			ESP_LOGI(TAG, "Ssid: %s\nBssid: "MACSTR"\nPrimary: %u\nSecond: %u\nRssi: %d\nAuthmode: %u\nPairwiseCipher: %u\nGroupcipher: %u\nAnt: %u\nBitmask:11b:%u g:%u n:%u lr:%u wps:%u ftm_resp:%u ftm_ini:%u res: %u\n",
					ap_info->ssid, MAC2STR(ap_info->bssid),
					ap_info->primary, ap_info->second,
					ap_info->rssi, ap_info->authmode,
					ap_info->pairwise_cipher, ap_info->group_cipher,
					ap_info->ant, ap_info->phy_11b, ap_info->phy_11g,
					ap_info->phy_11n, ap_info->phy_lr,
					ap_info->wps, ap_info->ftm_responder,
					ap_info->ftm_initiator, ap_info->reserved
					);
			ESP_LOGI(TAG, "Country:\n  cc:%s schan: %u nchan: %u max_tx_pow: %d policy: %u\n",
					p_a_cntry->cc, p_a_cntry->schan, p_a_cntry->nchan,
					p_a_cntry->max_tx_power,p_a_cntry->policy);

			//p_a_sta->rm_enabled = H_GET_BIT(STA_RM_ENABLED_BIT, p_c_sta->bitmask);
		}
		break;
    } case RPC_ID__Resp_WifiClearApList: {
		RPC_FAIL_ON_NULL(resp_wifi_clear_ap_list);
		RPC_ERR_IN_RESP(resp_wifi_clear_ap_list);
		break;
	} case RPC_ID__Resp_WifiRestore: {
		RPC_FAIL_ON_NULL(resp_wifi_restore);
		RPC_ERR_IN_RESP(resp_wifi_restore);
		break;
	} case RPC_ID__Resp_WifiClearFastConnect: {
		RPC_FAIL_ON_NULL(resp_wifi_clear_fast_connect);
		RPC_ERR_IN_RESP(resp_wifi_clear_fast_connect);
		break;
	} case RPC_ID__Resp_WifiDeauthSta: {
		RPC_FAIL_ON_NULL(resp_wifi_deauth_sta);
		RPC_ERR_IN_RESP(resp_wifi_deauth_sta);
		break;
	} default: {
		ESP_LOGE(TAG, "Unsupported rpc Resp[%u]\n", rpc_msg->msg_id);
		goto fail_parse_rpc_msg;
		break;
	}

	}

	/* 4. Free up buffers */
	rpc__free_unpacked(rpc_msg, NULL);
	rpc_msg = NULL;
	app_resp->resp_event_status = SUCCESS;
	return SUCCESS;

	/* 5. Free up buffers in failure cases */
fail_parse_rpc_msg:
	rpc__free_unpacked(rpc_msg, NULL);
	rpc_msg = NULL;
	app_resp->resp_event_status = FAILURE;
	return FAILURE;

fail_parse_rpc_msg2:
	rpc__free_unpacked(rpc_msg, NULL);
	rpc_msg = NULL;
	return FAILURE;
}
