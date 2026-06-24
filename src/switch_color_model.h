#pragma once
#include <zephyr/bluetooth/mesh.h>

#define SWITCH_COLOR_COMPANY_ID 0x0059  // Nordic Semiconductor
#define SWITCH_COLOR_MODEL_ID   0x0001  

#define OP_SWITCH_COLOR_SET       BT_MESH_MODEL_OP_3(0x01, SWITCH_COLOR_COMPANY_ID)
#define OP_SWITCH_COLOR_SET_UNACK BT_MESH_MODEL_OP_3(0x02, SWITCH_COLOR_COMPANY_ID)
#define OP_SWITCH_COLOR_STATUS    BT_MESH_MODEL_OP_3(0x03, SWITCH_COLOR_COMPANY_ID)

int switch_color_msg_handler(const struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf);

extern const struct bt_mesh_model_op switch_color_ops[];
extern struct bt_mesh_model switch_color_model[];

#define MAX_NODES 12

// Payload structures (Binary format)
struct switch_color_set_msg {
    uint8_t colors[MAX_NODES];
} __packed;

struct switch_color_status_msg {
    uint8_t status;
} __packed;
