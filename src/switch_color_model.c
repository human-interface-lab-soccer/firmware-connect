#include "switch_color_model.h"
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/bluetooth/mesh/cfg_srv.h>
#include <zephyr/sys/printk.h>
#include "model_handler.h"

#define UNICAST_ADDRESS_START 2

// Set LED pattern based on color number
static void set_led_color(uint8_t color)
{
    // Default to OFF
    int r = 0, g = 0, b = 0;

    switch (color) {
        case 1: r = 1; break; // Red
        case 2: g = 1; break; // Green
        case 3: b = 1; break; // Blue
        case 4: r = 1; g = 1; break; // Yellow/Orange
        case 5: r = 1; b = 1; break; // Magenta
        case 6: g = 1; b = 1; break; // Cyan
        case 7: r = 1; g = 1; b = 1; break; // White
        case 0:
        default:
            break; // OFF
    }

    if (ext_leds[0].port) gpio_pin_set_dt(&ext_leds[0], r);
    if (ext_leds[1].port) gpio_pin_set_dt(&ext_leds[1], g);
    if (ext_leds[2].port) gpio_pin_set_dt(&ext_leds[2], b);
    
    printk("LED Color Set to: %d\n", color);
}

static int handle_color_set(const struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf,
                            bool ack_required)
{
    // Payload length check
    if (buf->len < sizeof(struct switch_color_set_msg)) {
        printk("Error: Invalid payload length %d\n", buf->len);
        return -EINVAL;
    }

    // Cast the binary payload to our struct
    struct switch_color_set_msg *msg = (struct switch_color_set_msg *)buf->data;
    
    // Retrieve node's unicast address
    uint16_t addr = bt_mesh_primary_addr();
    int soft_node_address = (int)addr - 2; // UNICAST_ADDRESS_START

    uint8_t target_color = 0;
    if (soft_node_address >= 0 && soft_node_address < MAX_NODES) {
        target_color = msg->colors[soft_node_address];
    }

    set_led_color(target_color);
    
    // Validate bounds for node address and extract this node's color from array
    if (soft_node_address >= 0 && soft_node_address < MAX_NODES) {
        target_color = msg->colors[soft_node_address];
    } else {
        printk("Warning: Node address %d out of bounds for color array\n", soft_node_address);
    }

    printk("Node %d (soft: %d) → target_color=%d\n", addr, soft_node_address, target_color);

    // Apply color to hardware LEDs
    set_led_color(target_color);

    // Send Status Response if it is an acknowledged message (SET)
    if (ack_required) {
        BT_MESH_MODEL_BUF_DEFINE(rsp, OP_SWITCH_COLOR_STATUS, sizeof(struct switch_color_status_msg));
        bt_mesh_model_msg_init(&rsp, OP_SWITCH_COLOR_STATUS);

        struct switch_color_status_msg status;
        status.status = target_color;
        net_buf_simple_add_mem(&rsp, &status, sizeof(status));

        bt_mesh_model_send(model, ctx, &rsp, NULL, NULL);
    }

    return 0;
}

int switch_color_set_handler(const struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple *buf)
{
    return handle_color_set(model, ctx, buf, true); // ACKあり
}

int switch_color_set_unack_handler(const struct bt_mesh_model *model,
                                   struct bt_mesh_msg_ctx *ctx,
                                   struct net_buf_simple *buf)
{
    return handle_color_set(model, ctx, buf, false); // ACKなし
}

// Vendor Model Operations
const struct bt_mesh_model_op switch_color_ops[] = {
    { OP_SWITCH_COLOR_SET,       BT_MESH_LEN_EXACT(sizeof(struct switch_color_set_msg)), switch_color_set_handler },
    { OP_SWITCH_COLOR_SET_UNACK, BT_MESH_LEN_EXACT(sizeof(struct switch_color_set_msg)), switch_color_set_unack_handler },
    BT_MESH_MODEL_OP_END,
};

// Vendor Model Definition
struct bt_mesh_model switch_color_model[] = {
    BT_MESH_MODEL_VND(SWITCH_COLOR_COMPANY_ID, SWITCH_COLOR_MODEL_ID, switch_color_ops, NULL, NULL),
};
