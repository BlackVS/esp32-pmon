cmake_minimum_required(VERSION 3.5)
idf_component_register(SRCS 
                            app.cpp 
                            gpio.cpp 
                            leds.cpp 
                            leds_alarm.cpp 
                            nvs.cpp 
                            ws2812_control.cpp
                            btn.cpp
                            console.cpp
                            oled.cpp
                            vfs.cpp
                            wifi.cpp
                            targets.cpp
                            cmd_system.cpp 
                            cmd_monitor.cpp
                            cmd_tester.cpp
                            cmd_scan.cpp
                            cmd_targets.cpp
                            cmd_join.cpp
                            cmd_npm.cpp
                            task.cpp
                        INCLUDE_DIRS 
                            "."
                    )
set( COMPONENT_ADD_LDFLAGS+= -z muldefs)
