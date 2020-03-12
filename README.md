# NNC Badge 2019 based packet monitor

At power on packet monitor runs on channel 1.
To switch channels press sensor key on badge.
In packet monitor mode deauth attack detector is enabled.
Serial console supported.
Run help in comsole to get list of commands.

Versions:
* master - uses ESP-IDF SDK 4.x dev version. Not compatible with SDK stable versions. Frozen.
* 3.1 - uses ESP-IDF SDK 3.1 stable. Frozen.
* 4.0 - uses ESP-IDF SDK 4.0 stable. Active.
```
help 
  Print the list of registered commands

version 
  Get version of chip and SDK

reboot 
  Software reset of the chip

startup  clear|show|add|delete|edit|run [-l <line_number>] [-t <text>]
  Startup script editor
  -l, --line=<line_number>  Lime number to edit
  -t, --text=<text>  Text to insert

join  [--timeout=<t>] [<ssid>] [<pass>]
  Join to WiFi AP as a station. If no ssid/password set - try use stored in nv
  --timeout=<t>  Connection timeout, ms
        <ssid>  SSID of AP
        <pass>  PSK of AP

npm  [-lv] [-i <packet_id]
  NoName badge Packets Manager
    -l, --list  List available packets
  -v, --verbose  Verbose/detailed output
  -i, --install=<packet_id  Install packet

targets  [-lc] [-d <id>]...
  Show found APs/Stations
    -l, --list  Show found targets
   -c, --clear  Clear
  -d, --delete=<id>  Remove AP/stations from list by id

pmon  [-v] [-c <channel>] [--start] [--stop]
  Monitor all WiFi packets and show statistics
  -v, --verbose  show statistics in console
  -c, --channel=<channel>  WiFi channel to monitor
       --start  start packet monitor
        --stop  stop packet monitor

test  [-r] -c <channel> [-n <packets]
  Test deauth detector
     -r, --run  run Deauth detector tester
  -c, --channel=<channel>  WiFi channel to test
  -n, --packets=<packets  How many packets to send. By default 100 packets

scan  [-vapsr] [-c <channel>] [-t <seconds>]
  Scan APs/Stations
  -v, --verbose  Verbose to console
     -a, --all  Scans for APs+stations
     -p, --aps  Scans for APs only
  -s, --stations  Scans for Stations only
  -r, --results  Show last scan results
  -c, --channel=<channel>  WiFi channel to scan only. If not used or 0 - scan all channels.
  -t, --time=<seconds>  How long in seconds it should scan.

radar  [-t] [--start] [--stop] [-c <channel>] [-m all|AA:BB:CC:DD:EE:FF] [-p packets to monitor] [-o OLED mode]
  Monitor RSSI of specified targets
       --start  start packets monitor
        --stop  stop packets monitor
  -c, --channel=<channel>  WiFi channel to monitor. If not used or 0 - scan all channels.
  -t, --targets  Monitor selected targets
  -m, --mac=all|AA:BB:CC:DD:EE:FF  Monitor target with specified mac address

ble  [-v] scan|stop|show|clear [-t <seconds>]
  Scan for BLE devices
  -v, --verbose  Verbose to console
  -t, --time=<seconds>  How long in seconds it should scan.

```


