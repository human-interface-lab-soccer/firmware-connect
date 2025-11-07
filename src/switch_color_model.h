#pragma once
#include <zephyr/bluetooth/mesh.h>

#define SWITCH_COLOR_COMPANY_ID 0xFFFF  // わかるまでの仮設定
#define SWITCH_COLOR_MODEL_ID   0x0001  

// Vendor Model用関数・変数の宣言
int switch_color_msg_handler(const struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf);

extern const struct bt_mesh_model_op switch_color_ops[];
extern struct bt_mesh_model switch_color_model[];
