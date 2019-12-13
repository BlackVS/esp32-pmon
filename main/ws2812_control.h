#ifndef WS2812_CONTROL_H
#define WS2812_CONTROL_H
#include "badge_config.h"
#include <stdint.h>

// Configure these based on your project needs ********
#define LED_RMT_TX_CHANNEL RMT_CHANNEL_0
#define LED_RMT_TX_GPIO PIN_WS2812
// ****************************************************

#define BITS_PER_LED_CMD 24
#define LED_BUFFER_ITEMS ((NUM_LEDS * BITS_PER_LED_CMD))

// These values are determined by measuring pulse timing with logic analyzer and
// adjusting to match datasheet.
#define T0H 14 // 0 bit high time
#define T1H 52 // 1 bit high time
#define TL 52  // low time for either bit

// This structure is used for indicating what the colors of each LED should be
// set to. There is a 32bit value for each LED. Only the lower 3 bytes are used
// and they hold the Red (byte 2), Green (byte 1), and Blue (byte 0) values to
// be set.
struct led_state {
  uint32_t leds[NUM_LEDS];
};

// Setup the hardware peripheral. Only call this once.
void ws2812_control_init(void);

// Update the LEDs to the new state. Call as needed.
// This function will block the current task until the RMT peripheral is
// finished sending the entire sequence.
void ws2812_write_leds(struct led_state new_state);

#endif