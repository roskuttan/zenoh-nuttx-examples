#include <zenoh-pico.h>
#include <stdio.h>
#include <stdbool.h>

#define ZENOH_CONFIG_MODE "client"
#define ZENOH_CONFIG_CONNECT "serial//dev/ttyS1#baudrate=115200"
#define KEYEXPR "/pong_topic"
#define ZENOH_SESSION_RETRY_DELAY_S 2

void data_handler(z_loaned_sample_t *sample, void *ctx) {
    z_bytes_slice_iterator_t iter = z_bytes_get_slice_iterator(&sample->payload);
    z_view_slice_t slice;
    if (z_bytes_slice_iterator_next(&iter, &slice)) {
        const uint8_t *data = z_slice_data((const z_loaned_slice_t *)&slice);
        size_t len = z_slice_len((const z_loaned_slice_t *)&slice);
        printf("[INFO] Received data (%zu bytes): ", len);
        for (size_t i = 0; i < len; ++i) {
            printf("%c", data[i]);
        }
        printf("\n");
    } else {
        printf("[ERROR] Payload extraction failed!\n");
    }
}

int z_sub(int argc, char *argv[]) {
    z_owned_session_t session;

    // Attempt to open a Zenoh session with retries
    while (true) {
        z_owned_config_t config;
        if (z_config_default(&config) != 0) {
            printf("[ERROR] Failed to create default config.\n");
            return -1;
        }

        zp_config_insert(z_config_loan(&config), Z_CONFIG_MODE_KEY, ZENOH_CONFIG_MODE);
        zp_config_insert(z_config_loan(&config), Z_CONFIG_CONNECT_KEY, ZENOH_CONFIG_CONNECT);

        int result = z_open(&session, &config, NULL);
        if (result == 0) {
            printf("[INFO] Zenoh session opened successfully.\n");
            break;
        }

        printf("[ERROR] Failed to open Zenoh session. Error code: %d\n", result);
        z_sleep_s(ZENOH_SESSION_RETRY_DELAY_S);
    }

    // Start read and lease tasks after session is ready
    zp_task_read_options_t read_options;
    zp_task_read_options_default(&read_options);
    zp_start_read_task(z_session_loan(&session), &read_options);

    zp_task_lease_options_t lease_options;
    zp_task_lease_options_default(&lease_options);
    zp_start_lease_task(z_session_loan(&session), &lease_options);

    // Prepare key expression once
    z_owned_keyexpr_t keyexpr;
    if (z_keyexpr_from_str(&keyexpr, KEYEXPR) != 0) {
        printf("[ERROR] Failed to create key expression from string: %s\n", KEYEXPR);
        z_close(z_session_loan(&session), NULL);
        return -1;
    }

    // Declare subscriber with retry
    z_owned_subscriber_t subscriber;
    int sub_result = -1;

    while (sub_result < 0) {
        z_owned_closure_sample_t callback;
        if (z_closure_sample(&callback, data_handler, NULL, NULL) != 0) {
            printf("[ERROR] Failed to create sample closure callback!\n");
            z_sleep_s(1);
            continue;
        }

        sub_result = z_declare_subscriber(
            z_session_loan(&session),
            &subscriber,
            z_keyexpr_loan(&keyexpr),
            z_closure_sample_move(&callback),
            NULL
        );

        if (sub_result < 0) {
            printf("[ERROR] Failed to declare subscriber! Error code: %d\n", sub_result);
            z_sleep_s(1);
        } else {
            printf("[INFO] Subscriber declared successfully.\n");
        }
    }

    // Main loop: keep running
    while (true) {
        z_sleep_s(1);
    }

    // Cleanup
    z_undeclare_subscriber(z_subscriber_move(&subscriber));
    z_close(z_session_loan(&session), NULL);
    return 0;
}
