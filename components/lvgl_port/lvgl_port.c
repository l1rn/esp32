#include "lvgl/lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_display.h"
#include "driver/i2c.h"

#define SSD1306_ADDR 0x3C
#define I2C_MASTER_NUM I2C_NUM_0

void gui_task(void *pvParameter){
	(void) pvParameter;
	while(1){
		lv_timer_handler();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

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

void display_flush(lv_display_t *disp_drv, const lv_area_t *area, u8 *px_map){
	u32 width = lv_area_get_width(area);
	u32 height = lv_area_get_height(area);
	for(int y = area->y1; y <= area->y2; y++){
		u8 page = y / 8;
		
		ssd1306_write_cmd(0x00 + (area->x1 & 0x0F));
		ssd1306_write_cmd(0x10 + ((area->x1 >> 4) & 0x0F));

		u8 data_buf[128];
		int buf_idx = 0;
		for(int x = area->x1; x <= area->x2; x++){
			u8 byte = 0;
			for(int bit = 0; bit < 8; bit++){
				int cur_y = y + bit;
				if(cur_y > area->y2) break;

				int map_x = x - area->x1;
				int map_y = cur_y - area->y1;

				int stride = (width + 7) / 8;
				int byte_idx = map_y * stride + (map_x / 8);
				int bit_idx = 7 - (map_x % 8);
			
				if((px_map[byte_idx] >> bit_idx) & 1) {
					byte |= (1 << bit);
				}
			}
			data_buf[buf_idx++] = byte;
		}
		ssd1306_write_data(data_buf, buf_idx);
	}
	lv_display_flush_ready(disp_drv);
}

void setup_lvgl_display(void){
	lv_init();
	
	lv_display_t *disp = lv_display_create(128, 64);

	static u8 buf[128 * 64 / 8];
	lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
	lv_display_set_flush_cb(disp, display_flush);
	
	xTaskCreatePinnedToCore(gui_task, "gui", 4096, NULL, 5, NULL, 1);
}
