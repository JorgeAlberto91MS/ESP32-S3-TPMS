#include <Arduino.h>
#include <lvgl.h>
#include "src/esp_bsp.h"
#include "src/display.h"
//#include "src/bg_image.c"

struct TireData {
    lv_obj_t * card;
    lv_obj_t * label_p;
    lv_obj_t * bar_p;
    lv_obj_t * label_t;
    lv_obj_t * bar_t;
};

TireData tires[4];
static lv_style_t style_bar_temp;
static lv_style_t style_bar_press;

void setup_styles() {
    // Estilo Temperatura: Rojo arriba (Calor), Azul abajo (Frío)
    lv_style_init(&style_bar_temp);
    lv_style_set_bg_opa(&style_bar_temp, LV_OPA_COVER);
    lv_style_set_bg_grad_dir(&style_bar_temp, LV_GRAD_DIR_VER);
    lv_style_set_bg_color(&style_bar_temp, lv_color_hex(0xFF0000));     
    lv_style_set_bg_grad_color(&style_bar_temp, lv_color_hex(0x0000FF)); 
    
    lv_style_init(&style_bar_press);
    lv_style_set_bg_color(&style_bar_press, lv_color_hex(0x00FFCC));
}

void create_tire_widget(int idx, const char* title, int x, int y) {
    // PANTALLA
    lv_obj_t * scr = lv_scr_act();
    // COLOR DE FONDO PANTALLA
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x999999), 0);

    // Título Superior
    lv_obj_t * Screentitle = lv_label_create(scr);
    lv_label_set_text(Screentitle, "TPMS - MONITOR");
    lv_obj_set_style_text_font(Screentitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(Screentitle, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(Screentitle, LV_ALIGN_TOP_MID, 0, 10);

    // 1. Tarjeta principal - Un poco más alta para dar aire a los textos
    tires[idx].card = lv_obj_create(scr);
    lv_obj_set_size(tires[idx].card, 201, 120); 
    //lv_obj_set_pos(tires[idx].card, x, y);
    lv_obj_set_style_bg_color(tires[idx].card, lv_color_hex(0x111318), 0);
    lv_obj_set_style_border_color(tires[idx].card, lv_color_hex(0x222630), 0);
    lv_obj_set_style_radius(tires[idx].card, 10, 0);
    lv_obj_clear_flag(tires[idx].card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(tires[idx].card, LV_ALIGN_CENTER, x, y);


    // 2. Etiqueta Posición
    lv_obj_t * l_pos = lv_label_create(tires[idx].card);
    lv_label_set_text(l_pos, title);
    lv_obj_set_style_text_font(l_pos, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l_pos, lv_color_hex(0x777777), 0);
    lv_obj_align(l_pos, LV_ALIGN_TOP_LEFT, -5, -5);

    // 3. PRESIÓN (Texto Principal)
    tires[idx].label_p = lv_label_create(tires[idx].card);
    lv_label_set_text(tires[idx].label_p, "--.-");
    lv_obj_set_style_text_font(tires[idx].label_p, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(tires[idx].label_p, lv_color_hex(0xFFFFFF), 0);
    // Alineado al centro-izquierda
    lv_obj_align(tires[idx].label_p, LV_ALIGN_LEFT_MID, 0, -5);

    // 4. Unidad PSI (Debajo de la presión)
    lv_obj_t * l_unit = lv_label_create(tires[idx].card);
    lv_label_set_text(l_unit, "PSI");
    lv_obj_set_style_text_font(l_unit, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l_unit, lv_color_hex(0x00FFCC), 0);
    lv_obj_align_to(l_unit, tires[idx].label_p, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    // 5. TEMPERATURA (Arriba a la derecha)
    tires[idx].label_t = lv_label_create(tires[idx].card);
    lv_label_set_text(tires[idx].label_t, "--C");
    lv_obj_set_style_text_font(tires[idx].label_t, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(tires[idx].label_t, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(tires[idx].label_t, LV_ALIGN_TOP_RIGHT, 0, -5);

    // 6. BARRA TEMP (Vertical) - CORRECCIÓN DE ALINEACIÓN
    tires[idx].bar_t = lv_bar_create(tires[idx].card);
    lv_obj_set_size(tires[idx].bar_t, 10, 65);
    // Cambiado LV_ALIGN_RIGHT_BOTTOM por LV_ALIGN_BOTTOM_RIGHT
    lv_obj_align(tires[idx].bar_t, LV_ALIGN_BOTTOM_RIGHT, 0, 0); 
    lv_obj_add_style(tires[idx].bar_t, &style_bar_temp, LV_PART_INDICATOR);
    lv_bar_set_range(tires[idx].bar_t, 0, 80);

    // 7. BARRA PRESIÓN (Horizontal)
    tires[idx].bar_p = lv_bar_create(tires[idx].card);
    lv_obj_set_size(tires[idx].bar_p, 130, 8);
    lv_obj_align(tires[idx].bar_p, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_add_style(tires[idx].bar_p, &style_bar_press, LV_PART_INDICATOR);
    lv_bar_set_range(tires[idx].bar_p, 0, 50);
}

void update_tire_values() {
    if (bsp_display_lock(0)) {
        for (int i = 0; i < 4; i++) {
            float p = (float)random(280, 360) / 10.0;
            int t = random(15, 75);

            char buf_p[12], buf_t[12];
            snprintf(buf_p, sizeof(buf_p), "%.1f", p);
            snprintf(buf_t, sizeof(buf_t), "%d°C", t);

            lv_label_set_text(tires[i].label_p, buf_p);
            lv_label_set_text(tires[i].label_t, buf_t);

            lv_bar_set_value(tires[i].bar_p, (int)p, LV_ANIM_ON);
            lv_bar_set_value(tires[i].bar_t, t, LV_ANIM_ON);
        }
        bsp_display_unlock();
    }
}

void setup() {
    Serial.begin(115200);
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = EXAMPLE_LCD_QSPI_H_RES * EXAMPLE_LCD_QSPI_V_RES,
        .rotate = LV_DISP_ROT_90, 
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();

    bsp_display_lock(0);
        setup_styles();
        //draw_ui_elements(lv_scr_act());
        
        // --- NUEVAS POSICIONES BASADAS EN TU FOTO ---
        // Separamos las tarjetas a los bordes (X) y las distribuimos en alto (Y)
        // Tarjetas Izquierdas (X=10)
        create_tire_widget(0, "F-IZQUIERDA", -120+5, -60);   // Frontal Izquierda
        create_tire_widget(2, "T-IZQUIERDA", -120+5, 80);  // Trasera Izquierda (Bajamos Y)

        // Tarjetas Derechas (X=170 aprox para dejar el centro libre)
        create_tire_widget(1, "F-DERECHA", 120-5, -60);  // Frontal Derecha
        create_tire_widget(3, "T-DERECHA", 120-5, 80); // Trasera Derecha
        
        // Nota: Si tu pantalla es más ancha, aumenta el 310 a 330 o 350.

        lv_timer_create([](lv_timer_t* t){ update_tire_values(); }, 2000, NULL);
    bsp_display_unlock();
}

void loop() { delay(20); }