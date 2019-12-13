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

    void scan_APs(void);

protected:
} WiFi;



//void wifi_init(void);

