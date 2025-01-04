/*
 * Copyright (c) 2021 Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * This example uses OTAA to join the LoRaWAN network and then sends a
 * "hello world" uplink message periodically and prints out the
 * contents of any downlink message.
 */

#include <stdio.h>
#include <string.h>
#include "hardware/pwm.h"

#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "pico/board-config.h"
#include "tusb.h"

// edit with LoRaWAN Node Region and OTAA settings
#include "config.h"

// pin configuration for SX126x radio modules
const struct lorawan_sx126x_settings sx126x_settings = {
    .spi = {
        .inst   = spi0,
        .mosi   = RADIO_MOSI,
        .miso   = RADIO_MISO,
        .sck    = RADIO_SCLK,
        .nss    = RADIO_NSS
    },

    .reset  = RADIO_RESET,
    .busy   = RADIO_BUSY,
    // sx127x would use dio0 pin, and sx126x dont use it 
    .dio1   = RADIO_DIO_1
};

// OTAA settings
const struct lorawan_otaa_settings otaa_settings = {
    .device_eui = LORAWAN_DEVICE_EUI,
    .app_eui = LORAWAN_APP_EUI,
    .app_key = LORAWAN_APP_KEY,
    .channel_mask = LORAWAN_CHANNEL_MASK};

// variables for receiving data
int receive_length = 0;
uint8_t receive_buffer[242];
uint8_t receive_port = 0;

void pwm_set_rgbw(uint8_t red, uint8_t green, uint8_t blue, uint8_t white)
{
    pwm_set_gpio_level(LED_RED, red << 8);
    pwm_set_gpio_level(LED_GREEN, green << 8);
    pwm_set_gpio_level(LED_BLUE, blue << 8);
    pwm_set_gpio_level(LED_WHITE, white << 8);
}

int main(void)
{
    // --------- Init ---------
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();

    while (!tud_cdc_connected())
    {
        tight_loop_contents();
    }

    sleep_ms(1000);

    printf("Pico LoRaWAN - Hello OTAA\n\n");

    printf("Configuring LEDS ... ");

    // Initialize onboard LED for blinking.
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Tell the LED pins that the PWM is in charge of its value.
    gpio_set_function(LED_RED, GPIO_FUNC_PWM);
    gpio_set_function(LED_GREEN, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE, GPIO_FUNC_PWM);
    gpio_set_function(LED_WHITE, GPIO_FUNC_PWM);

    // Get some sensible defaults for the slice configuration. By default, the counter is allowed to wrap over its maximum range (0 to 2**16-1)
    pwm_config config = pwm_get_default_config();

    // Set divider, reduces counter clock to sysclock/this value
    pwm_config_set_clkdiv(&config, 4.f); // Why 4.f here? Is this a "resolution"-thing?

    // Figure out which slice we just connected to the LED pin, this is done for the other colors below
    uint slice_num_red = pwm_gpio_to_slice_num(LED_RED);

    // Load the configuration into our PWM slice, and set it running.
    // As I understand, this needs to be done for each PWM pin, however we can keep the &config?
    pwm_init(slice_num_red, &config, true);

    uint slice_num_greed = pwm_gpio_to_slice_num(LED_GREEN);
    pwm_init(slice_num_greed, &config, true);

    uint slice_num_blue = pwm_gpio_to_slice_num(LED_BLUE);
    pwm_init(slice_num_blue, &config, true);

    uint slice_num_white = pwm_gpio_to_slice_num(LED_WHITE);
    pwm_init(slice_num_white, &config, true);

    // Turn off all the leds
    pwm_set_rgbw(COLOR_BLACK);

    // Blink POWER led for status
    pwm_set_rgbw(COLOR_RED);
    sleep_ms(50);
    pwm_set_rgbw(COLOR_BLACK);
    sleep_ms(400);

    pwm_set_rgbw(COLOR_GREEN);
    sleep_ms(50);
    pwm_set_rgbw(COLOR_BLACK);
    sleep_ms(400);

    pwm_set_rgbw(COLOR_BLUE);
    sleep_ms(50);
    pwm_set_rgbw(COLOR_BLACK);
    sleep_ms(400);

    pwm_set_rgbw(COLOR_WHITE);
    sleep_ms(50);
    pwm_set_rgbw(COLOR_BLACK);
    sleep_ms(400);

    printf("Ok\n");
    // -------------------

    // uncomment next line to enable debug
    lorawan_debug(true);

    // initialize the LoRaWAN stack
    printf("Configuring LoRaWAN ... ");
    if (lorawan_init_otaa(&sx126x_settings, LORAWAN_REGION, &otaa_settings) < 0)
    {
        printf("failed!!!\n");
        while (1)
        {
            tight_loop_contents();
        }
    }
    else
    {
        printf("Ok\n");
    }

    // Start the join process and wait
    printf("Joining LoRaWAN network ... ");
    lorawan_join();

    while (!lorawan_is_joined())
    {
        // Let the lorwan library process pending events
        lorawan_process();

        // Blink board led for status
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        lorawan_process_timeout_ms(50);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        lorawan_process_timeout_ms(50);
    }

    printf("joined successfully!\n");

    uint32_t last_message_time = 0;
    uint32_t last_blink_time = 0;

    // loop forever
    while (1)
    {

        // let the lorwan library process pending events
        lorawan_process();

        // get the current time and see if 5 seconds have passed
        // since the last message was sent
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Blink onboard led every cycle
        if ((now - last_blink_time) > 1000)
        {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
            lorawan_process_timeout_ms(50);
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
            last_blink_time = now;
        }

        // Send message every 5 seconds
        if ((now - last_message_time) > 120000)
        {
            const char *message = "hello world!";

            // try to send an unconfirmed uplink message
            printf("sending unconfirmed message '%s' ... ", message);
            if (lorawan_send_unconfirmed(message, strlen(message), 2) < 0)
            {
                printf("failed!!!\n");

                // Blink POWER led for status
                pwm_set_rgbw(COLOR_RED);
                lorawan_process_timeout_ms(50);
                pwm_set_rgbw(COLOR_BLACK);
            }
            else
            {
                printf("success!\n");

                // Blink POWER led for status
                pwm_set_rgbw(COLOR_GREEN);
                lorawan_process_timeout_ms(50);
                pwm_set_rgbw(COLOR_BLACK);
            }

            last_message_time = now;
        }

        // check if a downlink message was received
        receive_length = lorawan_receive(receive_buffer, sizeof(receive_buffer), &receive_port);
        if (receive_length > -1)
        {
            printf("received a %d byte message on port %d: ", receive_length, receive_port);

            // Blink POWER led for status
            pwm_set_rgbw(COLOR_BLUE);
            lorawan_process_timeout_ms(100);
            pwm_set_rgbw(COLOR_BLACK);

            for (int i = 0; i < receive_length; i++)
            {
                printf("%02x", receive_buffer[i]);
            }
            printf("\n");
        }
    }

    return 0;
}
