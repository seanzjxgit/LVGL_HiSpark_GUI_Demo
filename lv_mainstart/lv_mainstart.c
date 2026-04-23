#include "lv_mainstart.h"
#include "lvgl.h"



#define COLOR_BG_TOP      lv_color_hex(0x7B3FE4)
#define COLOR_BG_BOT      lv_color_hex(0x00C8FF)
#define COLOR_WHITE       lv_color_hex(0xFFFFFF)
#define COLOR_GLASS_TINT  lv_color_hex(0xC8AAFF)
#define COLOR_SHADOW      lv_color_hex(0x1A0050)

#define PAGE_COUNT          4
#define POSITION_HOME_NUM   4
#define MESSAGE_MEETING_NUM 5
#define COLOR_LEVEL_HIGH    lv_color_hex(0xFF5252)
#define COLOR_LEVEL_MID     lv_color_hex(0xFFD74E)
#define COLOR_LEVEL_LOW     lv_color_hex(0x69FF7A)
#define USER_CARD_NUM       3

LV_IMG_DECLARE(User)
LV_IMG_DECLARE(Man)
LV_IMG_DECLARE(Woman)

typedef enum { GENDER_MALE = 0, GENDER_FEMALE } user_gender_t;

typedef struct {
    char          *name;
    user_gender_t  gender;
    char          *position[POSITION_HOME_NUM];
    uint8_t        position_count;
    bool           valid;
} user_card_t;


char     *usr_name        = NULL;
char     *home_time       = NULL;
char     *home_date       = NULL;
uint8_t   batt_pct        = 0;
char     *position[POSITION_HOME_NUM] = {0};
char     *meeting_time[MESSAGE_MEETING_NUM]    = {0};
char     *meeting_content[MESSAGE_MEETING_NUM] = {0};
char     *level_text[3];
uint8_t   meeting_level[MESSAGE_MEETING_NUM]   = {0};
uint8_t   meeting_order[MESSAGE_MEETING_NUM]   = {0};
static lv_obj_t *meeting_list_obj = NULL;
static lv_obj_t *card_list_obj    = NULL;
static uint8_t   meeting_sort_asc = 0;
static void rebuild_meeting_list(void);
static void rebuild_card_list(void);
static lv_obj_t *sort_priority_btn = NULL;
static lv_obj_t *sort_time_btn_obj = NULL;
static user_card_t user_cards[USER_CARD_NUM];

/* PPT
static uint16_t ppt_current_page = 1;
static uint16_t ppt_total_pages  = 20;
*/

static bool    ctrl_starflash_enabled = false;
static bool    ctrl_bluetooth_enabled = true;
static bool    ctrl_nfc_enabled       = false;
static bool    ctrl_wifi_enabled      = true;
static uint8_t ctrl_brightness        = 75;
static uint8_t ctrl_volume            = 50;

/*
 * static void hw_ppt_prev_page(void);
 * static void hw_ppt_next_page(void);
 * void hw_ppt_get_page_info(uint16_t *current, uint16_t *total);
 * void hw_ppt_set_page_info(uint16_t current, uint16_t total);
 */
static void hw_starflash_set(bool enable);
static void hw_bluetooth_set(bool enable);
static void hw_nfc_set(bool enable);
static void hw_wifi_set(bool enable);
static void hw_brightness_set(uint8_t value);
static void hw_volume_set(uint8_t value);

static lv_obj_t *pages[PAGE_COUNT];
static bool      page_created[PAGE_COUNT];
static uint8_t   current_page = 0;
static lv_obj_t *tab_btns[PAGE_COUNT];

static const char *page_names[PAGE_COUNT] = { "Home", "Meet", "Card", "Ctrl" };
static const char *page_icons[PAGE_COUNT] = {
    LV_SYMBOL_HOME, LV_SYMBOL_VIDEO, LV_SYMBOL_FILE, LV_SYMBOL_SETTINGS
};


static lv_style_t style_screen_bg;
static lv_style_t style_page;
static lv_style_t style_card;
static lv_style_t style_dock_wrap;
static lv_style_t style_dock_btn_normal;
static lv_style_t style_dock_btn_active;

static lv_color_t level_color(uint8_t lv) {
    if(lv == 0) return COLOR_LEVEL_HIGH;
    if(lv == 1) return COLOR_LEVEL_MID;
    return COLOR_LEVEL_LOW;
}

static void setup_styles(void)
{
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, COLOR_BG_TOP);
    lv_style_set_bg_grad_color(&style_screen_bg, COLOR_BG_BOT);
    lv_style_set_bg_grad_dir(&style_screen_bg, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);

    lv_style_init(&style_page);
    lv_style_set_bg_opa(&style_page, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_page, 0);
    lv_style_set_pad_all(&style_page, 0);
    lv_style_set_radius(&style_page, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_WHITE);
    lv_style_set_bg_opa(&style_card, LV_OPA_20);
    lv_style_set_radius(&style_card, 10);
    lv_style_set_border_color(&style_card, COLOR_GLASS_TINT);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_opa(&style_card, LV_OPA_40);
    lv_style_set_shadow_color(&style_card, COLOR_SHADOW);
    lv_style_set_shadow_width(&style_card, 14);
    lv_style_set_shadow_spread(&style_card, 0);
    lv_style_set_shadow_opa(&style_card, LV_OPA_30);
    lv_style_set_shadow_ofs_x(&style_card, 0);
    lv_style_set_shadow_ofs_y(&style_card, 3);
    lv_style_set_pad_all(&style_card, 6);

    lv_style_init(&style_dock_wrap);
    lv_style_set_bg_color(&style_dock_wrap, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_wrap, 64);
    lv_style_set_radius(&style_dock_wrap, LV_RADIUS_CIRCLE);
    lv_style_set_border_color(&style_dock_wrap, COLOR_WHITE);
    lv_style_set_border_width(&style_dock_wrap, 1);
    lv_style_set_border_opa(&style_dock_wrap, LV_OPA_50);
    lv_style_set_shadow_color(&style_dock_wrap, COLOR_SHADOW);
    lv_style_set_shadow_width(&style_dock_wrap, 10);
    lv_style_set_shadow_spread(&style_dock_wrap, 1);
    lv_style_set_shadow_opa(&style_dock_wrap, LV_OPA_30);
    lv_style_set_shadow_ofs_x(&style_dock_wrap, 0);
    lv_style_set_shadow_ofs_y(&style_dock_wrap, 2);
    lv_style_set_pad_hor(&style_dock_wrap, 6);
    lv_style_set_pad_ver(&style_dock_wrap, 3);

    lv_style_init(&style_dock_btn_normal);
    lv_style_set_bg_color(&style_dock_btn_normal, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_btn_normal, LV_OPA_0);
    lv_style_set_border_width(&style_dock_btn_normal, 0);
    lv_style_set_shadow_width(&style_dock_btn_normal, 0);
    lv_style_set_radius(&style_dock_btn_normal, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_dock_btn_normal, 2);
    lv_style_set_text_color(&style_dock_btn_normal, COLOR_WHITE);
    lv_style_set_text_opa(&style_dock_btn_normal, LV_OPA_70);

    lv_style_init(&style_dock_btn_active);
    lv_style_set_bg_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_btn_active, LV_OPA_30);
    lv_style_set_border_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_border_width(&style_dock_btn_active, 1);
    lv_style_set_border_opa(&style_dock_btn_active, LV_OPA_60);
    lv_style_set_shadow_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_shadow_width(&style_dock_btn_active, 6);
    lv_style_set_shadow_opa(&style_dock_btn_active, LV_OPA_40);
    lv_style_set_radius(&style_dock_btn_active, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_dock_btn_active, 2);
    lv_style_set_text_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_text_opa(&style_dock_btn_active, LV_OPA_COVER);
}

static lv_obj_t *create_glass_card(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}


static void create_home_page(lv_obj_t *parent)
{

    lv_obj_t *avatar_card = create_glass_card(parent, 110, 160);
    lv_obj_align(avatar_card, LV_ALIGN_LEFT_MID, 6, 0);

    lv_obj_t *avatar_circle = lv_obj_create(avatar_card);
    lv_obj_set_size(avatar_circle, 76, 76);
    lv_obj_set_style_radius(avatar_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(avatar_circle, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(avatar_circle, LV_OPA_60, 0);
    lv_obj_set_style_border_width(avatar_circle, 0, 0);
    lv_obj_set_style_pad_all(avatar_circle, 0, 0);
    lv_obj_set_style_clip_corner(avatar_circle, true, 0);
    lv_obj_set_scrollbar_mode(avatar_circle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(avatar_circle, LV_DIR_NONE);
    lv_obj_align(avatar_circle, LV_ALIGN_TOP_MID, 0, 6);

    lv_obj_t *av_icon = lv_img_create(avatar_circle);
    lv_img_set_src(av_icon, &User);
    lv_obj_set_size(av_icon, 76, 76);
    lv_img_set_size_mode(av_icon, LV_IMG_SIZE_MODE_REAL);
    lv_img_set_zoom(av_icon, 256);
    lv_img_set_antialias(av_icon, true);
    lv_obj_align(av_icon, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *name_box = lv_obj_create(avatar_card);
    lv_obj_set_size(name_box, 94, 24);
    lv_obj_align(name_box, LV_ALIGN_BOTTOM_MID, 0, -30);
    lv_obj_set_style_bg_color(name_box, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(name_box, LV_OPA_20, 0);
    lv_obj_set_style_border_color(name_box, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(name_box, 1, 0);
    lv_obj_set_style_border_opa(name_box, LV_OPA_40, 0);
    lv_obj_set_style_radius(name_box, 6, 0);
    lv_obj_set_style_pad_all(name_box, 0, 0);
    lv_obj_set_scrollbar_mode(name_box, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *name_box2 = lv_obj_create(avatar_card);
    lv_obj_set_size(name_box2, 94, 24);
    lv_obj_align(name_box2, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(name_box2, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(name_box2, LV_OPA_20, 0);
    lv_obj_set_style_border_color(name_box2, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(name_box2, 1, 0);
    lv_obj_set_style_border_opa(name_box2, LV_OPA_40, 0);
    lv_obj_set_style_radius(name_box2, 6, 0);
    lv_obj_set_style_pad_all(name_box2, 0, 0);
    lv_obj_set_scrollbar_mode(name_box2, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *name_lbl1 = lv_label_create(name_box);
    lv_label_set_text(name_lbl1, usr_name);
    lv_obj_set_style_text_font(name_lbl1, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(name_lbl1, COLOR_WHITE, 0);
    lv_obj_center(name_lbl1);

    lv_obj_t *id_lbl = lv_label_create(name_box2);
    lv_label_set_text(id_lbl, "ID: " THIS_DEVICE_USER_ID);
    lv_obj_set_style_text_font(id_lbl, &lv_font_montserrat_12, -3);
    lv_obj_set_style_text_color(id_lbl, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align_to(id_lbl, name_lbl1, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    lv_obj_t *info_card = create_glass_card(parent, 175, 68);
    lv_obj_align(info_card, LV_ALIGN_TOP_RIGHT, -6, 6);

    lv_obj_t *time_lbl = lv_label_create(info_card);
    lv_label_set_text(time_lbl, home_time);
    lv_obj_set_style_text_color(time_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(time_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *date_lbl = lv_label_create(info_card);
    lv_label_set_text(date_lbl, home_date);
    lv_obj_set_style_text_color(date_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(date_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_opa(date_lbl, LV_OPA_60, 0);
    lv_obj_align_to(date_lbl, time_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);


    #define BATT_FILL_MAX_W 20
    uint8_t fill_w = (uint8_t)((uint32_t)batt_pct * BATT_FILL_MAX_W / 100);
    if(fill_w < 2) fill_w = 2;
    lv_color_t fill_color;
    if(batt_pct > 50)      fill_color = lv_color_hex(0x69FF7A);
    else if(batt_pct > 20) fill_color = lv_color_hex(0xFFD74E);
    else                   fill_color = lv_color_hex(0xFF5252);

    lv_obj_t *batt_border = lv_obj_create(info_card);
    lv_obj_set_size(batt_border, 22, 11);
    lv_obj_align(batt_border, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(batt_border, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(batt_border, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(batt_border, 1, 0);
    lv_obj_set_style_border_opa(batt_border, LV_OPA_80, 0);
    lv_obj_set_style_radius(batt_border, 2, 0);
    lv_obj_set_style_pad_all(batt_border, 0, 0);
    lv_obj_set_scrollbar_mode(batt_border, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_border, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *batt_tip = lv_obj_create(info_card);
    lv_obj_set_size(batt_tip, 3, 5);
    lv_obj_align_to(batt_tip, batt_border, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
    lv_obj_set_style_bg_color(batt_tip, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(batt_tip, LV_OPA_80, 0);
    lv_obj_set_style_border_width(batt_tip, 0, 0);
    lv_obj_set_style_radius(batt_tip, 1, 0);
    lv_obj_set_scrollbar_mode(batt_tip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_tip, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *batt_fill = lv_obj_create(batt_border);
    lv_obj_set_size(batt_fill, fill_w, 6);
    lv_obj_align(batt_fill, LV_ALIGN_LEFT_MID, 1, 0);
    lv_obj_set_style_bg_color(batt_fill, fill_color, 0);
    lv_obj_set_style_bg_opa(batt_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(batt_fill, 0, 0);
    lv_obj_set_style_radius(batt_fill, 1, 0);
    lv_obj_set_style_pad_all(batt_fill, 0, 0);
    lv_obj_set_scrollbar_mode(batt_fill, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_fill, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *batt_lbl = lv_label_create(info_card);
    char batt_str[8];
    lv_snprintf(batt_str, sizeof(batt_str), "%d%%", batt_pct);
    lv_label_set_text(batt_lbl, batt_str);
    lv_obj_set_style_text_font(batt_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(batt_lbl, fill_color, 0);
    lv_obj_align_to(batt_lbl, batt_border, LV_ALIGN_OUT_LEFT_MID, -2, 0);


    lv_obj_t *pos_card = create_glass_card(parent, 175, 96);
    lv_obj_align(pos_card, LV_ALIGN_BOTTOM_RIGHT, -6, -6);

    lv_obj_t *pos_head = lv_label_create(pos_card);
    lv_obj_set_style_text_color(pos_head, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(pos_head, &lv_font_montserrat_14, 0);
    lv_obj_align(pos_head, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(pos_head, "Job Position:");

    lv_obj_t *pos_lbl[POSITION_HOME_NUM];
    pos_lbl[0] = lv_label_create(pos_card);
    lv_label_set_text(pos_lbl[0], position[0]);
    lv_obj_set_style_text_color(pos_lbl[0], COLOR_WHITE, 0);
    lv_obj_set_style_text_font(pos_lbl[0], &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_opa(pos_lbl[0], LV_OPA_80, 0);
    lv_label_set_long_mode(pos_lbl[0], LV_LABEL_LONG_DOT);
    lv_obj_set_width(pos_lbl[0], 162);
    lv_obj_align_to(pos_lbl[0], pos_head, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);

    for(int i = 1; i < POSITION_HOME_NUM; i++) {
        pos_lbl[i] = lv_label_create(pos_card);
        lv_label_set_text(pos_lbl[i], position[i]);
        lv_obj_set_style_text_color(pos_lbl[i], COLOR_WHITE, 0);
        lv_obj_set_style_text_font(pos_lbl[i], &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_opa(pos_lbl[i], LV_OPA_80, 0);
        lv_label_set_long_mode(pos_lbl[i], LV_LABEL_LONG_DOT);
        lv_obj_set_width(pos_lbl[i], 162);
        lv_obj_align_to(pos_lbl[i], pos_lbl[i-1], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    }
}


static void level_dropdown_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    uint8_t data_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    meeting_level[data_idx] = (uint8_t)lv_dropdown_get_selected(dd);
    rebuild_meeting_list();
}

static void set_sort_btn_active(lv_obj_t *active, lv_obj_t *inactive)
{
    lv_obj_set_style_bg_color(active, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_bg_opa(active, LV_OPA_40, 0);
    lv_obj_set_style_border_color(active, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_border_opa(active, LV_OPA_70, 0);
    lv_obj_set_style_shadow_color(active, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_shadow_width(active, 6, 0);
    lv_obj_set_style_shadow_opa(active, LV_OPA_40, 0);
    lv_obj_set_style_bg_color(inactive, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(inactive, LV_OPA_30, 0);
    lv_obj_set_style_border_color(inactive, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_opa(inactive, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(inactive, 0, 0);
}

static void reset_sort_btns(void)
{
    if(!sort_priority_btn || !sort_time_btn_obj) return;
    lv_color_t b = COLOR_GLASS_TINT;
    lv_obj_set_style_bg_color(sort_priority_btn, b, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_priority_btn, b, 0);
    lv_obj_set_style_border_opa(sort_priority_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_priority_btn, 0, 0);
    lv_obj_set_style_bg_color(sort_time_btn_obj, b, 0);
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_time_btn_obj, b, 0);
    lv_obj_set_style_border_opa(sort_time_btn_obj, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_time_btn_obj, 0, 0);
    lv_label_set_text(lv_obj_get_child(sort_priority_btn, 0), LV_SYMBOL_DOWN " Pri");
    lv_label_set_text(lv_obj_get_child(sort_time_btn_obj,  0), LV_SYMBOL_DOWN " Time");
    meeting_sort_asc = 0;
}

static void sort_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    meeting_sort_asc ^= 1;
    lv_label_set_text(lv_obj_get_child(btn, 0),
        meeting_sort_asc ? LV_SYMBOL_UP " Pri" : LV_SYMBOL_DOWN " Pri");
    set_sort_btn_active(sort_priority_btn, sort_time_btn_obj);
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM - 1; i++)
        for(uint8_t j = 0; j < MESSAGE_MEETING_NUM - 1 - i; j++) {
            uint8_t a = meeting_order[j], b = meeting_order[j+1];
            bool sw = meeting_sort_asc
                ? (meeting_level[a] > meeting_level[b])
                : (meeting_level[a] < meeting_level[b]);
            if(sw) { meeting_order[j] = b; meeting_order[j+1] = a; }
        }
    rebuild_meeting_list();
}

static void sort_time_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    static uint8_t tsa = 1;
    tsa ^= 1;
    lv_label_set_text(lv_obj_get_child(btn, 0),
        tsa ? LV_SYMBOL_UP " Time" : LV_SYMBOL_DOWN " Time");
    set_sort_btn_active(sort_time_btn_obj, sort_priority_btn);
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM - 1; i++)
        for(uint8_t j = 0; j < MESSAGE_MEETING_NUM - 1 - i; j++) {
            uint8_t a = meeting_order[j], b = meeting_order[j+1];
            int cmp = strncmp(meeting_time[a], meeting_time[b], 5);
            bool sw = tsa ? (cmp > 0) : (cmp < 0);
            if(sw) { meeting_order[j] = b; meeting_order[j+1] = a; }
        }
    rebuild_meeting_list();
}

static void meeting_refresh_cb(lv_event_t *e)
{
    (void)e;
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM; i++) meeting_order[i] = i;
    reset_sort_btns();
    rebuild_meeting_list();
}


static void rebuild_meeting_list(void)
{
    if(!meeting_list_obj) return;
    lv_obj_clean(meeting_list_obj);

    for(uint8_t oi = 0; oi < MESSAGE_MEETING_NUM; oi++) {
        uint8_t i  = meeting_order[oi];
        lv_color_t c = level_color(meeting_level[i]);

        lv_obj_t *item = lv_obj_create(meeting_list_obj);
        lv_obj_set_size(item, lv_pct(100), 50);
        lv_obj_add_style(item, &style_card, LV_PART_MAIN);
        lv_obj_set_style_pad_all(item, 0, 0);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);


        lv_obj_t *left = lv_obj_create(item);
        lv_obj_set_size(left, lv_pct(78), lv_pct(100));
        lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_left(left, 10, 0);
        lv_obj_set_style_pad_right(left, 4, 0);
        lv_obj_set_style_pad_ver(left, 4, 0);
        lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *bar = lv_obj_create(left);
        lv_obj_set_size(bar, 3, lv_pct(70));
        lv_obj_align(bar, LV_ALIGN_LEFT_MID, -4, 0);
        lv_obj_set_style_bg_color(bar, c, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_shadow_color(bar, c, 0);
        lv_obj_set_style_shadow_width(bar, 4, 0);
        lv_obj_set_style_shadow_opa(bar, LV_OPA_50, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *time_lbl = lv_label_create(left);
        lv_label_set_text(time_lbl, meeting_time[i]);
        lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(time_lbl, c, 0);
        lv_obj_align(time_lbl, LV_ALIGN_TOP_LEFT, 6, 0);

        lv_obj_t *cont_lbl = lv_label_create(left);
        lv_label_set_text(cont_lbl, meeting_content[i]);
        lv_obj_set_style_text_font(cont_lbl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(cont_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(cont_lbl, LV_OPA_80, 0);
        lv_label_set_long_mode(cont_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(cont_lbl, lv_pct(90));
        lv_obj_align_to(cont_lbl, time_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);


        lv_obj_t *div = lv_obj_create(item);
        lv_obj_set_size(div, 1, lv_pct(70));
        lv_obj_align(div, LV_ALIGN_RIGHT_MID, -lv_pct(22), 0);
        lv_obj_set_style_bg_color(div, COLOR_WHITE, 0);
        lv_obj_set_style_bg_opa(div, LV_OPA_20, 0);
        lv_obj_set_style_border_width(div, 0, 0);
        lv_obj_clear_flag(div, LV_OBJ_FLAG_CLICKABLE);


        lv_obj_t *right = lv_obj_create(item);
        lv_obj_set_size(right, lv_pct(22), lv_pct(100));
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(right, 0, 0);
        lv_obj_set_style_pad_all(right, 3, 0);
        lv_obj_set_scrollbar_mode(right, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *dd = lv_dropdown_create(right);
        lv_dropdown_set_options(dd, "Urg\nImp\nNrm");
        lv_dropdown_set_selected(dd, meeting_level[i]);
        lv_obj_set_size(dd, lv_pct(100), 26);
        lv_obj_align(dd, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(dd, c, 0);
        lv_obj_set_style_bg_opa(dd, LV_OPA_30, 0);
        lv_obj_set_style_border_color(dd, c, 0);
        lv_obj_set_style_border_width(dd, 1, 0);
        lv_obj_set_style_border_opa(dd, LV_OPA_60, 0);
        lv_obj_set_style_radius(dd, 6, 0);
        lv_obj_set_style_text_color(dd, c, 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_12, 0);
        lv_obj_set_style_pad_ver(dd, 0, 0);
        lv_obj_set_style_pad_hor(dd, 3, 0);

        lv_obj_t *ddlist = lv_dropdown_get_list(dd);
        lv_obj_set_style_bg_color(ddlist, lv_color_hex(0x3A49F2), 0);
        lv_obj_set_style_bg_opa(ddlist, LV_OPA_90, 0);
        lv_obj_set_style_border_color(ddlist, COLOR_WHITE, 0);
        lv_obj_set_style_border_width(ddlist, 1, 0);
        lv_obj_set_style_border_opa(ddlist, LV_OPA_50, 0);
        lv_obj_set_style_radius(ddlist, 6, 0);
        lv_obj_set_style_text_color(ddlist, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(ddlist, &lv_font_montserrat_12, 0);
        lv_obj_set_style_bg_color(ddlist, COLOR_GLASS_TINT,
            LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(ddlist, LV_OPA_40,
            LV_PART_SELECTED | LV_STATE_CHECKED);

        lv_obj_add_event_cb(dd, level_dropdown_cb, LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)i);
    }
}

static void create_meeting_page(lv_obj_t *parent)
{
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM; i++) meeting_order[i] = i;


    lv_obj_t *hdr = create_glass_card(parent, lv_pct(96), 28);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_pad_hor(hdr, 6, 0);
    lv_obj_set_style_pad_ver(hdr, 0, 0);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "Meetings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, COLOR_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 0, 0);


    sort_priority_btn = lv_btn_create(hdr);
    lv_obj_set_size(sort_priority_btn, 60, 20);
    lv_obj_align(sort_priority_btn, LV_ALIGN_RIGHT_MID, -88, 0);
    lv_obj_set_style_radius(sort_priority_btn, 10, 0);
    lv_obj_set_style_bg_color(sort_priority_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_priority_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_width(sort_priority_btn, 1, 0);
    lv_obj_set_style_border_opa(sort_priority_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_priority_btn, 0, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_50, LV_STATE_PRESSED);
    lv_obj_t *slbl = lv_label_create(sort_priority_btn);
    lv_label_set_text(slbl, LV_SYMBOL_DOWN " Pri");
    lv_obj_set_style_text_font(slbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(slbl, COLOR_WHITE, 0);
    lv_obj_center(slbl);
    lv_obj_add_event_cb(sort_priority_btn, sort_btn_cb, LV_EVENT_CLICKED, NULL);


    sort_time_btn_obj = lv_btn_create(hdr);
    lv_obj_set_size(sort_time_btn_obj, 60, 20);
    lv_obj_align(sort_time_btn_obj, LV_ALIGN_RIGHT_MID, -22, 0);
    lv_obj_set_style_radius(sort_time_btn_obj, 10, 0);
    lv_obj_set_style_bg_color(sort_time_btn_obj, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_time_btn_obj, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_width(sort_time_btn_obj, 1, 0);
    lv_obj_set_style_border_opa(sort_time_btn_obj, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_time_btn_obj, 0, 0);
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_50, LV_STATE_PRESSED);
    lv_obj_t *tlbl = lv_label_create(sort_time_btn_obj);
    lv_label_set_text(tlbl, LV_SYMBOL_DOWN " Time");
    lv_obj_set_style_text_font(tlbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(tlbl, COLOR_WHITE, 0);
    lv_obj_center(tlbl);
    lv_obj_add_event_cb(sort_time_btn_obj, sort_time_btn_cb, LV_EVENT_CLICKED, NULL);


    lv_obj_t *rfr = lv_btn_create(hdr);
    lv_obj_set_size(rfr, 20, 20);
    lv_obj_align(rfr, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(rfr, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(rfr, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(rfr, LV_OPA_20, 0);
    lv_obj_set_style_border_color(rfr, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(rfr, 1, 0);
    lv_obj_set_style_shadow_width(rfr, 0, 0);
    lv_obj_set_style_bg_opa(rfr, LV_OPA_40, LV_STATE_PRESSED);
    lv_obj_t *ri = lv_label_create(rfr);
    lv_label_set_text(ri, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(ri, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ri, COLOR_WHITE, 0);
    lv_obj_center(ri);
    lv_obj_add_event_cb(rfr, meeting_refresh_cb, LV_EVENT_CLICKED, NULL);


    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(96), lv_pct(82));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 4, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    meeting_list_obj = list;
    rebuild_meeting_list();
}

/* =====================================================================
 * Card ҳ
 * ===================================================================== */
static void card_refresh_cb(lv_event_t *e) { (void)e; rebuild_card_list(); }

static void rebuild_card_list(void)
{
    if(!card_list_obj) return;
    lv_obj_clean(card_list_obj);

    bool any = false;
    for(uint8_t i = 0; i < USER_CARD_NUM; i++)
        if(user_cards[i].valid) { any = true; break; }

    if(!any) {
        lv_obj_t *ec = lv_obj_create(card_list_obj);
        lv_obj_set_size(ec, lv_pct(100), 50);
        lv_obj_add_style(ec, &style_card, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(ec, LV_SCROLLBAR_MODE_OFF);
        lv_obj_t *el = lv_label_create(ec);
        lv_label_set_text(el, "No exchanged cards yet");
        lv_obj_set_style_text_font(el, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(el, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(el, LV_OPA_40, 0);
        lv_obj_center(el);
        return;
    }

    for(uint8_t i = 0; i < USER_CARD_NUM; i++) {
        if(!user_cards[i].valid) continue;
        user_card_t *u = &user_cards[i];
        lv_color_t gc = (u->gender == GENDER_MALE)
            ? lv_color_hex(0x64C8FF) : lv_color_hex(0xFF8FB0);

        lv_coord_t ch = 54 + u->position_count * 16;
        if(ch < 76) ch = 76;

        lv_obj_t *card = lv_obj_create(card_list_obj);
        lv_obj_set_size(card, lv_pct(100), ch);
        lv_obj_add_style(card, &style_card, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 6, 0);
        lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);


        lv_obj_t *sb = lv_obj_create(card);
        lv_obj_set_size(sb, 4, ch - 12);
        lv_obj_align(sb, LV_ALIGN_LEFT_MID, -6, 0);
        lv_obj_set_style_bg_color(sb, gc, 0);
        lv_obj_set_style_bg_opa(sb, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(sb, 0, 0);
        lv_obj_set_style_radius(sb, 2, 0);
        lv_obj_set_style_shadow_color(sb, gc, 0);
        lv_obj_set_style_shadow_width(sb, 6, 0);
        lv_obj_set_style_shadow_opa(sb, LV_OPA_50, 0);
        lv_obj_clear_flag(sb, LV_OBJ_FLAG_CLICKABLE);


        lv_obj_t *ac = lv_obj_create(card);
        lv_obj_set_size(ac, 44, 44);
        lv_obj_align(ac, LV_ALIGN_TOP_LEFT, 6, 0);
        lv_obj_set_style_radius(ac, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(ac, gc, 0);
        lv_obj_set_style_bg_opa(ac, LV_OPA_30, 0);
        lv_obj_set_style_border_color(ac, gc, 0);
        lv_obj_set_style_border_width(ac, 1, 0);
        lv_obj_set_style_border_opa(ac, LV_OPA_60, 0);
        lv_obj_set_style_pad_all(ac, 0, 0);
        lv_obj_set_style_clip_corner(ac, true, 0);
        lv_obj_set_scrollbar_mode(ac, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(ac, LV_DIR_NONE);

        lv_obj_t *ai = lv_img_create(ac);
        lv_img_set_src(ai, (u->gender == GENDER_MALE) ? &Man : &Woman);
        lv_obj_set_size(ai, 44, 44);
        lv_img_set_size_mode(ai, LV_IMG_SIZE_MODE_REAL);
        lv_img_set_zoom(ai, 256);
        lv_img_set_antialias(ai, true);
        lv_obj_center(ai);


        lv_obj_t *info = lv_obj_create(card);
        lv_obj_set_size(info, 170, ch - 12);
        lv_obj_align(info, LV_ALIGN_TOP_LEFT, 58, 0);
        lv_obj_set_style_bg_opa(info, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(info, 0, 0);
        lv_obj_set_style_pad_all(info, 0, 0);
        lv_obj_set_scrollbar_mode(info, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(info, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *nl = lv_label_create(info);
        lv_label_set_text(nl, u->name);
        lv_obj_set_style_text_font(nl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(nl, gc, 0);
        lv_obj_align(nl, LV_ALIGN_TOP_LEFT, 0, 0);

        lv_obj_t *gl = lv_label_create(info);
        lv_label_set_text(gl, (u->gender == GENDER_MALE) ? "Male" : "Female");
        lv_obj_set_style_text_font(gl, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(gl, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(gl, LV_OPA_60, 0);
        lv_obj_align_to(gl, nl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

        lv_obj_t *dv = lv_obj_create(info);
        lv_obj_set_size(dv, 166, 1);
        lv_obj_align_to(dv, gl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);
        lv_obj_set_style_bg_color(dv, COLOR_WHITE, 0);
        lv_obj_set_style_bg_opa(dv, LV_OPA_20, 0);
        lv_obj_set_style_border_width(dv, 0, 0);
        lv_obj_clear_flag(dv, LV_OBJ_FLAG_CLICKABLE);

        lv_obj_t *prev = dv;
        for(uint8_t j = 0; j < u->position_count && j < POSITION_HOME_NUM; j++) {
            lv_obj_t *pl = lv_label_create(info);
            lv_label_set_text(pl, u->position[j]);
            lv_label_set_long_mode(pl, LV_LABEL_LONG_DOT);
            lv_obj_set_width(pl, 166);
            lv_obj_set_style_text_font(pl, &lv_font_montserrat_12, 0);
            lv_obj_set_style_text_color(pl, COLOR_WHITE, 0);
            lv_obj_set_style_text_opa(pl, LV_OPA_80, 0);
            lv_obj_align_to(pl, prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 3);
            prev = pl;
        }
    }
}

static void create_card_page(lv_obj_t *parent)
{
    lv_obj_t *hdr = create_glass_card(parent, lv_pct(96), 28);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_pad_hor(hdr, 6, 0);
    lv_obj_set_style_pad_ver(hdr, 0, 0);

    lv_obj_t *tl = lv_label_create(hdr);
    lv_label_set_text(tl, LV_SYMBOL_SHUFFLE "  Exchanged Cards");
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(tl, COLOR_WHITE, 0);
    lv_obj_align(tl, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *rb = lv_btn_create(hdr);
    lv_obj_set_size(rb, 20, 20);
    lv_obj_align(rb, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(rb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(rb, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(rb, LV_OPA_20, 0);
    lv_obj_set_style_border_color(rb, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(rb, 1, 0);
    lv_obj_set_style_shadow_width(rb, 0, 0);
    lv_obj_set_style_bg_opa(rb, LV_OPA_40, LV_STATE_PRESSED);
    lv_obj_t *ri = lv_label_create(rb);
    lv_label_set_text(ri, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(ri, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(ri, COLOR_WHITE, 0);
    lv_obj_center(ri);
    lv_obj_add_event_cb(rb, card_refresh_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(96), lv_pct(82));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);
    card_list_obj = list;
    rebuild_card_list();
}

/* =====================================================================
 * Controller ҳ
 * ===================================================================== */
static void switch_event_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    uint32_t id  = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    bool on      = lv_obj_has_state(sw, LV_STATE_CHECKED);
    switch(id) {
        case 0: ctrl_starflash_enabled = on; hw_starflash_set(on); break;
        case 1: ctrl_bluetooth_enabled = on; hw_bluetooth_set(on); break;
        case 2: ctrl_nfc_enabled       = on; hw_nfc_set(on);       break;
        case 3: ctrl_wifi_enabled      = on; hw_wifi_set(on);      break;
    }
}

static void brightness_slider_cb(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target(e);
    lv_obj_t *v = lv_event_get_user_data(e);
    int32_t val = lv_slider_get_value(s);
    ctrl_brightness = (uint8_t)val;
    char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_label_set_text(v, buf);
    hw_brightness_set(ctrl_brightness);
}

static void volume_slider_cb(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target(e);
    lv_obj_t *v = lv_event_get_user_data(e);
    int32_t val = lv_slider_get_value(s);
    ctrl_volume = (uint8_t)val;
    char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", (int)val);
    lv_label_set_text(v, buf);
    hw_volume_set(ctrl_volume);
}


static lv_obj_t *create_switch_card(lv_obj_t *parent, const char *icon,
    const char *name, bool init, uint32_t id, lv_color_t col)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 118, 54);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *il = lv_label_create(card);
    lv_label_set_text(il, icon);
    lv_obj_set_style_text_color(il, col, 0);
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
    lv_obj_align(il, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *nl = lv_label_create(card);
    lv_label_set_text(nl, name);
    lv_obj_set_style_text_color(nl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_opa(nl, LV_OPA_80, 0);
    lv_obj_align(nl, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 34, 18);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x404040),
        LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sw, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x606060),
        LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER,
        LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(sw, col,
        LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER,
        LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_color(sw, col,
        LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(sw, 5,
        LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_opa(sw, LV_OPA_40,
        LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(sw, COLOR_WHITE,
        LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER,
        LV_PART_KNOB | LV_STATE_DEFAULT);
    if(init) lv_obj_add_state(sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED,
                        (void *)(uintptr_t)id);
    return card;
}


static lv_obj_t *create_slider_card(lv_obj_t *parent, const char *icon,
    const char *name, uint8_t init_val, lv_event_cb_t cb, lv_color_t col)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 118, 68);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *il = lv_label_create(card);
    lv_label_set_text(il, icon);
    lv_obj_set_style_text_color(il, col, 0);
    lv_obj_set_style_text_font(il, &lv_font_montserrat_14, 0);
    lv_obj_align(il, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *vl = lv_label_create(card);
    char buf[8]; lv_snprintf(buf, sizeof(buf), "%d%%", init_val);
    lv_label_set_text(vl, buf);
    lv_obj_set_style_text_color(vl, col, 0);
    lv_obj_set_style_text_font(vl, &lv_font_montserrat_12, 0);
    lv_obj_align(vl, LV_ALIGN_TOP_RIGHT, 0, 0);

    lv_obj_t *nl = lv_label_create(card);
    lv_label_set_text(nl, name);
    lv_obj_set_style_text_color(nl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(nl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_opa(nl, LV_OPA_70, 0);
    lv_obj_align(nl, LV_ALIGN_TOP_LEFT, 0, 20);

    lv_obj_t *sl = lv_slider_create(card);
    lv_obj_set_size(sl, lv_pct(100), 8);
    lv_obj_align(sl, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(sl, 0, 100);
    lv_slider_set_value(sl, init_val, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sl, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sl, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_radius(sl, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sl, col, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(sl, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sl, COLOR_WHITE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(sl, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(sl, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(sl, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, vl);
    return card;
}

static void create_controller_page(lv_obj_t *parent)
{
    /*
     * ============================================================
     * PPT
     * ============================================================
     *
     * lv_obj_t *ppt_card = create_glass_card(parent, lv_pct(92), 80);
     * lv_obj_align(ppt_card, LV_ALIGN_TOP_MID, 0, 4);
     * ...
     * lv_obj_add_event_cb(prev_btn, ppt_prev_cb, LV_EVENT_CLICKED, page_label);
     * lv_obj_add_event_cb(next_btn, ppt_next_cb, LV_EVENT_CLICKED, page_label);
     *
     * ============================================================
     */


    lv_obj_t *tc = create_glass_card(parent, lv_pct(96), 24);
    lv_obj_align(tc, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_t *tl = lv_label_create(tc);
    lv_label_set_text(tl, LV_SYMBOL_SETTINGS "  Settings");
    lv_obj_set_style_text_color(tl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(tl, &lv_font_montserrat_12, 0);
    lv_obj_center(tl);

    /* ���п�Ƭ����  252 = 118*2 + 16 */
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 252, 185);
    lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_style_pad_row(cont, 6, 0);
    lv_obj_set_style_pad_column(cont, 16, 0);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    create_switch_card(cont, LV_SYMBOL_WIFI,      "StarFlash",
        ctrl_starflash_enabled, 0, lv_color_hex(0xFF6B9D));
    create_switch_card(cont, LV_SYMBOL_BLUETOOTH, "Bluetooth",
        ctrl_bluetooth_enabled, 1, lv_color_hex(0x64C8FF));
    create_switch_card(cont, "N",                 "NFC",
        ctrl_nfc_enabled,       2, lv_color_hex(0xFFD74E));
    create_switch_card(cont, LV_SYMBOL_WIFI,      "WiFi",
        ctrl_wifi_enabled,      3, lv_color_hex(0x69FF7A));
    create_slider_card(cont, LV_SYMBOL_EYE_OPEN,  "Brightness",
        ctrl_brightness, brightness_slider_cb, lv_color_hex(0xFFD74E));
    create_slider_card(cont, LV_SYMBOL_VOLUME_MAX,"Volume",
        ctrl_volume,     volume_slider_cb,     lv_color_hex(0x64FFDA));
}


typedef void (*page_create_fn)(lv_obj_t *);
static const page_create_fn page_creators[PAGE_COUNT] = {
    create_home_page,
    create_meeting_page,
    create_card_page,
    create_controller_page,
};

static void show_page(uint8_t index)
{
    if(!page_created[index]) {
        page_creators[index](pages[index]);
        page_created[index] = true;
    }
    for(uint8_t i = 0; i < PAGE_COUNT; i++) {
        if(i == index) lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else           lv_obj_add_flag(pages[i],   LV_OBJ_FLAG_HIDDEN);
    }
    for(uint8_t i = 0; i < PAGE_COUNT; i++) {
        if(i == index)
            lv_obj_add_style(tab_btns[i], &style_dock_btn_active, LV_PART_MAIN);
        else
            lv_obj_remove_style(tab_btns[i], &style_dock_btn_active, LV_PART_MAIN);
    }
    current_page = index;
}

static void tab_btn_event_cb(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    show_page((uint8_t)idx);
}


void lv_mainstart(void)
{

    usr_name  = "User";
    home_time = "12:20";
    home_date = "2026/01/01 Mon";
    batt_pct  = 70;

    position[0] = "CTO";
    position[1] = "VP Engineering";
    position[2] = "Eng Manager";
    position[3] = "Sr. Engineer";

    meeting_time[0] = "09:00-09:30";
    meeting_time[1] = "10:00-11:00";
    meeting_time[2] = "13:30-14:00";
    meeting_time[3] = "15:00-16:30";
    meeting_time[4] = "17:00-17:30";

    meeting_content[0] = "Morning standup";
    meeting_content[1] = "Product review";
    meeting_content[2] = "Client demo";
    meeting_content[3] = "Q3 roadmap";
    meeting_content[4] = "Daily wrap-up";

    level_text[0] = "Urgent";
    level_text[1] = "Important";
    level_text[2] = "Normal";

    meeting_level[0] = 0; meeting_level[1] = 1;
    meeting_level[2] = 2; meeting_level[3] = 0;
    meeting_level[4] = 1;

    user_cards[0].valid = true;  user_cards[0].name = "Alice Chen";
    user_cards[0].gender = GENDER_FEMALE;
    user_cards[0].position[0] = "Product Mgr";
    user_cards[0].position[1] = "UX Lead";
    user_cards[0].position_count = 2;

    user_cards[1].valid = true;  user_cards[1].name = "Bob Zhang";
    user_cards[1].gender = GENDER_MALE;
    user_cards[1].position[0] = "Sr. Engineer";
    user_cards[1].position[1] = "Tech Lead";
    user_cards[1].position[2] = "Architect";
    user_cards[1].position_count = 3;

    user_cards[2].valid = false; user_cards[2].name = "";
    user_cards[2].position_count = 0;

    /* PPT
    ppt_current_page = 1;
    ppt_total_pages  = 20;
    */

    setup_styles();

    lv_obj_t *screen = lv_scr_act();
    lv_obj_add_style(screen, &style_screen_bg, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);


    for(uint8_t i = 0; i < PAGE_COUNT; i++) {
        page_created[i] = false;
        pages[i] = lv_obj_create(screen);
        lv_obj_set_size(pages[i], lv_pct(100), lv_pct(84));
        lv_obj_align(pages[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(pages[i], &style_page, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(pages[i], LV_SCROLLBAR_MODE_OFF);
        lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* Dock 36px */
    lv_obj_t *dock_wrap = lv_obj_create(screen);
    lv_obj_set_size(dock_wrap, lv_pct(100), 36);
    lv_obj_align(dock_wrap, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_opa(dock_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dock_wrap, 0, 0);
    lv_obj_set_style_pad_all(dock_wrap, 0, 0);
    lv_obj_set_scrollbar_mode(dock_wrap, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(dock_wrap, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *dock_pill = lv_obj_create(dock_wrap);
    lv_obj_set_height(dock_pill, 32);
    lv_obj_set_width(dock_pill, LV_SIZE_CONTENT);
    lv_obj_align(dock_pill, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(dock_pill, &style_dock_wrap, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(dock_pill, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_layout(dock_pill, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dock_pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock_pill, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dock_pill, 5, 0);
    lv_obj_set_style_pad_hor(dock_pill, 8, 0);
    lv_obj_set_style_pad_ver(dock_pill, 2, 0);

    static const lv_color_t icon_colors[PAGE_COUNT] = {
        LV_COLOR_MAKE(0xC7, 0x35, 0x17),
        LV_COLOR_MAKE(0x05, 0x91, 0x1F),
        LV_COLOR_MAKE(0xFF, 0xD7, 0x4E),
        LV_COLOR_MAKE(0x11, 0x09, 0x91),
    };

    for(uint8_t i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(dock_pill);
        lv_obj_set_size(btn, 56, 26);
        lv_obj_add_style(btn, &style_dock_btn_normal, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, COLOR_WHITE, LV_STATE_PRESSED);
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, page_icons[i]);
        lv_obj_set_style_text_color(icon, icon_colors[i], 0);
        lv_obj_set_style_text_opa(icon, LV_OPA_COVER, 0);
        lv_obj_set_style_text_font(icon, &lv_font_montserrat_12, 0);

        lv_obj_t *name_l = lv_label_create(btn);
        lv_label_set_text(name_l, page_names[i]);
        lv_obj_set_style_text_font(name_l, &lv_font_montserrat_12, 0);
        lv_obj_set_style_text_color(name_l, icon_colors[i], 0);
        lv_obj_set_style_text_opa(name_l, LV_OPA_70, 0);

        lv_obj_add_event_cb(btn, tab_btn_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)i);
        tab_btns[i] = btn;
    }

    show_page(0);
}



/* PPT
void hw_ppt_get_page_info(uint16_t *current, uint16_t *total) {
    if(current) *current = ppt_current_page;
    if(total)   *total   = ppt_total_pages;
}
void hw_ppt_set_page_info(uint16_t current, uint16_t total) {
    if(current > 0 && current <= total) ppt_current_page = current;
    if(total > 0) ppt_total_pages = total;
}
static void hw_ppt_prev_page(void) {}
static void hw_ppt_next_page(void) {}
*/

static void hw_starflash_set(bool enable) { (void)enable; }
static void hw_bluetooth_set(bool enable) { (void)enable; }
static void hw_nfc_set(bool enable)       { (void)enable; }
static void hw_wifi_set(bool enable)      { (void)enable; }
static void hw_brightness_set(uint8_t v)  { (void)v; }
static void hw_volume_set(uint8_t v)      { (void)v; }
