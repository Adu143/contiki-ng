/*
 * Copyright (c) 2015 NXP B.V.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of NXP B.V. nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NXP B.V. AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL NXP B.V. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Theo van Daele <theo.van.daele@nxp.com>
 *
 */
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/routing/routing.h"
#include "coap-engine.h"
#include "dev/leds.h"
#include "button-sensor.h"
#include "pot-sensor.h"
#include <stdio.h>
#include <stdlib.h>

static char content[COAP_MAX_CHUNK_SIZE];
static int content_len = 0;

static void event_sensors_dr1199_handler();
static void get_sensors_dr1199_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_switch_sw1_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_switch_sw2_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_switch_sw3_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_switch_sw4_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_switch_dio8_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_pot_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_d1_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_d2_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_d3_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_d3_1174_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_d6_1174_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void put_post_led_all_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

#define CONTENT_PRINTF(...) { if(content_len < sizeof(content)) { content_len += snprintf(content + content_len, sizeof(content) - content_len, __VA_ARGS__); } }

#define PARSE_SWITCH(POSITION)  if(button_sensor.value(0) & (1 << POSITION)) { \
                                  CONTENT_PRINTF("PRESSED"); \
                                } else { \
                                  CONTENT_PRINTF("RELEASED"); \
                                }

#define SET_LED(LED)            if(atoi((const char *)request_content) != 0) {   \
                                  leds_on(LED);                                  \
                                } else {                                         \
                                  leds_off(LED);                                 \
                                }

/*---------------------------------------------------------------------------*/
PROCESS(start_app, "START_APP");
AUTOSTART_PROCESSES(&sensors_process, &start_app);

/*---------------------------------------------------------------------------*/

/*********** CoAP sensor/ resource *************************************************/

/*******************************************************************/
/* Observable resource and event handler to obtain all sensor data */
/*******************************************************************/
EVENT_RESOURCE(resource_sensors_dr1199,               /* name */
               "obs;title=\"All_DR1199_sensors\"",    /* attributes */
               get_sensors_dr1199_handler,            /* GET handler */
               NULL,                                  /* POST handler */
               NULL,                                  /* PUT handler */
               NULL,                                  /* DELETE handler */
               event_sensors_dr1199_handler);         /* event handler */
static void
get_sensors_dr1199_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == APPLICATION_JSON) {
    content_len = 0;
    CONTENT_PRINTF("{\"DR1199\":[");
    CONTENT_PRINTF("{\"Switch\":\"0x%X\"},", button_sensor.value(0));
    CONTENT_PRINTF("{\"Pot\":\"%d\"}", pot_sensor.value(0));
    CONTENT_PRINTF("]}");
    coap_set_header_content_format(response, APPLICATION_JSON);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
static void
event_sensors_dr1199_handler()
{
  /* Registered observers are notified and will trigger the GET handler to create the response. */
  coap_notify_observers(&resource_sensors_dr1199);
}
/***********************************************/
/* Resource and handler to obtain switch value */
/***********************************************/
RESOURCE(resource_switch_sw1,
         "title=\"SW1\"",
         get_switch_sw1_handler,
         NULL,
         NULL,
         NULL);
static void
get_switch_sw1_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    PARSE_SWITCH(1)
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
RESOURCE(resource_switch_sw2,
         "title=\"SW2\"",
         get_switch_sw2_handler,
         NULL,
         NULL,
         NULL);
static void
get_switch_sw2_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    PARSE_SWITCH(2)
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
RESOURCE(resource_switch_sw3,
         "title=\"SW3\"",
         get_switch_sw3_handler,
         NULL,
         NULL,
         NULL);
static void
get_switch_sw3_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    PARSE_SWITCH(3)
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
RESOURCE(resource_switch_sw4,
         "title=\"SW4\"",
         get_switch_sw4_handler,
         NULL,
         NULL,
         NULL);
static void
get_switch_sw4_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    PARSE_SWITCH(4)
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
RESOURCE(resource_switch_dio8,
         "title=\"DIO8\"",
         get_switch_dio8_handler,
         NULL,
         NULL,
         NULL);
static void
get_switch_dio8_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    PARSE_SWITCH(0)
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
/*******************************************************/
/*  Resource and handler to obtain potentiometer value */
/*******************************************************/
RESOURCE(resource_pot,
         "title=\"Potentiometer\"",
         get_pot_handler,
         NULL,
         NULL,
         NULL);
static void
get_pot_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  content_len = 0;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    CONTENT_PRINTF("%d", pot_sensor.value(0));
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
/************************************/
/* Resource and handler to set leds */
/************************************/
RESOURCE(resource_led_d1,
         "title=\"LED D1 <[0,1]>\"",
         NULL,
         put_post_led_d1_handler,
         put_post_led_d1_handler,
         NULL);
static void
put_post_led_d1_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    SET_LED(LEDS_GREEN)
  }
}
RESOURCE(resource_led_d2,
         "title=\"LED D2 <[0,1]>\"",
         NULL,
         put_post_led_d2_handler,
         put_post_led_d2_handler,
         NULL);
static void
put_post_led_d2_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    SET_LED(LEDS_BLUE)
  }
}
RESOURCE(resource_led_d3,
         "title=\"LED D3 <[0,1]>\"",
         NULL,
         put_post_led_d3_handler,
         put_post_led_d3_handler,
         NULL);
static void
put_post_led_d3_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    SET_LED(LEDS_RED)
  }
}
RESOURCE(resource_led_d3_1174,
         "title=\"LED D3 1174<[0,1]>\"",
         NULL,
         put_post_led_d3_1174_handler,
         put_post_led_d3_1174_handler,
         NULL);
static void
put_post_led_d3_1174_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    SET_LED(LEDS_GP0);
  }
}
RESOURCE(resource_led_d6_1174,
         "title=\"LED D6 1174<[0,1]>\"",
         NULL,
         put_post_led_d6_1174_handler,
         put_post_led_d6_1174_handler,
         NULL);
static void
put_post_led_d6_1174_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    SET_LED(LEDS_GP1);
  }
}
RESOURCE(resource_led_all,
         "title=\"LED All <[0,1]>\"",
         NULL,
         put_post_led_all_handler,
         put_post_led_all_handler,
         NULL);
static void
put_post_led_all_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    if(atoi((const char *)request_content) != 0) {
      leds_on(LEDS_ALL);
    } else {
      leds_off(LEDS_ALL);
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(start_app, ev, data)
{
  PROCESS_BEGIN();

  static int is_coordinator = 0;

  /* is_coordinator = node_id == 1; */

  /* Make sensor active for measuring */
  SENSORS_ACTIVATE(button_sensor);
  SENSORS_ACTIVATE(pot_sensor);

  /* Start net stack */
  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }
  NETSTACK_MAC.on();
  printf("Starting RPL node\n");

  coap_engine_init();
  coap_activate_resource(&resource_switch_sw1, "DR1199/Switch/SW1");
  coap_activate_resource(&resource_switch_sw2, "DR1199/Switch/SW2");
  coap_activate_resource(&resource_switch_sw3, "DR1199/Switch/SW3");
  coap_activate_resource(&resource_switch_sw4, "DR1199/Switch/SW4");
  coap_activate_resource(&resource_switch_dio8, "DR1199/Switch/DIO8");
  coap_activate_resource(&resource_pot, "DR1199/Potentiometer");
  coap_activate_resource(&resource_led_d1, "DR1199/LED/D1");
  coap_activate_resource(&resource_led_d2, "DR1199/LED/D2");
  coap_activate_resource(&resource_led_d3, "DR1199/LED/D3");
  coap_activate_resource(&resource_led_d3_1174, "DR1199/LED/D3On1174");
  coap_activate_resource(&resource_led_d6_1174, "DR1199/LED/D6On1174");
  coap_activate_resource(&resource_led_all, "DR1199/LED/All");
  coap_activate_resource(&resource_sensors_dr1199, "DR1199/AllSensors");
  /* If sensor process generates an event, call event_handler of resource.
     This will make this resource observable by the client */
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL((ev == sensors_event) &&
                             ((data == &button_sensor) || (data == &pot_sensor)));
    event_sensors_dr1199_handler();
  }

  PROCESS_END();
}
