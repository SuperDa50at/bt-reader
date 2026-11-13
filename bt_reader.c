#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <bt/bt_service/bt.h>
#include <profiles/serial_profile.h>
#include <stdio.h>
#include <string.h>

#define TEXT_MAX 8192
#define LINE_MAX 1024
#define COLS     22
#define ROWS     6
#define RX_BUF   512

#define SAVE_DIR  EXT_PATH("apps_data/bt_reader")
#define SAVE_FILE SAVE_DIR "/received.txt"

typedef enum {
    EventInput,
    EventData,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} AppEvent;

typedef struct {
    FuriMutex* mutex;
    FuriMessageQueue* queue;
    ViewPort* vp;
    Storage* storage;
    char text[TEXT_MAX];
    size_t len;
    uint16_t line_start[LINE_MAX];
    size_t lines;
    int scroll;
    bool connected;
    bool dirty;
    uint32_t rx_events;
    uint32_t rx_bytes;
    const char* status;
} App;

static inline bool is_cont(uint8_t c) {
    return (c & 0xC0) == 0x80;
}

static void relayout(App* a) {
    a->lines = 0;
    a->line_start[a->lines++] = 0;
    int col = 0;
    for(size_t i = 0; i < a->len && a->lines < LINE_MAX - 1; i++) {
        uint8_t c = a->text[i];
        if(c == '\n') {
            col = 0;
            a->line_start[a->lines++] = i + 1;
            continue;
        }
        if(is_cont(c)) continue;
        if(col == COLS) {
            col = 0;
            a->line_start[a->lines++] = i;
        }
        col++;
    }
}

static void save_to_sd(App* a) {
    storage_simply_mkdir(a->storage, EXT_PATH("apps_data"));
    storage_simply_mkdir(a->storage, SAVE_DIR);
    File* f = storage_file_alloc(a->storage);
    if(storage_file_open(f, SAVE_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(f, a->text, a->len);
        storage_file_close(f);
        a->status = "Saved to SD";
    } else {
        a->status = "SD write error";
    }
    storage_file_free(f);
    a->dirty = false;
}

static void load_from_sd(App* a) {
    File* f = storage_file_alloc(a->storage);
    if(storage_file_open(f, SAVE_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        a->len = storage_file_read(f, a->text, TEXT_MAX - 1);
        a->text[a->len] = 0;
        storage_file_close(f);
    }
    storage_file_free(f);
    relayout(a);
}

static void delete_from_sd(App* a) {
    storage_simply_remove(a->storage, SAVE_FILE);
    a->len = 0;
    a->text[0] = 0;
    a->scroll = 0;
    a->dirty = false;
    a->status = "Deleted from SD";
    relayout(a);
}

static void draw_cb(Canvas* canvas, void* ctx) {
    App* a = ctx;
    furi_mutex_acquire(a->mutex, FuriWaitForever);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    if(a->len == 0) {
        canvas_draw_str(canvas, 2, 12, a->connected ? "Connected. Send text" : "Waiting for BLE...");
        char info[40];
        snprintf(
            info,
            sizeof(info),
            "RX ev:%lu b:%lu",
            (unsigned long)a->rx_events,
            (unsigned long)a->rx_bytes);
        canvas_draw_str(canvas, 2, 24, info);
        canvas_draw_str(canvas, 2, 38, "Hold OK: delete file");
        canvas_draw_str(canvas, 2, 48, "Up/Down: scroll");
        if(a->status) canvas_draw_str(canvas, 2, 62, a->status);
    } else {
        for(int r = 0; r < ROWS; r++) {
            int li = a->scroll + r;
            if(li >= (int)a->lines) break;
            char buf[COLS * 2 + 2];
            size_t p = a->line_start[li], o = 0;
            int col = 0;
            while(p < a->len && o < sizeof(buf) - 1) {
                uint8_t c = a->text[p];
                if(c == '\n') break;
                if(!is_cont(c)) {
                    if(col == COLS) break;
                    col++;
                }
                buf[o++] = c;
                p++;
            }
            buf[o] = 0;
            canvas_draw_str(canvas, 1, 10 + r * 10, buf);
        }
    }
    furi_mutex_release(a->mutex);
}

static void input_cb(InputEvent* e, void* ctx) {
    AppEvent ev = {.type = EventInput, .input = *e};
    furi_message_queue_put((FuriMessageQueue*)ctx, &ev, 0);
}

static uint16_t rx_cb(SerialServiceEvent event, void* ctx) {
    App* a = ctx;
    furi_mutex_acquire(a->mutex, FuriWaitForever);
    a->rx_events++;
    a->connected = true;
    if(event.event == SerialServiceEventTypeDataReceived) {
        a->rx_bytes += event.data.size;
        for(uint16_t i = 0; i < event.data.size && a->len < TEXT_MAX - 1; i++) {
            char c = event.data.buffer[i];
            if(c == '\r') continue;
            a->text[a->len++] = c;
        }
        a->text[a->len] = 0;
        a->dirty = true;
        relayout(a);
    }
    furi_mutex_release(a->mutex);
    AppEvent ev = {.type = EventData};
    furi_message_queue_put(a->queue, &ev, 0);
    view_port_update(a->vp);
    return RX_BUF;
}

int32_t bt_reader_app(void* p) {
    UNUSED(p);
    App* a = malloc(sizeof(App));
    memset(a, 0, sizeof(App));
    a->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    a->queue = furi_message_queue_alloc(32, sizeof(AppEvent));
    a->storage = furi_record_open(RECORD_STORAGE);
    a->lines = 1;
    load_from_sd(a);

    Bt* bt = furi_record_open(RECORD_BT);
    bt_disconnect(bt);
    furi_delay_ms(200);
    bt_keys_storage_set_storage_path(bt, APP_DATA_PATH(".bt_reader.keys"));
    FuriHalBleProfileBase* profile = bt_profile_start(bt, ble_profile_serial, NULL);
    furi_check(profile);
    ble_profile_serial_set_event_callback(profile, RX_BUF, rx_cb, a);

    a->vp = view_port_alloc();
    view_port_draw_callback_set(a->vp, draw_cb, a);
    view_port_input_callback_set(a->vp, input_cb, a->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, a->vp, GuiLayerFullscreen);

    AppEvent ev;
    bool run = true;
    while(run) {
        FuriStatus s = furi_message_queue_get(a->queue, &ev, 300);
        furi_mutex_acquire(a->mutex, FuriWaitForever);
        if(s == FuriStatusErrorTimeout) {
            if(a->dirty) save_to_sd(a);
        } else if(s == FuriStatusOk && ev.type == EventInput) {
            InputEvent* in = &ev.input;
            int max_scroll = (int)a->lines - ROWS;
            if(max_scroll < 0) max_scroll = 0;
            if(in->type == InputTypeShort || in->type == InputTypeRepeat) {
                if(in->key == InputKeyUp && a->scroll > 0) a->scroll--;
                if(in->key == InputKeyDown && a->scroll < max_scroll) a->scroll++;
                if(in->key == InputKeyBack) run = false;
            } else if(in->type == InputTypeLong && in->key == InputKeyOk) {
                delete_from_sd(a);
            }
        }
        furi_mutex_release(a->mutex);
        view_port_update(a->vp);
    }

    ble_profile_serial_set_event_callback(profile, 0, NULL, NULL);
    furi_mutex_acquire(a->mutex, FuriWaitForever);
    if(a->dirty) save_to_sd(a);
    furi_mutex_release(a->mutex);

    gui_remove_view_port(gui, a->vp);
    furi_record_close(RECORD_GUI);
    view_port_free(a->vp);
    bt_disconnect(bt);
    furi_delay_ms(200);
    bt_keys_storage_set_default_path(bt);
    bt_profile_restore_default(bt);
    furi_record_close(RECORD_BT);
    furi_record_close(RECORD_STORAGE);
    furi_message_queue_free(a->queue);
    furi_mutex_free(a->mutex);
    free(a);
    return 0;
}
