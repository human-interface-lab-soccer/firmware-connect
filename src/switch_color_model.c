#include "switch_color_model.h"
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <math.h>
#include "model_handler.h"

#define UNICAST_ADDRESS_START 2
#define BATCH_SIZE 4

int switch_color_msg_handler(const struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    char msg[13] = {0};
    size_t len = MIN(buf->len, 12);
    memcpy(msg, buf->data, len);

    // 4桁ごとに分割
    int color_num = 0;
    int color_num2 = 0;
    int color_num3 = 0;

    char buf1[5] = {0}, buf2[5] = {0}, buf3[5] = {0};
    memcpy(buf1, &msg[0],  4);
    memcpy(buf2, &msg[4],  4);
    memcpy(buf3, &msg[8],  4);
    color_num  = atoi(buf1);
    color_num2 = atoi(buf2);
    color_num3 = atoi(buf3);

    // 自ノードのアドレス取得
    uint16_t addr = ctx->addr;  // 受信元のユニキャストアドレスを利用

    // アドレスに基づく相対位置
    int soft_node_address   = (int)addr - UNICAST_ADDRESS_START;
    int color_num_quotient  = soft_node_address / BATCH_SIZE;
    int color_num_remainder = soft_node_address % BATCH_SIZE;

    // 上位桁から割り当てるように、BATCH_SIZE - 1 - remainder で桁位置を反転
    int target_digit = (BATCH_SIZE - 1) - color_num_remainder;

    int new_color_num = 0;

    switch (color_num_quotient) {
        case 0:
            new_color_num = (int)(color_num / pow(10, target_digit)) % 10;
            break;
        case 1:
            new_color_num = (int)(color_num2 / pow(10, target_digit)) % 10;
            break;
        case 2:
            new_color_num = (int)(color_num3 / pow(10, target_digit)) % 10;
            break;
        default:
            new_color_num = 0;
            break;
    }

    printk("Node %d → color_num=%d, color_num2=%d, color_num3=%d\n", addr, color_num, color_num2, color_num3);
    printk(" → quotient=%d remainder=%d digit=%d → command=%d\n",
           color_num_quotient, color_num_remainder, target_digit, new_color_num);

    // 指定色に応じてLEDを制御
    switch (new_color_num) {
        case 0: // 消灯
            gpio_pin_set(leds[0].dev, leds[0].pin, 0);
            gpio_pin_set(leds[1].dev, leds[1].pin, 0);
            gpio_pin_set(leds[2].dev, leds[2].pin, 0);
            printk("LED OFF\n");
            break;

        case 1: // 赤
            gpio_pin_set(leds[0].dev, leds[0].pin, 1);
            gpio_pin_set(leds[1].dev, leds[1].pin, 0);
            gpio_pin_set(leds[2].dev, leds[2].pin, 0);
            printk("LED RED\n");
            break;

        case 2: // 緑
            gpio_pin_set(leds[0].dev, leds[0].pin, 0);
            gpio_pin_set(leds[1].dev, leds[1].pin, 1);
            gpio_pin_set(leds[2].dev, leds[2].pin, 0);
            printk("LED GREEN\n");
            break;

        case 3: // 青
            gpio_pin_set(leds[0].dev, leds[0].pin, 0);
            gpio_pin_set(leds[1].dev, leds[1].pin, 0);
            gpio_pin_set(leds[2].dev, leds[2].pin, 1);
            printk("LED BLUE\n");
            break;

        default:
            printk("Unknown color: %d\n", new_color_num);
            break;
    }

    return 0;
}


// Vendor Modelのオペレーション定義
const struct bt_mesh_model_op switch_color_ops[] = {
    { BT_MESH_MODEL_OP_3(0x01, SWITCH_COLOR_COMPANY_ID), 0, switch_color_msg_handler },
    BT_MESH_MODEL_OP_END,
};

// Vendor Model定義
struct bt_mesh_model switch_color_model[] = {
    BT_MESH_MODEL_VND(SWITCH_COLOR_COMPANY_ID, SWITCH_COLOR_MODEL_ID, switch_color_ops, NULL, NULL),
};
