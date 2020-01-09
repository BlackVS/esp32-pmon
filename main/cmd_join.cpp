#include "app.h"

CJoinTask wifi_join("WiFi join", 4096, 5, &pool_wifi_tasks);

/** Arguments used by 'join' function */
static struct {
  struct arg_int *timeout;
  struct arg_str *ssid;
  struct arg_str *password;
  struct arg_end *end;
} join_args;

static int cmd_join(int argc, char **argv) {
  int nerrors = arg_parse(argc, argv, (void **)&join_args);
  if (nerrors != 0) {
    arg_print_errors(stderr, join_args.end, argv[0]);
    return 1;
  }

  wifi_join.ssid=join_args.ssid->sval[0];
  wifi_join.password=join_args.password->sval[0];
  wifi_join.timeout=join_args.timeout->ival[0];

  wifi_join.start();

  //nvs_settings_setString("wifi-user-ap", join_args.ssid->sval[0]);
  //nvs_settings_setString("wifi-user-pass", join_args.password->sval[0]);
  //printf("WiFi Connected, credentials saved\r\n");

  return 0;
}

void register_cmd_join() 
{
  join_args.timeout = arg_int0(NULL, "timeout", "<t>", "Connection timeout, ms");
  join_args.timeout->ival[0] = 15000; // set default value
  join_args.ssid = arg_str1(NULL, NULL, "<ssid>", "SSID of AP");
  join_args.password = arg_str0(NULL, NULL, "<pass>", "PSK of AP");
  join_args.end = arg_end(2);

  const esp_console_cmd_t join_cmd = {.command = "join",
                                      .help = "Join WiFi AP as a station",
                                      .hint = NULL,
                                      .func = &cmd_join,
                                      .argtable = &join_args};

  ESP_ERROR_CHECK(esp_console_cmd_register(&join_cmd));
}

esp_err_t CJoinTask::_start_internal(void)
{
  esp_err_t err = WiFi.join2AP(ssid.c_str(), password.c_str(), timeout, true);
  if(is_connected()) {
    oled_printf_refresh(0,0,STYLE_NORMAL,"Connected to AP!");
    oled_printf_refresh(0,8,STYLE_NORMAL,"SSID: %s", ssid.c_str());
    /* IP Addr assigned to STA */
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);

    char buf[16];
    esp_ip4addr_ntoa(&ip_info.ip, buf, sizeof(buf));
    oled_printf_refresh(0,16,STYLE_NORMAL,"  IP: %s", buf);

    esp_ip4addr_ntoa(&ip_info.netmask, buf, sizeof(buf));
    oled_printf_refresh(0,24,STYLE_NORMAL,"MASK: %s", buf);

    esp_ip4addr_ntoa(&ip_info.gw, buf, sizeof(buf));
    oled_printf_refresh(0,32,STYLE_NORMAL,"  GW: %s", buf);
  } else {
    oled_printf_refresh(0,0,STYLE_NORMAL,"Failed to connected to AP!");
    oled_printf_refresh(0,8,STYLE_NORMAL," SSID: %s", ssid.c_str());
  }
  return err;
}

esp_err_t CJoinTask::_stop_internal(void)
{
    WiFi.set_mode(WiFi_MODE_NONE); 
    return ESP_OK;
}

bool CJoinTask::is_connected(void)
{
    return WiFi.join2AP_getstate()==WiFi_JOIN2AP_CONNECTED;
}

