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
#include "model_handler.h"

// 外付けLEDのピンを定義
#define RED_LED_NODE DT_NODELABEL(gpio0)
#define RED_LED_PIN 1  // P0.01 に対応

#define GREEN_LED_NODE DT_NODELABEL(gpio0)
#define GREEN_LED_PIN 2  

#define BLUE_LED_NODE DT_NODELABEL(gpio0)
#define BLUE_LED_PIN 3  

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

struct led_info leds[] = {
	{ .dev = DEVICE_DT_GET(RED_LED_NODE),   .pin = RED_LED_PIN,   .name = "Red"   },
	{ .dev = DEVICE_DT_GET(GREEN_LED_NODE), .pin = GREEN_LED_PIN, .name = "Green" },
	{ .dev = DEVICE_DT_GET(BLUE_LED_NODE),  .pin = BLUE_LED_PIN,  .name = "Blue"  },
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

	// 外付けLED制御（配列アクセスで統一）
	if (led_idx >= 0 && led_idx < ARRAY_SIZE(leds)) {
		struct led_info *ext_led = &leds[led_idx];
		if (device_is_ready(ext_led->dev)) {
			gpio_pin_set(ext_led->dev, ext_led->pin, set->on_off);
			printk("%s LED (P0.%02d) -> %d\n", ext_led->name, ext_led->pin, set->on_off);
		}
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
        1,
        BT_MESH_MODEL_LIST(
            BT_MESH_MODEL_CFG_SRV,
            BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
            // BT_MESH_MODEL_ONOFF_SRV(&led_ctx[0].srv)
			),
		BT_MESH_MODEL_LIST(
			BT_MESH_MODEL_VND(SWITCH_COLOR_COMPANY_ID, SWITCH_COLOR_MODEL_ID, switch_color_ops, NULL, NULL),
		)),
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


// // 配列定義のみ（中身はあとで代入）
// static const struct bt_mesh_model sig_models[] = {
//     BT_MESH_MODEL_CFG_SRV,
//     BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
//     BT_MESH_MODEL_ONOFF_SRV(&led_ctx[0].srv),
// };
// static const struct bt_mesh_model vnd_models[1];

// static struct bt_mesh_comp comp = {
//     .cid = CONFIG_BT_COMPANY_ID,
//     .elem = elements,
//     .elem_count = ARRAY_SIZE(elements),
// };

/* --- モデル群定義 --- */
// static const struct bt_mesh_model sig_models[] = {
// 	BT_MESH_MODEL_CFG_SRV,
// 	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
// 	BT_MESH_MODEL_ONOFF_SRV(&led_ctx[0].srv),
// };

static struct bt_mesh_model vnd_models[1];

/* --- Composition定義 --- */
static struct bt_mesh_comp comp = {
	.cid = CONFIG_BT_COMPANY_ID,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};


const struct bt_mesh_comp *model_handler_init(void)
{
	k_work_init_delayable(&attention_blink_work, attention_blink);

	// LED初期化
	for (int i = 0; i < ARRAY_SIZE(leds); i++) {
		if (!device_is_ready(leds[i].dev)) {
			printk("Error: %s LED device not ready.\n", leds[i].name);
			return NULL;
		}

		int ret = gpio_pin_configure(leds[i].dev, leds[i].pin, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			printk("Error: Failed to configure %s LED (err %d)\n", leds[i].name, ret);
			return NULL;
		}
	}

	// Initialize the vendor model properly
	memcpy(&vnd_models[0], &switch_color_model[0], sizeof(struct bt_mesh_model));

	return &comp;
}
