/**
 * @file main.c
 * @brief Aplicativo Gerenciador de Bluetooth para Tab5 OS
 */

#include "tab5_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void update_bt_view(void)
{
    tab5_bt_info_t info = {0};
    tab5_err_t err = tab5_system_get_bt_status(&info);

    char buf[512];
    if (err == TAB5_OK && info.is_enabled) {
        snprintf(buf, sizeof(buf),
                 "====================================\n"
                 "         STATUS BLUETOOTH          \n"
                 "====================================\n\n"
                 " Controlador: ATIVO\n"
                 " Conexoes:    %d dispositivo(s)\n\n"
                 " Dispositivos pareados:\n"
                 "  [1] Teclado BLE (Pareado)\n"
                 "  [2] Fone de Ouvido BT (Pronto)\n\n"
                 " Pressione Atualizar para buscar novos\n"
                 " acessorios nas proximidades.\n",
                 info.connected_devices);
    } else {
        snprintf(buf, sizeof(buf),
                 "====================================\n"
                 "         STATUS BLUETOOTH          \n"
                 "====================================\n\n"
                 " Controlador: DESATIVADO / STANDBY\n\n"
                 " Ative o modulo Bluetooth para conectar\n"
                 " perifericos e fones de ouvido sem fio.\n");
    }

    tab5_ui_obj_t ta = tab5_ui_get_main_textarea();
    if (ta != NULL) {
        tab5_ui_textarea_set_text(ta, buf);
    }
}

static void on_refresh_clicked(void *user_data)
{
    (void)user_data;
    tab5_sound_play_beep(1400, 30);
    update_bt_view();
    tab5_ui_show_toast("Status Bluetooth atualizado", 1500);
}

static void app_init(void)
{
    tab5_system_log(2, "tab5_bt", "Aplicativo Bluetooth iniciado");
    tab5_ui_app_bar_set_title("Bluetooth");
    tab5_ui_app_bar_add_action_button("LV_SYMBOL_REFRESH", on_refresh_clicked, NULL);
    update_bt_view();
}

static void app_resume(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth retomado");
    update_bt_view();
}

static void app_pause(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth pausado");
}

static void app_destroy(void)
{
    tab5_system_log(2, "tab5_bt", "Bluetooth finalizado");
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
