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

// 外付けLEDのピンを定義
#define RED_LED_NODE DT_NODELABEL(gpio0)
#define RED_LED_PIN 1  // P0.01 に対応

#define GREEN_LED_NODE DT_NODELABEL(gpio0)
#define GREEN_LED_PIN 2  

#define BLUE_LED_NODE DT_NODELABEL(gpio0)
#define BLUE_LED_PIN 3  

static const struct device *red_led_dev;
static const struct device *green_led_dev;
static const struct device *blue_led_dev;


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

static struct led_ctx led_ctx[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers) },
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	{ .srv = BT_MESH_ONOFF_SRV_INIT(&onoff_handlers) },
#endif
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
	int led_idx = led - &led_ctx[0];

	if (set->on_off == led->value) {
		goto respond;
	}

	led->value = set->on_off;
	dk_set_led(led_idx, set->on_off);

	// 赤LEDとled0が対応
	if (led_idx == 0 && red_led_dev) {
    	gpio_pin_set(red_led_dev, RED_LED_PIN, set->on_off);
    	printk("Red LED (P0.01) -> %d\n", set->on_off);
	}
	// 緑LEDとled1が対応
	if (led_idx == 1 && green_led_dev) {
    	gpio_pin_set(green_led_dev, GREEN_LED_PIN, set->on_off);
    	printk("Green LED (P0.02) -> %d\n", set->on_off);
	}
	// 青LEDとled2が対応
	if (led_idx == 2 && blue_led_dev) {
    	gpio_pin_set(blue_led_dev, BLUE_LED_PIN, set->on_off);
    	printk("Blue LED (P0.03) -> %d\n", set->on_off);
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
	static int idx;
	const uint8_t pattern[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
		BIT(0),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
		BIT(1),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
		BIT(2),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
		BIT(3),
#endif
	};

	if (attention) {
		dk_set_leds(pattern[idx++ % ARRAY_SIZE(pattern)]);
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

static struct bt_mesh_elem elements[] = {
#if DT_NODE_EXISTS(DT_ALIAS(led0))
	BT_MESH_ELEM(
		1, BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_CFG_SRV,
			BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
			BT_MESH_MODEL_ONOFF_SRV(&led_ctx[0].srv)),
		BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led1))
	BT_MESH_ELEM(
		2, BT_MESH_MODEL_LIST(BT_MESH_MODEL_ONOFF_SRV(&led_ctx[1].srv)),
		BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led2))
	BT_MESH_ELEM(
		3, BT_MESH_MODEL_LIST(BT_MESH_MODEL_ONOFF_SRV(&led_ctx[2].srv)),
		BT_MESH_MODEL_NONE),
#endif
#if DT_NODE_EXISTS(DT_ALIAS(led3))
	BT_MESH_ELEM(
		4, BT_MESH_MODEL_LIST(BT_MESH_MODEL_ONOFF_SRV(&led_ctx[3].srv)),
		BT_MESH_MODEL_NONE),
#endif
};

static const struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	// LED初期化
	red_led_dev = DEVICE_DT_GET(RED_LED_NODE);
	if (!device_is_ready(red_led_dev)) {
    	printk("Error: GPIO device not ready.\n");
	}
	gpio_pin_configure(red_led_dev, RED_LED_PIN, GPIO_OUTPUT_INACTIVE);

	green_led_dev = DEVICE_DT_GET(GREEN_LED_NODE);
	if (!device_is_ready(green_led_dev)) {
    	printk("Error: GPIO device not ready.\n");
	}
	gpio_pin_configure(green_led_dev, GREEN_LED_PIN, GPIO_OUTPUT_INACTIVE);

	blue_led_dev = DEVICE_DT_GET(BLUE_LED_NODE);
	if (!device_is_ready(blue_led_dev)) {
    	printk("Error: GPIO device not ready.\n");
	}
	gpio_pin_configure(blue_led_dev, BLUE_LED_PIN, GPIO_OUTPUT_INACTIVE);

	return &comp;
}
