/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <dk_buttons_and_leds.h>
#include "model_handler.h"
#include <zephyr/drivers/gpio.h>
#include "switch_color_model.h"

// Define GPIOs from DeviceTree aliases
const struct gpio_dt_spec ext_leds[3] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(ext_led0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(ext_led1), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(ext_led2), gpios, {0}),
};

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set,
		    struct bt_mesh_onoff_status *rsp);

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp);

static const struct bt_mesh_onoff_srv_handlers onoff_handlers = {
	.set = led_set,
	.get = led_get,
};

struct led_ctx {
	struct bt_mesh_onoff_srv srv;
	bool value;
};

// Generic OnOff is only on Element 0 now
static struct led_ctx onoff_srv = {
	.srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers),
};

static void led_status(struct led_ctx *led, struct bt_mesh_onoff_status *status)
{
	status->remaining_time = 0;
	status->target_on_off = led->value;
	status->present_on_off = led->value;
}

static void led_set(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    const struct bt_mesh_onoff_set *set,
		    struct bt_mesh_onoff_status *rsp)
{
	struct led_ctx *led = CONTAINER_OF(srv, struct led_ctx, srv);

	if (set->on_off == led->value) {
		goto respond;
	}

	led->value = set->on_off;
    
    // Using simple Generic OnOff to control the first LED as a fallback/standard test
    if (ext_leds[0].port) {
        gpio_pin_set_dt(&ext_leds[0], set->on_off);
        printk("Generic OnOff -> LED %d\n", set->on_off);
    }

respond:
	if (rsp) {
		led_status(led, rsp);
	}
}

static void led_get(struct bt_mesh_onoff_srv *srv, struct bt_mesh_msg_ctx *ctx,
		    struct bt_mesh_onoff_status *rsp)
{
	struct led_ctx *led = CONTAINER_OF(srv, struct led_ctx, srv);
	led_status(led, rsp);
}

/* Set up a repeating delayed work to blink the DK's LEDs when attention is
 * requested.
 */
static struct k_work_delayable attention_blink_work;
static bool attention;

static void attention_blink(struct k_work *work)
{
	if (attention) {
		dk_set_leds(DK_ALL_LEDS_MSK);
		k_work_reschedule(&attention_blink_work, K_MSEC(30));
	} else {
		dk_set_leds(DK_NO_LEDS_MSK);
	}
}

static void attention_on(const struct bt_mesh_model *mod)
{
	attention = true;
	k_work_reschedule(&attention_blink_work, K_NO_WAIT);
}

static void attention_off(const struct bt_mesh_model *mod)
{
	/* Will stop rescheduling blink timer */
	attention = false;
}

static const struct bt_mesh_health_srv_cb health_srv_cb = {
	.attn_on = attention_on,
	.attn_off = attention_off,
};

static struct bt_mesh_health_srv health_srv = {
	.cb = &health_srv_cb,
};

BT_MESH_HEALTH_PUB_DEFINE(health_pub, 0);

// Single element composition containing Configuration, Health, Generic OnOff, and Vendor Model
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(
        1,
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_CFG_SRV,
            BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
            BT_MESH_MODEL_ONOFF_SRV(&onoff_srv.srv)
        ),
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_VND(SWITCH_COLOR_COMPANY_ID, SWITCH_COLOR_MODEL_ID, switch_color_ops, NULL, NULL)
        )
    ),
};

static struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	printk("--- Custom LED GPIO Initialization Start ---\n");
	// Configure LEDs via DeviceTree
	for (int i = 0; i < ARRAY_SIZE(ext_leds); i++) {
		if (ext_leds[i].port != NULL) {
			if (!gpio_is_ready_dt(&ext_leds[i])) {
				printk("Error: LED %d device not ready.\n", i);
				return NULL;
			}
			int ret = gpio_pin_configure_dt(&ext_leds[i], GPIO_OUTPUT_INACTIVE);
			if (ret < 0) {
				printk("Error: Failed to configure LED %d (err %d)\n", i, ret);
				return NULL;
			}
			printk("Success: LED %d (pin %d) configured as OUTPUT_INACTIVE.\n", i, ext_leds[i].pin);
		} else {
            printk("Warning: LED %d port is NULL. Skipping configuration.\n", i);
        }
	}
	printk("--- Custom LED GPIO Initialization End ---\n");

	return &comp;
}
