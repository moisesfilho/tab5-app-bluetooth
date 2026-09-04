/**
 * @file main.c
 * @brief Aplicativo Gerenciador de Bluetooth Desacoplado para Tab5 OS
 */

#include "tab5_sdk.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BT_DEVICES 16

static tab5_bt_dev_t s_devs[MAX_BT_DEVICES] = {0};
static tab5_ui_obj_t s_dev_btn_handles[MAX_BT_DEVICES] = {0};
static uint32_t s_dev_count = 0;
static char s_selected_mac[18] = {0};
static char s_selected_name[64] = {0};
static uint32_t s_selected_type = 0;

static tab5_ui_obj_t s_sw_bt = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_lbl_status = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_scan = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_lbl_selected_dev = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_connect = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_disconnect = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_btn_forget = TAB5_UI_INVALID_OBJ;
static tab5_ui_obj_t s_list_devices = TAB5_UI_INVALID_OBJ;

static void update_bt_status_view(void)
{
    tab5_bt_info_t info = {0};
    tab5_err_t err = tab5_system_get_bt_status(&info);

    if (s_lbl_status != TAB5_UI_INVALID_OBJ) {
        if (err == TAB5_OK && info.is_enabled) {
            char status_buf[128];
            snprintf(status_buf, sizeof(status_buf), "Ativo | Conexoes ativas: %d", info.connected_devices);
            tab5_ui_label_set_text(s_lbl_status, status_buf);
            tab5_ui_obj_set_style_text_color(s_lbl_status, 0x22C55E, 255);
        } else {
            tab5_ui_label_set_text(s_lbl_status, "Status: Desativado / Standby");
            tab5_ui_obj_set_style_text_color(s_lbl_status, 0x94A3B8, 255);
        }
    }

    if (s_lbl_selected_dev != TAB5_UI_INVALID_OBJ) {
        if (s_selected_mac[0]) {
            char sel_buf[128];
            snprintf(sel_buf, sizeof(sel_buf), "Dispositivo: %s (%s)",
                     s_selected_name[0] ? s_selected_name : "Desconhecido", s_selected_mac);
            tab5_ui_label_set_text(s_lbl_selected_dev, sel_buf);
        } else {
            tab5_ui_label_set_text(s_lbl_selected_dev, "Dispositivo: Nenhum selecionado");
        }
    }
}

static void scan_bt_devices(void)
{
    if (s_list_devices == TAB5_UI_INVALID_OBJ) {
        return;
    }
    tab5_ui_obj_clean(s_list_devices);
    s_dev_count = 0;

    tab5_ui_show_toast("Buscando dispositivos BLE...", 1200);

    tab5_err_t err = tab5_bt_scan(s_devs, MAX_BT_DEVICES, &s_dev_count);
    if (err == TAB5_OK && s_dev_count > 0) {
        for (uint32_t i = 0; i < s_dev_count; i++) {
            char label_buf[128];
            const char *sym = LV_SYMBOL_BLUETOOTH;
            if (s_devs[i].type == 1) {
                sym = LV_SYMBOL_KEYBOARD;
            } else if (s_devs[i].type == 3) {
                sym = LV_SYMBOL_AUDIO;
            }

            snprintf(label_buf, sizeof(label_buf), "%s (%s) %d dBm %s",
                     s_devs[i].name[0] ? s_devs[i].name : "Dispositivo BLE",
                     s_devs[i].mac, s_devs[i].rssi,
                     s_devs[i].connected ? "[Conectado]" : (s_devs[i].paired ? "[Pareado]" : ""));

            tab5_ui_obj_t btn = tab5_ui_list_add_btn(s_list_devices, sym, label_buf);
            if (btn != TAB5_UI_INVALID_OBJ) {
                s_dev_btn_handles[i] = btn;
            }
        }
    } else {
        tab5_ui_list_add_btn(s_list_devices, LV_SYMBOL_CLOSE, "Nenhum dispositivo BLE encontrado");
    }
}

static void on_select_device(uint32_t index)
{
    if (index >= s_dev_count) {
        return;
    }
    strncpy(s_selected_mac, s_devs[index].mac, sizeof(s_selected_mac) - 1);
    strncpy(s_selected_name, s_devs[index].name, sizeof(s_selected_name) - 1);
    s_selected_type = s_devs[index].type;
    update_bt_status_view();
    tab5_sound_play_beep(1200, 30);
    tab5_ui_show_toast("Dispositivo selecionado", 1000);
}

static void on_connect_clicked(void)
{
    if (s_selected_mac[0] == '\0') {
        tab5_ui_show_toast("Selecione um dispositivo primeiro", 1500);
        return;
    }

    tab5_ui_show_toast("Pareando e conectando...", 2000);
    tab5_bt_connect(s_selected_mac, s_selected_name, s_selected_type);
    tab5_sound_play_beep(1000, 30);
    update_bt_status_view();
}

static void on_disconnect_clicked(void)
{
    if (s_selected_mac[0] != '\0') {
        tab5_bt_disconnect(s_selected_mac);
        tab5_sound_play_beep(800, 30);
        tab5_ui_show_toast("Dispositivo desconectado", 1000);
        update_bt_status_view();
    }
}

static void on_forget_clicked(void)
{
    if (s_selected_mac[0] != '\0') {
        tab5_bt_forget(s_selected_mac);
        tab5_sound_play_beep(600, 40);
        tab5_ui_show_toast("Pareamento removido", 1200);
        s_selected_mac[0] = '\0';
        s_selected_name[0] = '\0';
        update_bt_status_view();
    }
}

static void build_bt_ui(void)
{
    uint32_t pal_surface = tab5_ui_theme_get_color(TAB5_UI_COLOR_SURFACE);
    uint32_t pal_surface_alt = tab5_ui_theme_get_color(TAB5_UI_COLOR_SURFACE_ALT);
    uint32_t pal_border = tab5_ui_theme_get_color(TAB5_UI_COLOR_BORDER);
    uint32_t pal_text = tab5_ui_theme_get_color(TAB5_UI_COLOR_TEXT);
    uint32_t pal_text_muted = tab5_ui_theme_get_color(TAB5_UI_COLOR_TEXT_MUTED);
    uint32_t pal_accent = tab5_ui_theme_get_color(TAB5_UI_COLOR_ACCENT);

    tab5_ui_obj_t scr = tab5_ui_get_screen();

    tab5_ui_obj_t main_cont = tab5_ui_container_create(scr);
    tab5_ui_obj_set_size(main_cont, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_align(main_cont, TAB5_UI_ALIGN_TOP_MID, 0, 104);
    tab5_ui_obj_set_flex_flow(main_cont, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(main_cont, 0, 0);
    tab5_ui_obj_set_style_border(main_cont, 0, 0);
    tab5_ui_obj_set_pad(main_cont, 14);
    tab5_ui_obj_set_gap(main_cont, 14);

    // 1. Card Status & Hardware
    tab5_ui_obj_t card_stat = tab5_ui_container_create(main_cont);
    tab5_ui_obj_set_size(card_stat, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(card_stat, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(card_stat, pal_surface, 255);
    tab5_ui_obj_set_style_border(card_stat, pal_border, 1);
    tab5_ui_obj_set_style_radius(card_stat, 12);
    tab5_ui_obj_set_pad(card_stat, 16);
    tab5_ui_obj_set_gap(card_stat, 12);

    tab5_ui_obj_t row_sw = tab5_ui_container_create(card_stat);
    tab5_ui_obj_set_size(row_sw, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(row_sw, TAB5_UI_FLEX_FLOW_ROW);
    tab5_ui_obj_set_style_bg(row_sw, 0, 0);
    tab5_ui_obj_set_style_border(row_sw, 0, 0);

    tab5_ui_obj_t lbl_h_title = tab5_ui_label_create(row_sw, LV_SYMBOL_BLUETOOTH "  CONTROLADOR BLUETOOTH");
    tab5_ui_obj_set_style_text_color(lbl_h_title, pal_accent, 255);
    tab5_ui_obj_set_flex_grow(lbl_h_title, 1);

    s_sw_bt = tab5_ui_switch_create(row_sw);
    tab5_ui_switch_set_state(s_sw_bt, tab5_bt_is_enabled());

    s_lbl_status = tab5_ui_label_create(card_stat, "Status: Verificando...");
    tab5_ui_obj_set_style_text_color(s_lbl_status, pal_text_muted, 255);

    s_btn_scan = tab5_ui_btn_create(card_stat, LV_SYMBOL_REFRESH "  Escanear Dispositivos Proximos");
    tab5_ui_obj_set_style_bg(s_btn_scan, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_scan, pal_text, 255);

    // 2. Card Dispositivo Selecionado
    tab5_ui_obj_t card_dev = tab5_ui_container_create(main_cont);
    tab5_ui_obj_set_size(card_dev, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(card_dev, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(card_dev, pal_surface, 255);
    tab5_ui_obj_set_style_border(card_dev, pal_border, 1);
    tab5_ui_obj_set_style_radius(card_dev, 12);
    tab5_ui_obj_set_pad(card_dev, 16);
    tab5_ui_obj_set_gap(card_dev, 12);

    s_lbl_selected_dev = tab5_ui_label_create(card_dev, "Dispositivo: Nenhum selecionado");
    tab5_ui_obj_set_style_text_color(s_lbl_selected_dev, pal_text, 255);

    tab5_ui_obj_t row_act = tab5_ui_container_create(card_dev);
    tab5_ui_obj_set_size(row_act, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(row_act, TAB5_UI_FLEX_FLOW_ROW);
    tab5_ui_obj_set_style_bg(row_act, 0, 0);
    tab5_ui_obj_set_style_border(row_act, 0, 0);
    tab5_ui_obj_set_gap(row_act, 10);

    s_btn_connect = tab5_ui_btn_create(row_act, LV_SYMBOL_OK " Conectar / Parear");
    tab5_ui_obj_set_style_bg(s_btn_connect, pal_accent, 255);
    tab5_ui_obj_set_style_text_color(s_btn_connect, 0xFFFFFF, 255);
    tab5_ui_obj_set_flex_grow(s_btn_connect, 2);

    s_btn_disconnect = tab5_ui_btn_create(row_act, LV_SYMBOL_CLOSE " Desconectar");
    tab5_ui_obj_set_style_bg(s_btn_disconnect, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_disconnect, pal_text, 255);
    tab5_ui_obj_set_flex_grow(s_btn_disconnect, 1);

    s_btn_forget = tab5_ui_btn_create(row_act, LV_SYMBOL_TRASH " Esquecer");
    tab5_ui_obj_set_style_bg(s_btn_forget, pal_surface_alt, 255);
    tab5_ui_obj_set_style_text_color(s_btn_forget, pal_text, 255);
    tab5_ui_obj_set_flex_grow(s_btn_forget, 1);

    // 3. Card Lista de Dispositivos
    tab5_ui_obj_t card_list = tab5_ui_container_create(main_cont);
    tab5_ui_obj_set_size(card_list, TAB5_UI_PCT(100), TAB5_UI_SIZE_CONTENT);
    tab5_ui_obj_set_flex_flow(card_list, TAB5_UI_FLEX_FLOW_COLUMN);
    tab5_ui_obj_set_style_bg(card_list, pal_surface, 255);
    tab5_ui_obj_set_style_border(card_list, pal_border, 1);
    tab5_ui_obj_set_style_radius(card_list, 12);
    tab5_ui_obj_set_pad(card_list, 16);
    tab5_ui_obj_set_gap(card_list, 10);

    tab5_ui_obj_t lbl_list_t = tab5_ui_label_create(card_list, LV_SYMBOL_SETTINGS "  DISPOSITIVOS ENCONTRADOS");
    tab5_ui_obj_set_style_text_color(lbl_list_t, pal_accent, 255);

    s_list_devices = tab5_ui_list_create(card_list);
    tab5_ui_obj_set_size(s_list_devices, TAB5_UI_PCT(100), 220);

    scan_bt_devices();
    update_bt_status_view();
}

static void app_init(void)
{
    tab5_system_log(2, "tab5_bt", "Aplicativo Bluetooth iniciado");
    tab5_ui_app_bar_set_title("Bluetooth");
    build_bt_ui();
}

static void app_resume(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth retomado");
    update_bt_status_view();
}

static void app_pause(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth pausado");
}

static void app_destroy(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth finalizado");
}

TAB5_APP_EXPORT void tab5_app_on_theme_changed(bool dark)
{
    (void)dark;
    tab5_ui_clear_content();
    build_bt_ui();
}

TAB5_APP_EXPORT void tab5_app_on_ui_event(tab5_ui_obj_t obj, uint32_t event_type, int32_t event_val)
{
    if (event_type == TAB5_UI_EVENT_VALUE_CHANGED && obj == s_sw_bt) {
        bool en = tab5_ui_switch_get_state(s_sw_bt);
        tab5_bt_set_enabled(en);
        tab5_sound_play_beep(en ? 1200 : 600, 30);
        tab5_ui_show_toast(en ? "Bluetooth Ativado" : "Bluetooth Desativado", 1000);
        update_bt_status_view();
        return;
    }

    if (event_type != TAB5_UI_EVENT_CLICKED) {
        return;
    }

    if (obj == s_btn_scan) {
        scan_bt_devices();
        return;
    } else if (obj == s_btn_connect) {
        on_connect_clicked();
        return;
    } else if (obj == s_btn_disconnect) {
        on_disconnect_clicked();
        return;
    } else if (obj == s_btn_forget) {
        on_forget_clicked();
        return;
    }

    for (uint32_t i = 0; i < s_dev_count; i++) {
        if (obj == s_dev_btn_handles[i]) {
            on_select_device(i);
            return;
        }
    }
}

TAB5_APP_EXPORT int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    tab5_lifecycle_callbacks_t cbs = {
        .on_init = app_init,
        .on_resume = app_resume,
        .on_pause = app_pause,
        .on_destroy = app_destroy,
        .on_open_file = NULL,
    };

    tab5_lifecycle_register(&cbs);
    app_init();
    return 0;
}

