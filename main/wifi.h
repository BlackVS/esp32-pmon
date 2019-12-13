#pragma once

extern "C" 

typedef enum {
     WiFi_EVENT_UNKNOWN=0,
     WiFi_EVENT_SUBSCRIBED, 
     WiFi_EVENT_UNSUBSCRIBED, 
     WiFi_EVENT_MODE_CHANGING, //starting to change mode
     WiFi_EVENT_MODE_CHANGED, //just changed mode
} WiFi_EVENT;


typedef void (*WiFi_PROMISCUOUS_CALLBACK)(void* buf, wifi_promiscuous_pkt_type_t type);
typedef void (*WiFi_EVENTS_CALLBACK)(WiFi_EVENT ev, uint32_t arg);
#define DEFAULT_SCAN_LIST_SIZE 100


typedef enum {
//compatibale with wifi_mode_t modes
    WiFi_MODE_NONE = 0,  /**< null mode */
    WiFi_MODE_STA,       /**< WiFi station mode */
    WiFi_MODE_AP,        /**< WiFi soft-AP mode */
    WiFi_MODE_APSTA,     /**< WiFi station + soft-AP mode */
//
    WiFi_MODE_PROMISCUOUS,
    //
    WiFi_MODE_MAX
} WiFi_MODES;

typedef struct
{
		uint16_t protocol:2;
		uint16_t type:2;
		uint16_t subtype:4;
		uint16_t to_ds:1;
		uint16_t from_ds:1;
		uint16_t more_frag:1;
		uint16_t retry:1;
		uint16_t pwr_mgmt:1;
		uint16_t more_data:1;
		uint16_t wep:1;
		uint16_t strict:1;
} wifi_header_frame_control_t;

typedef enum
{
		WiFi_MGMT_ASSOCIATION_REQ,
		WiFi_MGMT_ASSOCIATION_RES,
		WiFi_MGMT_REASSOCIATION_REQ,
		WiFi_MGMT_REASSOCIATION_RES,
		WiFi_MGMT_PROBE_REQ,
		WiFi_MGMT_PROBE_RES,
		WiFi_MGMT_NU1,	/* ......................*/
		WiFi_MGMT_NU2,	/* 0110, 0111 not used */
		WiFi_MGMT_BEACON,
		WiFi_MGMT_ATIM,
		WiFi_MGMT_DISASSOCIATION,
		WiFi_MGMT_AUTHENTICATION,
		WiFi_MGMT_DEAUTHENTICATION,
		WiFi_MGMT_ACTION,
		WiFi_MGMT_ACTION_NACK,
} wifi_mgmt_subtypes_t;

typedef struct {
	//uint16_t frame_ctrl;
    wifi_header_frame_control_t frame_ctrl;
	uint16_t duration_id;
	uint8_t mac_to[6]; /* receiver address */
	uint8_t mac_from[6]; /* sender address */
	uint8_t mac_mask[6]; /* filtering address */
	uint16_t sequence_ctrl;
	uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
	wifi_ieee80211_mac_hdr_t hdr;
	uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

class CWiFi{
    WiFi_MODES mode;
    wifi_country_t wf_country;
    uint32_t channel;
    //promiscuous
    WiFi_PROMISCUOUS_CALLBACK _promiscuous_cb;
    WiFi_EVENTS_CALLBACK      _events_cb;
public:
    CWiFi();
    void init(void);

    //mode
    void set_mode(WiFi_MODES m);
    WiFi_MODES get_mode(void);
    const char * mode2str(WiFi_MODES mode);

    //events
    void set_event_handler(WiFi_EVENTS_CALLBACK cb);

    //channel
    void set_channel(uint32_t ch, bool bForce=false);
    uint32_t get_channel(void);

    void set_promiscuous_callback(WiFi_PROMISCUOUS_CALLBACK callback);

    uint32_t scan_APs(void); //return a number of found APs
    int  scan_APs_get_count(void);
    bool scan_APs_get_data(uint32_t idx, wifi_ap_record_t& ap);

public:
    //static tools
    static const char* authmode2str(wifi_auth_mode_t authmode);
    static const char* cipher2str(wifi_cipher_type_t cipher);
protected:
} WiFi;



//void wifi_init(void);

