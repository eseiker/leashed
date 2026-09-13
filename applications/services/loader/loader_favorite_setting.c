/*
 * The "pin this setting to Favorites" dialog, kept in firmware.
 *
 * The loader menu and lib/toolbox/settings_helpers/submenu_based.c call
 * archive_favorites_handle_setting_pin_unpin(). It used to be linked in with
 * the Archive app, but Archive now runs from the SD card, so the dialog lives
 * here. The favourites-file helpers below are private copies of the ones in
 * applications/main/archive/helpers; keep the file format identical to them.
 */
#include <archive/helpers/archive_favorites.h>

#include <dialogs/dialogs.h>
#include <storage/storage.h>
#include <stdarg.h>

#define FAV_FILE_BUF_LEN 32

static bool favorite_setting_read_line(File* file, FuriString* str_result) {
    furi_string_reset(str_result);
    uint8_t buffer[FAV_FILE_BUF_LEN];
    bool result = false;

    do {
        size_t read_count = storage_file_read(file, buffer, FAV_FILE_BUF_LEN);
        if(storage_file_get_error(file) != FSE_OK) {
            return false;
        }

        for(size_t i = 0; i < read_count; i++) {
            if(buffer[i] == '\n') {
                uint32_t position = storage_file_tell(file);
                if(storage_file_get_error(file) != FSE_OK) {
                    return false;
                }
                position = position - read_count + i + 1;
                storage_file_seek(file, position, true);
                if(storage_file_get_error(file) != FSE_OK) {
                    return false;
                }
                result = true;
                break;
            } else {
                furi_string_push_back(str_result, buffer[i]);
            }
        }

        if(result || read_count == 0) {
            break;
        }
    } while(true);

    return result;
}

static void favorite_setting_append(const char* path, const char* line) {
    Storage* fs_api = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(fs_api);
    if(storage_file_open(file, path, FSAM_WRITE, FSOM_OPEN_APPEND)) {
        storage_file_write(file, line, strlen(line));
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

/* Returns true if a line equal to entry is present in the favourites file. */
static bool favorite_setting_is_present(const char* entry) {
    FuriString* buffer = furi_string_alloc();
    Storage* fs_api = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(fs_api);

    bool found = false;
    if(storage_file_open(file, ARCHIVE_FAV_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        while(favorite_setting_read_line(file, buffer)) {
            if(!furi_string_size(buffer)) continue;
            if(!furi_string_search_str(buffer, entry)) {
                found = true;
                break;
            }
        }
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(buffer);
    return found;
}

/* Rewrites the favourites file without lines matching entry. */
static void favorite_setting_remove(const char* entry) {
    FuriString* buffer = furi_string_alloc();
    Storage* fs_api = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(fs_api);

    if(storage_file_open(file, ARCHIVE_FAV_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        while(favorite_setting_read_line(file, buffer)) {
            if(!furi_string_size(buffer)) continue;
            if(furi_string_search_str(buffer, entry)) {
                furi_string_push_back(buffer, '\n');
                favorite_setting_append(ARCHIVE_FAV_TEMP_PATH, furi_string_get_cstr(buffer));
            }
        }
    }

    storage_file_close(file);
    storage_common_remove(fs_api, ARCHIVE_FAV_PATH);
    storage_common_rename(fs_api, ARCHIVE_FAV_TEMP_PATH, ARCHIVE_FAV_PATH);
    storage_common_remove(fs_api, ARCHIVE_FAV_TEMP_PATH);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    furi_string_free(buffer);
}

void archive_favorites_handle_setting_pin_unpin(const char* app_name, const char* setting) {
    DialogMessage* message = dialog_message_alloc();

    FuriString* entry = furi_string_alloc_printf("/app:setting/%s", app_name);
    if(setting) {
        furi_string_push_back(entry, '/');
        furi_string_cat_str(entry, setting);
    }
    const char* entry_str = furi_string_get_cstr(entry);

    bool is_favorite = favorite_setting_is_present(entry_str);
    dialog_message_set_header(
        message,
        is_favorite ? "Unpin This Setting?" : "Pin This Setting?",
        64,
        0,
        AlignCenter,
        AlignTop);
    dialog_message_set_text(
        message,
        is_favorite ? "It will no longer be\naccessible from the\nFavorites menu" :
                      "It will be accessible from the\nFavorites menu",
        64,
        32,
        AlignCenter,
        AlignCenter);
    dialog_message_set_buttons(
        message, is_favorite ? "Unpin" : "Go back", NULL, is_favorite ? "Keep pinned" : "Pin");

    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessageButton button = dialog_message_show(dialogs, message);
    furi_record_close(RECORD_DIALOGS);

    if(is_favorite && button == DialogMessageButtonLeft) {
        favorite_setting_remove(entry_str);
    } else if(!is_favorite && button == DialogMessageButtonRight) {
        furi_string_push_back(entry, '\n');
        favorite_setting_append(ARCHIVE_FAV_PATH, furi_string_get_cstr(entry));
    }

    furi_string_free(entry);
    dialog_message_free(message);
}
