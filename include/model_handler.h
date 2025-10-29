/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file
 * @brief Model handler
 */

#ifndef MODEL_HANDLER_H__
#define MODEL_HANDLER_H__

#include <zephyr/bluetooth/mesh.h>

#ifdef __cplusplus
extern "C" {
#endif

const struct bt_mesh_comp *model_handler_init(void);

// 外付けLEDの構造体
struct led_info {
	const struct device *dev;
	uint8_t pin;
	const char *name;
};

extern struct led_info leds[];

#ifdef __cplusplus
}
#endif

#endif /* MODEL_HANDLER_H__ */
