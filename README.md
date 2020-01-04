# NNC Badge 2019 based packet monitor

At power on packet monitor runs on channel 1.

To switch channels press sensor key on badge.

In packet monitor mode deauth attack detector is enabled.

Serial console supported.

Run help in comsole to get list of commands:


```
help 
  Print the list of registered commands

version 
  Get version of chip and SDK

pmon  [-v] [-c <channel>] [--start] [--stop]
  Monitor all WiFi packets and show statistics
  -v, --verbose             show statistics in console
  -c, --channel=<channel>   WiFi channel to monitor
  --start                   start packet monitor
  --stop                    stop packet monitor

scan  [-vapsr] [-c <channel>] [-t <seconds>]
  Scan APs/Stations
  -v, --verbose             Verbose to console
  -a, --all                 Scans for APs+stations
  -p, --aps                 Scans for APs only
  -s, --stations            Scans for Stations only
  -r, --results             Show last scan results
  -c, --channel=<channel>   WiFi channel to scan only. If not used or 0 - scan all channels.
  -t, --time=<seconds>      How long in seconds it should scan.
```


