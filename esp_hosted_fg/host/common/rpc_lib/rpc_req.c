// Copyright 2015-2022 Espressif Systems (Shanghai) PTE LTD
/* SPDX-License-Identifier: GPL-2.0 OR Apache-2.0 */

#include "rpc_core.h"
#include "rpc_api.h"
#include "adapter.h"
#include "esp_log.h"

DEFINE_LOG_TAG(rpc_req);

#define ADD_RPC_BUFF_TO_FREE_LATER(BuFf) {                                      \
	assert((app_req->n_rpc_free_buff_hdls+1)<=MAX_FREE_BUFF_HANDLES);           \
	app_req->rpc_free_buff_hdls[app_req->n_rpc_free_buff_hdls++] = BuFf;        \
}

#define RPC_ALLOC_ASSIGN(TyPe,MsG_StRuCt,InItFuNc)                            \
    TyPe *req_payload = (TyPe *)                                              \
        g_h.funcs->_h_calloc(1, sizeof(TyPe));                                \
    if (!req_payload) {                                                       \
        ESP_LOGE(TAG, "Failed to allocate memory for req->%s\n",#MsG_StRuCt);     \
        *failure_status = RPC_ERR_MEMORY_FAILURE;                              \
		return FAILURE;                                                       \
    }                                                                         \
    req->MsG_StRuCt = req_payload;                                             \
	InItFuNc(req_payload);                                                    \
    ADD_RPC_BUFF_TO_FREE_LATER((uint8_t*)req_payload);

//TODO: How this is different in slave_control.c
#define RPC_ALLOC_ELEMENT(TyPe,MsG_StRuCt,InIt_FuN) {                         \
    TyPe *NeW_AllocN = (TyPe *) g_h.funcs->_h_calloc(1, sizeof(TyPe));        \
    if (!NeW_AllocN) {                                                        \
        ESP_LOGE(TAG, "Failed to allocate memory for req->%s\n",#MsG_StRuCt);     \
        *failure_status = RPC_ERR_MEMORY_FAILURE;                              \
		return FAILURE;                                                       \
    }                                                                         \
    ADD_RPC_BUFF_TO_FREE_LATER((uint8_t*)NeW_AllocN);                         \
    MsG_StRuCt = NeW_AllocN;                                                  \
    InIt_FuN(MsG_StRuCt);                                                     \
}

/* RPC request is simple remote function invokation at slave from host
 *
 * For new RPC request, add up switch case for your message
 * If the RPC function to be invoked does not carry any arguments, just add
 * case in the top with intentional fall through
 * If any arguments are needed, you may have to add union for your message
 * in Ctrl_cmd_t in rpc_api.h and fill the request in new case
 *
 * For altogether new RPC function addition, please check
 * esp_hosted_fg/common/proto/esp_hosted_config.proto
 */
int compose_rpc_req(Rpc *req, ctrl_cmd_t *app_req, uint8_t *failure_status)
{
	switch(req->msg_id) {

	case RPC_ID__Req_GetWifiMode:
	//case RPC_ID__Req_GetAPConfig:
	//case RPC_ID__Req_DisconnectAP:
	//case RPC_ID__Req_GetSoftAPConfig:
	//case RPC_ID__Req_GetSoftAPConnectedSTAList:
	//case RPC_ID__Req_StopSoftAP:
	case RPC_ID__Req_WifiGetPs:
	case RPC_ID__Req_OTABegin:
	case RPC_ID__Req_OTAEnd:
	case RPC_ID__Req_WifiDeinit:
	case RPC_ID__Req_WifiStart:
	case RPC_ID__Req_WifiStop:
	case RPC_ID__Req_WifiConnect:
	case RPC_ID__Req_WifiDisconnect:
	case RPC_ID__Req_WifiScanStop:
	case RPC_ID__Req_WifiScanGetApNum:
	case RPC_ID__Req_WifiClearApList:
	case RPC_ID__Req_WifiRestore:
	case RPC_ID__Req_WifiClearFastConnect:
	case RPC_ID__Req_WifiStaGetApInfo:
	case RPC_ID__Req_WifiGetMaxTxPower:
	case RPC_ID__Req_WifiGetChannel:
	case RPC_ID__Req_WifiGetCountryCode:
	case RPC_ID__Req_WifiGetCountry: {
		/* Intentional fallthrough & empty */
		break;
#if 0
	} case RPC_ID__Req_GetAPScanList: {
		if (app_req->rsp_timeout_sec < DEFAULT_RPC_RSP_AP_SCAN_TIMEOUT)
			app_req->rsp_timeout_sec = DEFAULT_RPC_RSP_AP_SCAN_TIMEOUT;
		break;
#endif
	} case RPC_ID__Req_GetMACAddress: {
		RPC_ALLOC_ASSIGN(RpcReqGetMacAddress, req_get_mac_address,
				rpc__req__get_mac_address__init);

		req_payload->mode = app_req->u.wifi_mac.mode;

		break;
	} case RPC_ID__Req_SetMacAddress: {
		wifi_mac_t * p = &app_req->u.wifi_mac;
		RPC_ALLOC_ASSIGN(RpcReqSetMacAddress, req_set_mac_address,
				rpc__req__set_mac_address__init);

		if ((p->mode <= WIFI_MODE_NULL) ||
		    (p->mode >= WIFI_MODE_APSTA)||
		    (!(p->mac[0]))) {
			ESP_LOGE(TAG, "Invalid parameter\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->mode = p->mode;
		RPC_REQ_COPY_BYTES(req_payload->mac, p->mac, BSSID_LENGTH);

		break;
	} case RPC_ID__Req_SetWifiMode: {
		hosted_mode_t * p = &app_req->u.wifi_mode;
		RPC_ALLOC_ASSIGN(RpcReqSetMode, req_set_wifi_mode,
				rpc__req__set_mode__init);

		if ((p->mode < WIFI_MODE_NULL) || (p->mode >= WIFI_MODE_MAX)) {
			ESP_LOGE(TAG, "Invalid wifi mode\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}
		req_payload->mode = p->mode;
		break;
#if 0
	} case RPC_ID__Req_ConnectAP: {
		hosted_ap_config_t * p = &app_req->u.hosted_ap_config;
		RPC_ALLOC_ASSIGN(RpcReqConnectAP,req_connect_ap,
				rpc__req__connect_ap__init);

		if ((strlen((char *)p->ssid) > MAX_SSID_LENGTH) ||
				(!strlen((char *)p->ssid))) {
			ESP_LOGE(TAG, "Invalid SSID length\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if (strlen((char *)p->pwd) > MAX_PWD_LENGTH) {
			ESP_LOGE(TAG, "Invalid password length\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if (strlen((char *)p->bssid) > MAX_MAC_STR_LEN) {
			ESP_LOGE(TAG, "Invalid BSSID length\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->ssid  = (char *)&p->ssid;
		req_payload->pwd   = (char *)&p->pwd;
		req_payload->bssid = (char *)&p->bssid;
		req_payload->is_wpa3_supported = p->is_wpa3_supported;
		req_payload->listen_interval = p->listen_interval;
		break;
	} case RPC_ID__Req_SetSoftAPVendorSpecificIE: {
		wifi_softap_vendor_ie_t *p = &app_req->u.wifi_softap_vendor_ie;
		RPC_ALLOC_ASSIGN(RpcReqSetSoftAPVendorSpecificIE,
				req_set_softap_vendor_specific_ie,
				rpc__req__set_soft_apvendor_specific_ie__init);

		if ((p->type > WIFI_VND_IE_TYPE_ASSOC_RESP) ||
		    (p->type < WIFI_VND_IE_TYPE_BEACON)) {
			ESP_LOGE(TAG, "Invalid vendor ie type \n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((p->idx > WIFI_VND_IE_ID_1) || (p->idx < WIFI_VND_IE_ID_0)) {
			ESP_LOGE(TAG, "Invalid vendor ie ID index \n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if (!p->vnd_ie.payload) {
			ESP_LOGE(TAG, "Invalid vendor IE buffer \n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->enable = p->enable;
		req_payload->type = (RpcVendorIEType) p->type;
		req_payload->idx = (RpcVendorIEID) p->idx;

		req_payload->vendor_ie_data = (RpcReqVendorIEData *) \
			g_h.funcs->_h_malloc(sizeof(RpcReqVendorIEData));

		if (!req_payload->vendor_ie_data) {
			ESP_LOGE(TAG, "Mem alloc fail\n");
			*failure_status = RPC_ERR_MEMORY_FAILURE;
			return FAILURE;
		}
		ADD_RPC_BUFF_TO_FREE_LATER(req_payload->vendor_ie_data);

		rpc__req__vendor_iedata__init(req_payload->vendor_ie_data);

		req_payload->vendor_ie_data->element_id = p->vnd_ie.element_id;
		req_payload->vendor_ie_data->length = p->vnd_ie.length;
		req_payload->vendor_ie_data->vendor_oui.data =p->vnd_ie.vendor_oui;
		req_payload->vendor_ie_data->vendor_oui.len = VENDOR_OUI_BUF;

		req_payload->vendor_ie_data->payload.data = p->vnd_ie.payload;
		req_payload->vendor_ie_data->payload.len = p->vnd_ie.length-4;
		break;
	} case RPC_ID__Req_StartSoftAP: {
		hosted_softap_config_t *p = &app_req->u.wifi_softap_config;
		RPC_ALLOC_ASSIGN(RpcReqStartSoftAP, req_start_softap,
				rpc__req__start_soft_ap__init);

		if ((strlen((char *)&p->ssid) > MAX_SSID_LENGTH) ||
		    (!strlen((char *)&p->ssid))) {
			ESP_LOGE(TAG, "Invalid SSID length\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((strlen((char *)&p->pwd) > MAX_PWD_LENGTH) ||
		    ((p->encryption_mode != WIFI_AUTH_OPEN) &&
		     (strlen((char *)&p->pwd) < MIN_PWD_LENGTH))) {
			ESP_LOGE(TAG, "Invalid password length\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((p->channel < MIN_CHNL_NO) ||
		    (p->channel > MAX_CHNL_NO)) {
			ESP_LOGE(TAG, "Invalid softap channel\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((p->encryption_mode < WIFI_AUTH_OPEN) ||
		    (p->encryption_mode == WIFI_AUTH_WEP) ||
		    (p->encryption_mode > WIFI_AUTH_WPA_WPA2_PSK)) {

			ESP_LOGE(TAG, "Asked Encryption mode not supported\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((p->max_connections < MIN_CONN_NO) ||
		    (p->max_connections > MAX_CONN_NO)) {
			ESP_LOGE(TAG, "Invalid maximum connection number\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		if ((p->bandwidth < WIFI_BW_HT20) ||
		    (p->bandwidth > WIFI_BW_HT40)) {
			ESP_LOGE(TAG, "Invalid bandwidth\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->ssid = (char *)&p->ssid;
		req_payload->pwd = (char *)&p->pwd;
		req_payload->chnl = p->channel;
		req_payload->sec_prot = p->encryption_mode;
		req_payload->max_conn = p->max_connections;
		req_payload->ssid_hidden = p->ssid_hidden;
		req_payload->bw = p->bandwidth;
		break;
#endif
	} case RPC_ID__Req_WifiSetPs: {
		wifi_power_save_t * p = &app_req->u.wifi_ps;
		RPC_ALLOC_ASSIGN(RpcReqSetMode, req_wifi_set_ps,
				rpc__req__set_mode__init);

		if ((p->ps_mode < WIFI_PS_MIN_MODEM) ||
		    (p->ps_mode >= WIFI_PS_INVALID)) {
			ESP_LOGE(TAG, "Invalid power save mode\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->mode = p->ps_mode;
		break;
	} case RPC_ID__Req_OTAWrite: {
		ota_write_t *p = & app_req->u.ota_write;
		RPC_ALLOC_ASSIGN(RpcReqOTAWrite, req_ota_write,
				rpc__req__otawrite__init);

		if (!p->ota_data || (p->ota_data_len == 0)) {
			ESP_LOGE(TAG, "Invalid parameter\n");
			*failure_status = RPC_ERR_INCORRECT_ARG;
			return FAILURE;
		}

		req_payload->ota_data.data = p->ota_data;
		req_payload->ota_data.len = p->ota_data_len;
		break;
	} case RPC_ID__Req_WifiSetMaxTxPower: {
		RPC_ALLOC_ASSIGN(RpcReqWifiSetMaxTxPower,
				req_set_wifi_max_tx_power,
				rpc__req__wifi_set_max_tx_power__init);
		req_payload->wifi_max_tx_power = app_req->u.wifi_tx_power.power;
		break;
	} case RPC_ID__Req_ConfigHeartbeat: {
		RPC_ALLOC_ASSIGN(RpcReqConfigHeartbeat, req_config_heartbeat,
				rpc__req__config_heartbeat__init);
		req_payload->enable = app_req->u.e_heartbeat.enable;
		req_payload->duration = app_req->u.e_heartbeat.duration;
		if (req_payload->enable) {
			ESP_LOGW(TAG, "Enable heartbeat with duration %ld\n", (long int)req_payload->duration);
			if (CALLBACK_AVAILABLE != is_event_callback_registered(RPC_ID__Event_Heartbeat))
				ESP_LOGW(TAG, "Note: ** Subscribe heartbeat event to get notification **\n");
		} else {
			ESP_LOGI(TAG, "Disable Heartbeat\n");
		}
		break;
	} case RPC_ID__Req_WifiInit: {
		wifi_init_config_t * p_a = &app_req->u.wifi_init_config;
		RPC_ALLOC_ASSIGN(RpcReqWifiInit, req_wifi_init,
				rpc__req__wifi_init__init);
		RPC_ALLOC_ELEMENT(WifiInitConfig, req_payload->cfg, wifi_init_config__init);

		req_payload->cfg->static_rx_buf_num      = p_a->static_rx_buf_num       ;
		req_payload->cfg->dynamic_rx_buf_num     = p_a->dynamic_rx_buf_num      ;
		req_payload->cfg->tx_buf_type            = p_a->tx_buf_type             ;
		req_payload->cfg->static_tx_buf_num      = p_a->static_tx_buf_num       ;
		req_payload->cfg->dynamic_tx_buf_num     = p_a->dynamic_tx_buf_num      ;
		req_payload->cfg->cache_tx_buf_num       = p_a->cache_tx_buf_num        ;
		req_payload->cfg->csi_enable             = p_a->csi_enable              ;
		req_payload->cfg->ampdu_rx_enable        = p_a->ampdu_rx_enable         ;
		req_payload->cfg->ampdu_tx_enable        = p_a->ampdu_tx_enable         ;
		req_payload->cfg->amsdu_tx_enable        = p_a->amsdu_tx_enable         ;
		req_payload->cfg->nvs_enable             = p_a->nvs_enable              ;
		req_payload->cfg->nano_enable            = p_a->nano_enable             ;
		req_payload->cfg->rx_ba_win              = p_a->rx_ba_win               ;
		req_payload->cfg->wifi_task_core_id      = p_a->wifi_task_core_id       ;
		req_payload->cfg->beacon_max_len         = p_a->beacon_max_len          ;
		req_payload->cfg->mgmt_sbuf_num          = p_a->mgmt_sbuf_num           ;
		req_payload->cfg->sta_disconnected_pm    = p_a->sta_disconnected_pm     ;
		req_payload->cfg->espnow_max_encrypt_num = p_a->espnow_max_encrypt_num  ;
		req_payload->cfg->magic                  = p_a->magic                   ;

		/* uint64 - TODO: portable? */
		req_payload->cfg->feature_caps = p_a->feature_caps                      ;
		break;
    } case RPC_ID__Req_WifiGetConfig: {
		wifi_cfg_t * p_a = &app_req->u.wifi_config;
		RPC_ALLOC_ASSIGN(RpcReqWifiGetConfig, req_wifi_get_config,
				rpc__req__wifi_get_config__init);

		req_payload->iface = p_a->iface;
		break;
    } case RPC_ID__Req_WifiSetConfig: {
		wifi_cfg_t * p_a = &app_req->u.wifi_config;
		RPC_ALLOC_ASSIGN(RpcReqWifiSetConfig, req_wifi_set_config,
				rpc__req__wifi_set_config__init);

		req_payload->iface = p_a->iface;

		RPC_ALLOC_ELEMENT(WifiConfig, req_payload->cfg, wifi_config__init);

		switch(req_payload->iface) {

		case WIFI_IF_STA: {
			req_payload->cfg->u_case = WIFI_CONFIG__U_STA;

			wifi_sta_config_t *p_a_sta = &p_a->u.sta;
			RPC_ALLOC_ELEMENT(WifiStaConfig, req_payload->cfg->sta, wifi_sta_config__init);
			WifiStaConfig *p_c_sta = req_payload->cfg->sta;
			RPC_REQ_COPY_STR(p_c_sta->ssid, p_a_sta->ssid, SSID_LENGTH);

			RPC_REQ_COPY_STR(p_c_sta->password, p_a_sta->password, PASSWORD_LENGTH);

			p_c_sta->scan_method = p_a_sta->scan_method;
			p_c_sta->bssid_set = p_a_sta->bssid_set;

			if (p_a_sta->bssid_set)
				RPC_REQ_COPY_BYTES(p_c_sta->bssid, p_a_sta->bssid, BSSID_LENGTH);

			p_c_sta->channel = p_a_sta->channel;
			p_c_sta->listen_interval = p_a_sta->listen_interval;
			p_c_sta->sort_method = p_a_sta->sort_method;
			RPC_ALLOC_ELEMENT(WifiScanThreshold, p_c_sta->threshold, wifi_scan_threshold__init);
			p_c_sta->threshold->rssi = p_a_sta->threshold.rssi;
			p_c_sta->threshold->authmode = p_a_sta->threshold.authmode;
			RPC_ALLOC_ELEMENT(WifiPmfConfig, p_c_sta->pmf_cfg, wifi_pmf_config__init);
			p_c_sta->pmf_cfg->capable = p_a_sta->pmf_cfg.capable;
			p_c_sta->pmf_cfg->required = p_a_sta->pmf_cfg.required;

			if (p_a_sta->rm_enabled)
				H_SET_BIT(STA_RM_ENABLED_BIT, p_c_sta->bitmask);

			if (p_a_sta->btm_enabled)
				H_SET_BIT(STA_BTM_ENABLED_BIT, p_c_sta->bitmask);

			if (p_a_sta->mbo_enabled)
				H_SET_BIT(STA_MBO_ENABLED_BIT, p_c_sta->bitmask);

			if (p_a_sta->ft_enabled)
				H_SET_BIT(STA_FT_ENABLED_BIT, p_c_sta->bitmask);

			if (p_a_sta->owe_enabled)
				H_SET_BIT(STA_OWE_ENABLED_BIT, p_c_sta->bitmask);

			if (p_a_sta->transition_disable)
				H_SET_BIT(STA_TRASITION_DISABLED_BIT, p_c_sta->bitmask);

			WIFI_CONFIG_STA_SET_RESERVED_VAL(p_a_sta->reserved, p_c_sta->bitmask);

			p_c_sta->sae_pwe_h2e = p_a_sta->sae_pwe_h2e;
			p_c_sta->failure_retry_cnt = p_a_sta->failure_retry_cnt;

			break;
		} case WIFI_IF_AP: {
			req_payload->cfg->u_case = WIFI_CONFIG__U_AP;

			wifi_ap_config_t * p_a_ap = &p_a->u.ap;
			RPC_ALLOC_ELEMENT(WifiApConfig, req_payload->cfg->ap, wifi_ap_config__init);
			WifiApConfig * p_c_ap = req_payload->cfg->ap;

			RPC_REQ_COPY_STR(p_c_ap->ssid, p_a_ap->ssid, SSID_LENGTH);
			RPC_REQ_COPY_STR(p_c_ap->password, p_a_ap->password, PASSWORD_LENGTH);
			p_c_ap->ssid_len = p_a_ap->ssid_len;
			p_c_ap->channel = p_a_ap->channel;
			p_c_ap->authmode = p_a_ap->authmode;
			p_c_ap->ssid_hidden = p_a_ap->ssid_hidden;
			p_c_ap->max_connection = p_a_ap->max_connection;
			p_c_ap->beacon_interval = p_a_ap->beacon_interval;
			p_c_ap->pairwise_cipher = p_a_ap->pairwise_cipher;
			p_c_ap->ftm_responder = p_a_ap->ftm_responder;
			RPC_ALLOC_ELEMENT(WifiPmfConfig, p_c_ap->pmf_cfg, wifi_pmf_config__init);
			p_c_ap->pmf_cfg->capable = p_a_ap->pmf_cfg.capable;
			p_c_ap->pmf_cfg->required = p_a_ap->pmf_cfg.required;
			break;
        } default: {
            ESP_LOGE(TAG, "unexpected wifi iface [%u]\n", p_a->iface);
			break;
        }

        } /* switch */
		break;

    } case RPC_ID__Req_WifiScanStart: {
		wifi_scan_config_t * p_a = &app_req->u.wifi_scan_config.cfg;

		RPC_ALLOC_ASSIGN(RpcReqWifiScanStart, req_wifi_scan_start,
				rpc__req__wifi_scan_start__init);

		req_payload->block = app_req->u.wifi_scan_config.block;
		if (app_req->u.wifi_scan_config.cfg_set) {

			RPC_ALLOC_ELEMENT(WifiScanConfig, req_payload->config, wifi_scan_config__init);

			RPC_ALLOC_ELEMENT(WifiScanTime , req_payload->config->scan_time, wifi_scan_time__init);
			RPC_ALLOC_ELEMENT(WifiActiveScanTime, req_payload->config->scan_time->active, wifi_active_scan_time__init);
			ESP_LOGD(TAG, "scan start4\n");

			WifiScanConfig *p_c = req_payload->config;
			WifiScanTime *p_c_st = NULL;
			wifi_scan_time_t *p_a_st = &p_a->scan_time;

			RPC_REQ_COPY_STR(p_c->ssid, p_a->ssid, SSID_LENGTH);
			RPC_REQ_COPY_STR(p_c->bssid, p_a->bssid, MAC_SIZE_BYTES);
			p_c->channel = p_a->channel;
			p_c->show_hidden = p_a->show_hidden;
			p_c->scan_type = p_a->scan_type;

			p_c_st = p_c->scan_time;

			p_c_st->passive = p_a_st->passive;
			p_c_st->active->min = p_a_st->active.min ;
			p_c_st->active->max = p_a_st->active.max ;
			req_payload->config_set = 1;
		}
		ESP_LOGI(TAG, "Scan start Req\n");

		break;

	} case RPC_ID__Req_WifiScanGetApRecords: {
		RPC_ALLOC_ASSIGN(RpcReqWifiScanGetApRecords, req_wifi_scan_get_ap_records,
				rpc__req__wifi_scan_get_ap_records__init);
		req_payload->number = app_req->u.wifi_scan_ap_list.number;
		break;
	} case RPC_ID__Req_WifiDeauthSta: {
		RPC_ALLOC_ASSIGN(RpcReqWifiDeauthSta, req_wifi_deauth_sta,
				rpc__req__wifi_deauth_sta__init);
		req_payload->aid = app_req->u.wifi_deauth_sta.aid;
		break;
	} case RPC_ID__Req_WifiSetStorage: {
		wifi_storage_t * p = &app_req->u.wifi_storage;
		RPC_ALLOC_ASSIGN(RpcReqWifiSetStorage, req_wifi_set_storage,
				rpc__req__wifi_set_storage__init);
		req_payload->storage = *p;
		break;
	} case RPC_ID__Req_WifiSetBandwidth: {
		RPC_ALLOC_ASSIGN(RpcReqWifiSetBandwidth, req_wifi_set_bandwidth,
				rpc__req__wifi_set_bandwidth__init);
		req_payload->ifx = app_req->u.wifi_bandwidth.ifx;
		req_payload->bw = app_req->u.wifi_bandwidth.bw;
		break;
	} case RPC_ID__Req_WifiGetBandwidth: {
		RPC_ALLOC_ASSIGN(RpcReqWifiGetBandwidth, req_wifi_get_bandwidth,
				rpc__req__wifi_get_bandwidth__init);
		req_payload->ifx = app_req->u.wifi_bandwidth.ifx;
		break;
	} case RPC_ID__Req_WifiSetChannel: {
		RPC_ALLOC_ASSIGN(RpcReqWifiSetChannel, req_wifi_set_channel,
				rpc__req__wifi_set_channel__init);
		req_payload->primary = app_req->u.wifi_channel.primary;
		req_payload->second = app_req->u.wifi_channel.second;
		break;
	} case RPC_ID__Req_WifiSetCountryCode: {
		RPC_ALLOC_ASSIGN(RpcReqWifiSetCountryCode, req_wifi_set_country_code,
				rpc__req__wifi_set_country_code__init);
		RPC_REQ_COPY_BYTES(req_payload->country, (uint8_t *)&app_req->u.wifi_country_code.cc[0], sizeof(app_req->u.wifi_country_code.cc));
		req_payload->ieee80211d_enabled = app_req->u.wifi_country_code.ieee80211d_enabled;
		break;
	} case RPC_ID__Req_WifiSetCountry: {
		RPC_ALLOC_ASSIGN(RpcReqWifiSetCountry, req_wifi_set_country,
				rpc__req__wifi_set_country__init);

		RPC_ALLOC_ELEMENT(WifiCountry, req_payload->country, wifi_country__init);
		RPC_REQ_COPY_BYTES(req_payload->country->cc, (uint8_t *)&app_req->u.wifi_country.cc[0], sizeof(app_req->u.wifi_country.cc));
		req_payload->country->schan        = app_req->u.wifi_country.schan;
		req_payload->country->nchan        = app_req->u.wifi_country.nchan;
		req_payload->country->max_tx_power = app_req->u.wifi_country.max_tx_power;
		req_payload->country->policy       = app_req->u.wifi_country.policy;
		break;
	} default: {
		*failure_status = RPC_ERR_UNSUPPORTED_MSG;
		ESP_LOGE(TAG, "Unsupported RPC Req[%u]",req->msg_id);
		return FAILURE;
		break;
	}

	} /* switch */
	return SUCCESS;
}
