#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>
#include <string.h>

#define TEXT_MAX  16384
#define LINE_MAX  2048
#define COLS      22
#define ROWS      5

#define SAVE_DIR  EXT_PATH("apps_data/bt_reader")
#define SAVE_FILE SAVE_DIR "/received.txt"

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

static void load_from_sd(App* a) {
    a->len = 0;
    a->text[0] = 0;
    File* f = storage_file_alloc(a->storage);
    if(storage_file_open(f, SAVE_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t n = storage_file_read(f, a->text, TEXT_MAX - 1);
        for(size_t i = 0; i < n; i++) {
            if(a->text[i] != '\r') a->text[a->len++] = a->text[i];
        }
        a->text[a->len] = 0;
        storage_file_close(f);
        a->status = a->len ? "Loaded" : "File is empty";
    } else {
        a->status = "No received.txt";
    }
    storage_file_free(f);
    a->scroll = 0;
    relayout(a);
}

static void delete_from_sd(App* a) {
    storage_simply_remove(a->storage, SAVE_FILE);
    a->len = 0;
    a->text[0] = 0;
    a->scroll = 0;
    a->status = "Deleted from SD";
    relayout(a);
}

static void draw_cb(Canvas* canvas, void* ctx) {
    App* a = ctx;
    furi_mutex_acquire(a->mutex, FuriWaitForever);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);
    if(a->len == 0) {
        canvas_draw_str(canvas, 2, 10, a->status ? a->status : "No text");
        canvas_draw_str(canvas, 2, 24, "Put received.txt in:");
        canvas_draw_str(canvas, 2, 34, "ext/apps_data/bt_reader");
        canvas_draw_str(canvas, 2, 48, "OK: reload");
        canvas_draw_str(canvas, 2, 58, "Hold OK: delete file");
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
            canvas_draw_str(canvas, 1, 10 + r * 11, buf);
        }
        char pos[24];
        snprintf(pos, sizeof(pos), "%d/%d", a->scroll + 1, (int)a->lines);
        canvas_draw_str(canvas, 1, 63, pos);
    }
    furi_mutex_release(a->mutex);
}

static void input_cb(InputEvent* e, void* ctx) {
    furi_message_queue_put((FuriMessageQueue*)ctx, e, 0);
}

int32_t bt_reader_app(void* p) {
    UNUSED(p);
    App* a = malloc(sizeof(App));
    memset(a, 0, sizeof(App));
    a->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    a->queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    a->storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(a->storage, EXT_PATH("apps_data"));
    storage_simply_mkdir(a->storage, SAVE_DIR);
    a->lines = 1;
    load_from_sd(a);

    a->vp = view_port_alloc();
    view_port_draw_callback_set(a->vp, draw_cb, a);
    view_port_input_callback_set(a->vp, input_cb, a->queue);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, a->vp, GuiLayerFullscreen);

    InputEvent ev;
    bool run = true;
    while(run) {
        if(furi_message_queue_get(a->queue, &ev, FuriWaitForever) != FuriStatusOk) continue;
        furi_mutex_acquire(a->mutex, FuriWaitForever);
        int max_scroll = (int)a->lines - ROWS;
        if(max_scroll < 0) max_scroll = 0;
        if(ev.type == InputTypeShort || ev.type == InputTypeRepeat) {
            if(ev.key == InputKeyUp && a->scroll > 0) a->scroll--;
            if(ev.key == InputKeyDown && a->scroll < max_scroll) a->scroll++;
            if(ev.key == InputKeyLeft) a->scroll = (a->scroll > ROWS) ? a->scroll - ROWS : 0;
            if(ev.key == InputKeyRight)
                a->scroll = (a->scroll + ROWS < max_scroll) ? a->scroll + ROWS : max_scroll;
            if(ev.key == InputKeyOk && ev.type == InputTypeShort) load_from_sd(a);
            if(ev.key == InputKeyBack) run = false;
        } else if(ev.type == InputTypeLong && ev.key == InputKeyOk) {
            delete_from_sd(a);
        }
        furi_mutex_release(a->mutex);
        view_port_update(a->vp);
    }

    gui_remove_view_port(gui, a->vp);
    furi_record_close(RECORD_GUI);
    view_port_free(a->vp);
    furi_record_close(RECORD_STORAGE);
    furi_message_queue_free(a->queue);
    furi_mutex_free(a->mutex);
    free(a);
    return 0;
}
