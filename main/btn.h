#pragma once

#include "driver/touch_pad.h"

#define TOUCHPAD_DBLCLICK_DELAY  500  // MS uses 500ms
#define TOUCHPAD_LONGPRESS_DELAY 1000 // 1s 

typedef enum {
  TOUCHPAD_EVENT_DOWN =0,  //Down != Click
  TOUCHPAD_EVENT_UP,  
  TOUCHPAD_EVENT_CLICK,  
  TOUCHPAD_EVENT_DBLCLICK,  
  TOUCHPAD_EVENT_LONGPRESS,  
  //
  TOUCHPAD_EVENTS
} TOUCHPAD_EVENT;

typedef enum {
  TOUCHPAD_STATE_DISABLED=-1,
  TOUCHPAD_STATE_OFF=0,  
  TOUCHPAD_STATE_ON,  
  //
  TOUCHPAD_STATES
} TOUCHPAD_STATE;

//to use with xTaskCreate
void touchpad_task(void *pvParameters);

// should be called only once
void touchpad_task_init(void);

// event handlers
typedef void (*TOUCHPAD_EVENT_HANDLER)(TOUCHPAD_EVENT, int value);

void touchpad_add_handler(TOUCHPAD_EVENT, TOUCHPAD_EVENT_HANDLER);

// check state
TOUCHPAD_STATE touchpad_get_state(void);