#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_timer.h"

// LVGL 9 Include
#include "lvgl/lvgl.h"

// --- Hardware Configuration ---
#define DISP_W 128
#define DISP_H 64

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_SCL_IO 22
#define SSD1306_ADDR 0x3C

// --- I2C Helpers (Legacy ESP-IDF Driver) ---

static void ssd1306_write_cmd(uint8_t cmd) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (SSD1306_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, 0x00, true); // Control byte: Co=0, D/C#=0 (Command)
    i2c_master_write_byte(handle, cmd, true);
    i2c_master_stop(handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(handle);
}

static void ssd1306_write_data(const uint8_t *data, size_t len) {
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    i2c_master_start(handle);
    i2c_master_write_byte(handle, (SSD1306_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(handle, 0x40, true); // Control byte: Co=0, D/C#=1 (Data)
    i2c_master_write(handle, (uint8_t*)data, len, true);
    i2c_master_stop(handle);
    i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(10));
    i2c_cmd_link_delete(handle);
}

static void ssd1306_init_hardware(void) {
    // Basic SSD1306 Init Sequence
    ssd1306_write_cmd(0xAE); // Display Off
    ssd1306_write_cmd(0x20); // Set Memory Addressing Mode
    ssd1306_write_cmd(0x00); // Horizontal Addressing Mode
    ssd1306_write_cmd(0xB0); // Set Page Start Address for Page Addressing Mode
    ssd1306_write_cmd(0xC8); // Set COM Output Scan Direction
    ssd1306_write_cmd(0x00); // Set Low Column Address
    ssd1306_write_cmd(0x10); // Set High Column Address
    ssd1306_write_cmd(0x40); // Set Start Line Address
    ssd1306_write_cmd(0x81); // Set Contrast Control
    ssd1306_write_cmd(0xFF);
    ssd1306_write_cmd(0xA1); // Set Segment Re-map
    ssd1306_write_cmd(0xA6); // Set Normal/Inverse Display
    ssd1306_write_cmd(0xA8); // Set Multiplex Ratio
    ssd1306_write_cmd(0x3F);
    ssd1306_write_cmd(0xA4); // Entire Display ON (Resume)
    ssd1306_write_cmd(0xD3); // Set Display Offset
    ssd1306_write_cmd(0x00);
    ssd1306_write_cmd(0xD5); // Set Display Clock Divide Ratio/Oscillator Frequency
    ssd1306_write_cmd(0xF0);
    ssd1306_write_cmd(0xD9); // Set Pre-charge Period
    ssd1306_write_cmd(0x22);
    ssd1306_write_cmd(0xDA); // Set COM Pins Hardware Configuration
    ssd1306_write_cmd(0x12);
    ssd1306_write_cmd(0xDB); // Set VCOMH Deselect Level
    ssd1306_write_cmd(0x20);
    ssd1306_write_cmd(0x8D); // Charge Pump Setting
    ssd1306_write_cmd(0x14);
    ssd1306_write_cmd(0xAF); // Display ON
}

// --- LVGL 9 Flush Callback ---

/* * LVGL 9 typically provides a buffer where 1 byte = 8 horizontal pixels (if LV_COLOR_DEPTH 1).
 * SSD1306 expects "Page Addressing": vertical strips of 8 pixels.
 * We must rotate the bits in software before sending.
 */
static void ssd1306_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    // Calculate the width and height of the area being flushed
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);

    // We can only write full pages (rows of 8 pixels height) to SSD1306 easily.
    // Ensure LVGL is configured to render reasonably aligned buffers or handle partials carefully.
    // For simplicity here, we assume full updates or aligned areas.

    for(int y = area->y1; y <= area->y2; y += 8) {
        // Set the page address (SSD1306 has 8 pages for 64px height)
        uint8_t page = y / 8;
        ssd1306_write_cmd(0xB0 + page);
        
        // Set column address to x1
        ssd1306_write_cmd(0x00 + (area->x1 & 0x0F));
        ssd1306_write_cmd(0x10 + ((area->x1 >> 4) & 0x0F));

        uint8_t data_buf[128]; // Max width buffer
        int buf_idx = 0;

        for(int x = area->x1; x <= area->x2; x++) {
            uint8_t byte = 0;
            // Iterate down 8 pixels to form one vertical byte for SSD1306
            for(int bit = 0; bit < 8; bit++) {
                int current_y = y + bit;
                if(current_y > area->y2) break;

                // LVGL 9 (Monochrome): px_map is a bitmap.
                // We need to calculate the byte index and bit offset in the LVGL buffer.
                // px_map is relative to the area being flushed, not the whole screen.
                int map_x = x - area->x1;
                int map_y = current_y - area->y1;
                
                // Calculate position in the linear bitmap (row-major)
                // Stride is width / 8 (bytes per row)
                int stride = (width + 7) / 8; 
                int byte_idx = map_y * stride + (map_x / 8);
                int bit_idx = 7 - (map_x % 8); // MSB is left-most pixel usually

                if((px_map[byte_idx] >> bit_idx) & 1) {
                    byte |= (1 << bit);
                }
            }
            data_buf[buf_idx++] = byte;
        }
        ssd1306_write_data(data_buf, buf_idx);
    }

    lv_display_flush_ready(disp);
}

// --- LVGL Tick Task ---
// LVGL needs to know time has passed.
static void lv_tick_task(void *arg) {
    lv_tick_inc(5); // Tell LVGL that 5ms has passed
}

// --- Main Init ---
void app_main(void)
{
    // 1. I2C Init
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, conf.mode, 0, 0, 0);

    // 2. Hardware Init
    ssd1306_init_hardware();

    // 3. LVGL Init
    lv_init();

    // 4. Create Display Object (LVGL 9 Style)
    lv_display_t * disp = lv_display_create(DISP_W, DISP_H);
    
    // Create buffers. 
    // Recommended size: 1/10 screen or full screen (1024 bytes for 128x64 mono)
    // NOTE: For monochrome, size is in bytes, not pixels, if we pack it. 
    // However, LVGL usually expects size in pixels for the buffer init.
    // Let's allocate a buffer for 128x64 pixels.
    // In Monochrome mode (1 bit per px), 128x64 pixels = 1024 bytes.
    static uint8_t buf1[DISP_W * DISP_H / 8]; 
    
    lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, ssd1306_flush);

    // 5. Create UI
    lv_obj_t *label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "LVGL 9.5\nSSD1306");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

    // 6. Ticking setup (using esp_timer for precision)
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 5000); // 5ms

    // 7. Loop
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
