#include <furi.h>
#include <furi_hal.h>
#include <applications.h>
#include <assets_icons.h>
#include <loader/loader.h>

#include "../desktop_i.h"
#include "../views/desktop_events.h"
#include "../views/desktop_view_main.h"
#include "desktop_scene.h"

/* Archive is an SD-card app on this firmware, not a built-in one. */
#define ARCHIVE_APP_PATH EXT_PATH("apps/Tools/archive.fap")

#define TAG "DesktopSrv"

static void desktop_scene_main_new_idle_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventNewIdleAnimation);
}

static void desktop_scene_main_check_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventCheckAnimation);
}

static void desktop_scene_main_interact_animation_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(
        desktop->view_dispatcher, DesktopAnimationEventInteractAnimation);
}

static inline bool desktop_scene_main_check_none(const char* str) {
    return (str[1] == '\0' && str[0] == '?');
}

static void desktop_scene_main_open_app_or_profile(Desktop* desktop, FavoriteApp* application) {
    bool load_ok = false;
    if(strlen(application->name_or_path) > 0) {
        if(!desktop_scene_main_check_none(application->name_or_path)) {
            // Load app
            loader_start_detached_with_gui_error(desktop->loader, application->name_or_path, NULL);
        }
        load_ok = true;
    }
    // In case of "default" setting
    if(!load_ok) {
        loader_start_detached_with_gui_error(desktop->loader, "Passport", NULL);
    }
}

static void desktop_scene_main_start_favorite(Desktop* desktop, FavoriteApp* application) {
    if(strlen(application->name_or_path) > 0) {
        if(!desktop_scene_main_check_none(application->name_or_path)) {
            loader_start_detached_with_gui_error(desktop->loader, application->name_or_path, NULL);
        }
    } else {
        loader_start_detached_with_gui_error(desktop->loader, LOADER_APPLICATIONS_NAME, NULL);
    }
}

void desktop_scene_main_callback(DesktopEvent event, void* context) {
    Desktop* desktop = (Desktop*)context;
    if(desktop->in_transition) return;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, event);
}

void desktop_scene_main_on_enter(void* context) {
    Desktop* desktop = (Desktop*)context;
    DesktopMainView* main_view = desktop->main_view;

    animation_manager_set_context(desktop->animation_manager, desktop);
    animation_manager_set_new_idle_callback(
        desktop->animation_manager, desktop_scene_main_new_idle_animation_callback);
    animation_manager_set_check_callback(
        desktop->animation_manager, desktop_scene_main_check_animation_callback);
    animation_manager_set_interact_callback(
        desktop->animation_manager, desktop_scene_main_interact_animation_callback);

    desktop_main_set_callback(main_view, desktop_scene_main_callback, desktop);

    view_dispatcher_switch_to_view(desktop->view_dispatcher, DesktopViewIdMain);
}

bool desktop_scene_main_on_event(void* context, SceneManagerEvent event) {
    Desktop* desktop = (Desktop*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        switch(event.event) {
        case DesktopMainEventOpenMenu: {
            Loader* loader = furi_record_open(RECORD_LOADER);
            loader_show_menu(loader);
            furi_record_close(RECORD_LOADER);
            consumed = true;
        } break;

        case DesktopMainEventLock:
            scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneLockMenu, 0);
            desktop_lock(desktop);
            consumed = true;
            break;

        case DesktopMainEventOpenLockMenu:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneLockMenu);
            consumed = true;
            break;

        case DesktopMainEventOpenDebug:
            scene_manager_next_scene(desktop->scene_manager, DesktopSceneDebug);
            consumed = true;
            break;

        case DesktopMainEventOpenArchive:
            loader_start_detached_with_gui_error(desktop->loader, ARCHIVE_APP_PATH, NULL);
            consumed = true;
            break;

        case DesktopMainEventOpenPowerOff: {
            loader_start_detached_with_gui_error(desktop->loader, "Power", "off");
            consumed = true;
            break;
        }

        case DesktopMainEventOpenFavoriteLeftShort:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppLeftShort]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteLeftLong:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppLeftLong]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteRightShort:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppRightShort]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteRightLong:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppRightLong]);
            consumed = true;
            break;
        case DesktopMainEventOpenFavoriteOkLong:
            desktop_scene_main_start_favorite(
                desktop, &desktop->settings.favorite_apps[FavoriteAppOkLong]);
            consumed = true;
            break;

        case DesktopAnimationEventCheckAnimation:
            animation_manager_check_blocking_process(desktop->animation_manager);
            consumed = true;
            break;
        case DesktopAnimationEventNewIdleAnimation:
            animation_manager_new_idle_process(desktop->animation_manager);
            consumed = true;
            break;
        case DesktopAnimationEventInteractAnimation:
            if(!animation_manager_interact_process(desktop->animation_manager)) {
                if(!desktop->settings.dummy_mode) {
                    desktop_scene_main_open_app_or_profile(
                        desktop, &desktop->settings.favorite_apps[FavoriteAppRightShort]);
                } else {
                    desktop_scene_main_open_app_or_profile(
                        desktop, &desktop->settings.dummy_apps[DummyAppRightShort]);
                }
            }
            consumed = true;
            break;

        case DesktopDummyEventOpenLeft:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppLeftShort]);
            break;
        case DesktopDummyEventOpenDown:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppDownShort]);
            break;
        case DesktopDummyEventOpenOk:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppOkShort]);
            break;
        case DesktopDummyEventOpenUpLong:
            if(!desktop_scene_main_check_none(
                   desktop->settings.dummy_apps[DummyAppUpLong].name_or_path)) {
                desktop_scene_main_open_app_or_profile(
                    desktop, &desktop->settings.dummy_apps[DummyAppUpLong]);
            } else {
                scene_manager_set_scene_state(desktop->scene_manager, DesktopSceneLockMenu, 0);
                desktop_lock(desktop);
            }
            break;
        case DesktopDummyEventOpenDownLong:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppDownLong]);
            break;
        case DesktopDummyEventOpenLeftLong:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppLeftLong]);
            break;
        case DesktopDummyEventOpenRightLong:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppRightLong]);
            break;
        case DesktopDummyEventOpenOkLong:
            desktop_scene_main_open_app_or_profile(
                desktop, &desktop->settings.dummy_apps[DummyAppOkLong]);
            break;

        case DesktopLockedEventUpdate:
            desktop_view_locked_update(desktop->locked_view);
            consumed = true;
            break;

        default:
            break;
        }
    }

    return consumed;
}

void desktop_scene_main_on_exit(void* context) {
    Desktop* desktop = (Desktop*)context;

    animation_manager_set_new_idle_callback(desktop->animation_manager, NULL);
    animation_manager_set_check_callback(desktop->animation_manager, NULL);
    animation_manager_set_interact_callback(desktop->animation_manager, NULL);
    animation_manager_set_context(desktop->animation_manager, desktop);
}
