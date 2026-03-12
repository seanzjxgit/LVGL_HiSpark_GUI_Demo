#include "lv_mainstart.h"
#include "lvgl.h"
// 用于测试
#include<stdio.h>

#define COLOR_BG_TOP      lv_color_hex(0x7B3FE4)  // 深梦幻紫
#define COLOR_BG_BOT      lv_color_hex(0x00C8FF)  // 极光蓝
#define COLOR_WHITE       lv_color_hex(0xFFFFFF)
#define COLOR_GLASS_TINT  lv_color_hex(0xC8AAFF)  // 紫色玻璃高光
#define COLOR_SHADOW      lv_color_hex(0x1A0050)  // 深紫阴影

//字体（按需取消注释）
// LV_FONT_DECLARE(font_cn_30);  // 中文字体示例,看情况汉化，占内存

#define PAGE_COUNT  4
#define POSITION_HOME_NUM 4 //home页的position数量的上限
#define MESSAGE_MEETING_NUM 5
// 重要程度对应颜色
#define COLOR_LEVEL_HIGH    lv_color_hex(0xFF5252)  // 红 — 重要
#define COLOR_LEVEL_MID     lv_color_hex(0xFFD74E)  // 黄 — 一般
#define COLOR_LEVEL_LOW     lv_color_hex(0x69FF7A)  // 绿 — 不重要
#define USER_CARD_NUM 3 //名片交换的最大数量

//图片源的声明
LV_IMG_DECLARE(User)
LV_IMG_DECLARE(Man)
LV_IMG_DECLARE(Woman)


/* 性别枚举 */
typedef enum {
    GENDER_MALE = 0,
    GENDER_FEMALE
} user_gender_t;
/*
typedef struct {
    uint8_t  data_idx;   //对应 meeting_xxx 数组下标
    uint8_t  new_level;  // 选择的新等级
} level_change_t;
*/
/* 用户名片结构体 */
typedef struct {
    char          *name;
    user_gender_t  gender;
    char          *position[POSITION_HOME_NUM]; /* 最多4项职务 */
    uint8_t        position_count;              /* 实际职务数量 */
    bool           valid;                       /* false = 空槽位（未交换） */
} user_card_t;

//重要参数变量以及函数的声明（后面可能移入头文件）
static char *usr_name;
static char *home_time;
static char *home_date;
static uint8_t batt_pct;
static char *position[POSITION_HOME_NUM];
static char *meeting_time[MESSAGE_MEETING_NUM];
static char *meeting_content[MESSAGE_MEETING_NUM];
static char *level_text[3];
static uint8_t meeting_level[MESSAGE_MEETING_NUM];
//static level_change_t level_change_buf[MESSAGE_MEETING_NUM];
static uint8_t meeting_order[MESSAGE_MEETING_NUM];
static lv_obj_t *meeting_list_obj = NULL; //列表容器引用，供刷新使用
static lv_obj_t *card_list_obj = NULL; // 列表容器引用，供刷新使用
static uint8_t meeting_sort_asc = 0; // 排序方向：0=重要程度降序，1=升序
static void rebuild_meeting_list(void); // 前向声明
static void rebuild_card_list(void); // 前向声明
static lv_obj_t *sort_priority_btn = NULL;
static lv_obj_t *sort_time_btn_obj = NULL;
static user_card_t user_cards[USER_CARD_NUM]; // 名片数组
static uint16_t ppt_current_page = 1;    // 当前页码
static uint16_t ppt_total_pages;    // 总页数
static bool ctrl_starflash_enabled = false;  // 星闪（true=开启，false=关闭）
static bool ctrl_bluetooth_enabled = true;   // 蓝牙（true=开启，false=关闭）
static bool ctrl_nfc_enabled = false;        // NFC（true=开启，false=关闭）
static bool ctrl_wifi_enabled = true;        // WiFi（true=开启，false=关闭）
static uint8_t ctrl_brightness = 75;    // 屏幕亮度（0-100）
static uint8_t ctrl_volume = 50;        // 扬声器音量（0-100）


//硬件接口函数
static void hw_ppt_prev_page(void);
static void hw_ppt_next_page(void);
static void hw_starflash_set(bool enable) ;
static void hw_bluetooth_set(bool enable) ;
static void hw_nfc_set(bool enable);
static void hw_wifi_set(bool enable);
static void hw_brightness_set(uint8_t value);
static void hw_volume_set(uint8_t value) ;
void hw_ppt_get_page_info(uint16_t *current, uint16_t *total);
void hw_ppt_set_page_info(uint16_t current, uint16_t total) ;

static lv_obj_t *pages[PAGE_COUNT];       // 页面容器
static bool       page_created[PAGE_COUNT]; // 是否已初始化
static uint8_t    current_page = 0;
static lv_obj_t  *tab_btns[PAGE_COUNT];   // dock 按钮引用

/* 页面名称 & 对应图标（使用 LVGL 内置符号） */
static const char *page_names[PAGE_COUNT] = {
    "Home", "Meeting", "Card", "Ctrl"
};
static const char *page_icons[PAGE_COUNT] = {
    LV_SYMBOL_HOME,
    LV_SYMBOL_VIDEO,
    LV_SYMBOL_FILE,
    LV_SYMBOL_SETTINGS
};

//样式

static lv_style_t style_screen_bg;   // 渐变背景
static lv_style_t style_page;        // 页面容器（透明，不遮背景）
static lv_style_t style_card;        // 玻璃卡片
static lv_style_t style_dock_wrap;   // dock 外层胶囊容器
static lv_style_t style_dock_btn_normal;   // dock 按钮默认态
static lv_style_t style_dock_btn_active;   // dock 按钮选中态
static lv_color_t level_color(uint8_t lv) {
    if(lv == 0) return COLOR_LEVEL_HIGH;
    if(lv == 1) return COLOR_LEVEL_MID;
    return COLOR_LEVEL_LOW;
}

static void setup_styles(void)
{
    /* ---- 渐变背景 ---- */
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, COLOR_BG_TOP);
    lv_style_set_bg_grad_color(&style_screen_bg, COLOR_BG_BOT);
    lv_style_set_bg_grad_dir(&style_screen_bg, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);

    /* ---- 页面容器：完全透明，不遮背景 ---- */
    lv_style_init(&style_page);
    lv_style_set_bg_opa(&style_page, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_page, 0);
    lv_style_set_pad_all(&style_page, 0);
    lv_style_set_radius(&style_page, 0);

    /* ---- 玻璃卡片 ---- */
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, COLOR_WHITE);
    lv_style_set_bg_opa(&style_card, LV_OPA_20);           // 主体低透明白
    lv_style_set_radius(&style_card, 28);
    lv_style_set_border_color(&style_card, COLOR_GLASS_TINT);
    lv_style_set_border_width(&style_card, 1);             // 细高光边框
    lv_style_set_border_opa(&style_card, LV_OPA_40);
    /* 外阴影（深紫色，制造悬浮感） */
    lv_style_set_shadow_color(&style_card, COLOR_SHADOW);
    lv_style_set_shadow_width(&style_card, 40);
    lv_style_set_shadow_spread(&style_card, 0);
    lv_style_set_shadow_opa(&style_card, LV_OPA_30);
    lv_style_set_shadow_ofs_x(&style_card, 0);
    lv_style_set_shadow_ofs_y(&style_card, 8);
    lv_style_set_pad_all(&style_card, 16);

    /* ---- Dock 外层胶囊：Liquid Glass 核心 ---- */
    lv_style_init(&style_dock_wrap);
    lv_style_set_bg_color(&style_dock_wrap, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_wrap, 64);             // 磨砂白 ~25%，LV_OPA_25 v8不存在
    lv_style_set_radius(&style_dock_wrap, LV_RADIUS_CIRCLE);
    lv_style_set_border_color(&style_dock_wrap, COLOR_WHITE);
    lv_style_set_border_width(&style_dock_wrap, 1);
    lv_style_set_border_opa(&style_dock_wrap, LV_OPA_50);
    lv_style_set_shadow_color(&style_dock_wrap, COLOR_SHADOW);
    lv_style_set_shadow_width(&style_dock_wrap, 30);
    lv_style_set_shadow_spread(&style_dock_wrap, 2);
    lv_style_set_shadow_opa(&style_dock_wrap, LV_OPA_30);
    lv_style_set_shadow_ofs_x(&style_dock_wrap, 0);
    lv_style_set_shadow_ofs_y(&style_dock_wrap, 4);
    lv_style_set_pad_hor(&style_dock_wrap, 10);
    lv_style_set_pad_ver(&style_dock_wrap, 6);

    /* ---- Dock 按钮默认态：几乎透明 ---- */
    lv_style_init(&style_dock_btn_normal);
    lv_style_set_bg_color(&style_dock_btn_normal, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_btn_normal, LV_OPA_0);   // 完全透明
    lv_style_set_border_width(&style_dock_btn_normal, 0);
    lv_style_set_shadow_width(&style_dock_btn_normal, 0);
    lv_style_set_radius(&style_dock_btn_normal, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_dock_btn_normal, 6);
    lv_style_set_text_color(&style_dock_btn_normal, COLOR_WHITE);
    lv_style_set_text_opa(&style_dock_btn_normal, LV_OPA_70);

    /* ---- Dock 按钮选中态：纯白 pill，高亮 ---- */
    lv_style_init(&style_dock_btn_active);
    lv_style_set_bg_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_bg_opa(&style_dock_btn_active, LV_OPA_30);
    lv_style_set_border_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_border_width(&style_dock_btn_active, 1);
    lv_style_set_border_opa(&style_dock_btn_active, LV_OPA_60);
    lv_style_set_shadow_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_shadow_width(&style_dock_btn_active, 12);
    lv_style_set_shadow_opa(&style_dock_btn_active, LV_OPA_40);
    lv_style_set_radius(&style_dock_btn_active, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_dock_btn_active, 6);
    lv_style_set_text_color(&style_dock_btn_active, COLOR_WHITE);
    lv_style_set_text_opa(&style_dock_btn_active, LV_OPA_COVER);
}

//辅助：创建玻璃卡片子容器
static lv_obj_t *create_glass_card(lv_obj_t *parent, lv_coord_t w, lv_coord_t h)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, w, h);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    return card;
}

//各页面内容创建函数（按需实现）
static void create_home_page(lv_obj_t *parent)
{
    /* ---------- 头像卡片 ---------- */
    lv_obj_t *avatar_card = create_glass_card(parent, 280, 350);
    lv_obj_align(avatar_card, LV_ALIGN_LEFT_MID, 40, 0);

    // 头像占位圆形
    lv_obj_t *avatar_circle = lv_obj_create(avatar_card);
    lv_obj_set_size(avatar_circle, 200, 200);
    lv_obj_set_style_radius(avatar_circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(avatar_circle, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(avatar_circle, LV_OPA_60, 0);
    lv_obj_set_style_border_width(avatar_circle, 0, 0);
    lv_obj_set_style_pad_all(avatar_circle, 0, 0);          // 去掉内边距，图片贴边
    lv_obj_set_style_clip_corner(avatar_circle, true, 0);   // 关键：开启圆角裁剪
    lv_obj_set_scrollbar_mode(avatar_circle, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(avatar_circle, LV_DIR_NONE);      // 禁止滚动偏移
    lv_obj_align(avatar_circle, LV_ALIGN_TOP_MID, 0, 16); // 上移，留出下方空间

    // 头像图片
    lv_obj_t *av_icon = lv_img_create(avatar_circle);
    lv_img_set_src(av_icon, &User);
    lv_obj_set_size(av_icon, 200, 200);                     // 强制与圆同尺寸
    lv_img_set_size_mode(av_icon, LV_IMG_SIZE_MODE_REAL);   // 按控件尺寸显示
    lv_img_set_zoom(av_icon, 256);                          // 256 = 100% 原始，可调
    lv_img_set_antialias(av_icon, true);                    // 抗锯齿
    lv_obj_align(av_icon, LV_ALIGN_CENTER, 0, 0);

    // ---------- 姓名文本框 ----------
    lv_obj_t *name_box = lv_obj_create(avatar_card);
    lv_obj_set_size(name_box, 230, 50);
    lv_obj_align(name_box, LV_ALIGN_BOTTOM_MID, 0, -5); // 卡片底部居中
    lv_obj_set_style_bg_color(name_box, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(name_box, LV_OPA_20, 0);     // 半透明玻璃感
    lv_obj_set_style_border_color(name_box, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(name_box, 1, 0);
    lv_obj_set_style_border_opa(name_box, LV_OPA_40, 0);
    lv_obj_set_style_radius(name_box, 12, 0);
    lv_obj_set_style_pad_all(name_box, 0, 0);
    lv_obj_set_scrollbar_mode(name_box, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *name_lbl1 = lv_label_create(name_box);
    lv_label_set_text(name_lbl1, usr_name);
    lv_obj_set_style_text_font(name_lbl1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(name_lbl1, COLOR_WHITE, 0);
    lv_obj_center(name_lbl1);



    // ---------- 右上角小卡片 ----------
    lv_obj_t *info_card = create_glass_card(parent, 400, 120);
    lv_obj_align(info_card, LV_ALIGN_TOP_MID, 140, 28);

    lv_obj_t *name_lbl2 = lv_label_create(info_card);
    lv_label_set_text(name_lbl2, home_time);
    lv_obj_set_style_text_color(name_lbl2, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(name_lbl2, &lv_font_montserrat_34, 0);
    lv_obj_align(name_lbl2, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *role_lbl1 = lv_label_create(info_card);
    lv_label_set_text(role_lbl1, home_date);
    lv_obj_set_style_text_color(role_lbl1, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(role_lbl1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_opa(role_lbl1, LV_OPA_60, 0);
    lv_obj_align_to(role_lbl1, name_lbl2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 18);

     // ---------- 电池组件（信息卡片右上角）----------
    // 电池外框
    lv_obj_t *batt_border = lv_obj_create(info_card);
    lv_obj_set_size(batt_border, 36, 18);
    lv_obj_align(batt_border, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_opa(batt_border, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(batt_border, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(batt_border, 2, 0);
    lv_obj_set_style_border_opa(batt_border, LV_OPA_80, 0);
    lv_obj_set_style_radius(batt_border, 4, 0);
    lv_obj_set_style_pad_all(batt_border, 0, 0);
    lv_obj_set_scrollbar_mode(batt_border, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_border, LV_OBJ_FLAG_CLICKABLE);

    // 电池正极小突起
    lv_obj_t *batt_tip = lv_obj_create(info_card);
    lv_obj_set_size(batt_tip, 4, 8);
    lv_obj_align_to(batt_tip, batt_border, LV_ALIGN_OUT_RIGHT_MID, 1, 0);
    lv_obj_set_style_bg_color(batt_tip, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(batt_tip, LV_OPA_80, 0);
    lv_obj_set_style_border_width(batt_tip, 0, 0);
    lv_obj_set_style_radius(batt_tip, 2, 0);
    lv_obj_set_scrollbar_mode(batt_tip, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_tip, LV_OBJ_FLAG_CLICKABLE);

    // 电量填充（与百分比线性关系）
    // 内部可用宽度 = 36 - 2*2(边框) - 2*2(内边距留空) = 28px
    #define BATT_FILL_MAX_W  28
    uint8_t fill_w = (uint8_t)((uint32_t)batt_pct * BATT_FILL_MAX_W / 100);
    if(fill_w < 2) fill_w = 2; // 最小显示 2px，避免完全消失

    // 颜色：高电量绿，中电量黄，低电量红
    lv_color_t fill_color;
    if(batt_pct > 50)       fill_color = lv_color_hex(0x69FF7A); // 浅绿
    else if(batt_pct > 20)  fill_color = lv_color_hex(0xFFD74E); // 琥珀黄
    else                    fill_color = lv_color_hex(0xFF5252); // 警告红

    lv_obj_t *batt_fill = lv_obj_create(batt_border);
    lv_obj_set_size(batt_fill, fill_w, 10);
    lv_obj_align(batt_fill, LV_ALIGN_LEFT_MID, 2, 0);  // 左对齐，留 2px 内边距
    lv_obj_set_style_bg_color(batt_fill, fill_color, 0);
    lv_obj_set_style_bg_opa(batt_fill, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(batt_fill, 0, 0);
    lv_obj_set_style_radius(batt_fill, 2, 0);
    lv_obj_set_style_pad_all(batt_fill, 0, 0);
    lv_obj_set_scrollbar_mode(batt_fill, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(batt_fill, LV_OBJ_FLAG_CLICKABLE);

    // 百分比文字（电池图标左侧）
    lv_obj_t *batt_lbl = lv_label_create(info_card);
    char batt_str[8];
    lv_snprintf(batt_str, sizeof(batt_str), "%d%%", batt_pct);
    lv_label_set_text(batt_lbl, batt_str);
    lv_obj_set_style_text_font(batt_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(batt_lbl, fill_color, 0); // 颜色跟随电量
    lv_obj_align_to(batt_lbl, batt_border, LV_ALIGN_OUT_LEFT_MID, -4, 0);



    // ---------- 信息卡片 ----------
    lv_obj_t * position_card =create_glass_card(parent, 400,220);
    lv_obj_align(position_card,LV_ALIGN_CENTER,140,65);
    lv_obj_t * position_head_lbl =lv_label_create(position_card);
    lv_obj_set_style_text_color(position_head_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(position_head_lbl, &lv_font_montserrat_28, 0);
    lv_obj_align(position_head_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_text(position_head_lbl, "Job Position:");


    //此处未来可以尝试使用链表和枚举，使用栈，限制个数，以及优先级显示，职务高低的顺序等->目前采用的数组调整
    lv_obj_t * position_demo_lbl[POSITION_HOME_NUM];
    position_demo_lbl[0] =lv_label_create(position_card);
    lv_label_set_text(position_demo_lbl[0], position[0]);
    lv_obj_set_style_text_color(position_demo_lbl[0], COLOR_WHITE, 0);
    lv_obj_set_style_text_font(position_demo_lbl[0], &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_opa(position_demo_lbl[0], LV_OPA_80, 0);
    lv_obj_align_to(position_demo_lbl[0], position_head_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 18);

    for (int i=1;i<POSITION_HOME_NUM;i++)
    {
        position_demo_lbl[i] =lv_label_create(position_card);
        lv_label_set_text(position_demo_lbl[i], position[i]);
        lv_obj_set_style_text_color(position_demo_lbl[i], COLOR_WHITE, 0);
        lv_obj_set_style_text_font(position_demo_lbl[i], &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_opa(position_demo_lbl[i], LV_OPA_80, 0);
        lv_obj_align_to(position_demo_lbl[i], position_demo_lbl[i-1], LV_ALIGN_OUT_BOTTOM_LEFT, 0, 18);

    }





}

static void level_dropdown_cb(lv_event_t *e)
{
    lv_obj_t *dd = lv_event_get_target(e);
    uint8_t data_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    uint16_t sel = lv_dropdown_get_selected(dd);
    meeting_level[data_idx] = (uint8_t)sel;
    /* 关闭后重建列表 */
    rebuild_meeting_list();
}

/* 统一的按钮高亮切换函数 */
static void set_sort_btn_active(lv_obj_t *active_btn, lv_obj_t *inactive_btn)
{
    /* 激活态：青色 */
    lv_obj_set_style_bg_color(active_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_bg_opa(active_btn, LV_OPA_40, 0);
    lv_obj_set_style_border_color(active_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_border_opa(active_btn, LV_OPA_70, 0);
    lv_obj_set_style_shadow_color(active_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_shadow_width(active_btn, 12, 0);
    lv_obj_set_style_shadow_opa(active_btn, LV_OPA_40, 0);

    /* 非激活态：恢复原来紫色玻璃风格 */
    lv_obj_set_style_bg_color(inactive_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(inactive_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(inactive_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_opa(inactive_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(inactive_btn, 0, 0);
}

static void reset_sort_btns(void)
{
    if(!sort_priority_btn || !sort_time_btn_obj) return;
    /* 两个都恢复紫色 */
    lv_color_t base = COLOR_GLASS_TINT;
    lv_obj_set_style_bg_color(sort_priority_btn, base, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_priority_btn, base, 0);
    lv_obj_set_style_border_opa(sort_priority_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_priority_btn, 0, 0);

    lv_obj_set_style_bg_color(sort_time_btn_obj, base, 0);
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_time_btn_obj, base, 0);
    lv_obj_set_style_border_opa(sort_time_btn_obj, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_time_btn_obj, 0, 0);

    /* 箭头也重置为 DOWN */
    lv_obj_t *lbl_p = lv_obj_get_child(sort_priority_btn, 0);
    lv_obj_t *lbl_t = lv_obj_get_child(sort_time_btn_obj, 0);
    lv_label_set_text(lbl_p, LV_SYMBOL_DOWN " Priority");
    lv_label_set_text(lbl_t, LV_SYMBOL_DOWN " Time");
    meeting_sort_asc = 0;
}

static void sort_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    meeting_sort_asc ^= 1;

    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, meeting_sort_asc
        ? LV_SYMBOL_UP   "  Priority"
        : LV_SYMBOL_DOWN " Priority");

    /* 高亮 Priority，还原 Time */
    set_sort_btn_active(sort_priority_btn, sort_time_btn_obj);

    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM - 1; i++) {
        for(uint8_t j = 0; j < MESSAGE_MEETING_NUM - 1 - i; j++) {
            uint8_t a = meeting_order[j];
            uint8_t b = meeting_order[j+1];
            bool do_swap = meeting_sort_asc
                ? (meeting_level[a] > meeting_level[b])
                : (meeting_level[a] < meeting_level[b]);
            if(do_swap) {
                meeting_order[j]   = b;
                meeting_order[j+1] = a;
            }
        }
    }
    rebuild_meeting_list();
}

static void sort_time_btn_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    static uint8_t time_sort_asc = 1;
    time_sort_asc ^= 1;

    lv_obj_t *lbl = lv_obj_get_child(btn, 0);
    lv_label_set_text(lbl, time_sort_asc
        ? LV_SYMBOL_UP   "  Time"
        : LV_SYMBOL_DOWN " Time");

    /* 高亮 Time，还原 Priority */
    set_sort_btn_active(sort_time_btn_obj, sort_priority_btn);

    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM - 1; i++) {
        for(uint8_t j = 0; j < MESSAGE_MEETING_NUM - 1 - i; j++) {
            uint8_t a = meeting_order[j];
            uint8_t b = meeting_order[j+1];
            int cmp = strncmp(meeting_time[a], meeting_time[b], 5);
            bool do_swap = time_sort_asc ? (cmp > 0) : (cmp < 0);
            if(do_swap) {
                meeting_order[j]   = b;
                meeting_order[j+1] = a;
            }
        }
    }
    rebuild_meeting_list();
}

static void meeting_refresh_cb(lv_event_t *e)
{
    (void)e;
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM; i++) meeting_order[i] = i;
    reset_sort_btns();       /* 两个按钮都恢复紫色 + 箭头重置 */
    rebuild_meeting_list();
}

//核心：重建列表（清空 + 重绘）
static void rebuild_meeting_list(void)
{
    if(!meeting_list_obj) return;
    lv_obj_clean(meeting_list_obj);   /* 清空所有子控件 */

    for(uint8_t oi = 0; oi < MESSAGE_MEETING_NUM; oi++)
    {
        uint8_t i = meeting_order[oi];
        lv_color_t c = level_color(meeting_level[i]);

        /* ---------- 条目外层卡片 ---------- */
        lv_obj_t *item = lv_obj_create(meeting_list_obj);
        lv_obj_set_size(item, lv_pct(100), 80);
        lv_obj_add_style(item, &style_card, LV_PART_MAIN);
        lv_obj_set_style_pad_all(item, 0, 0);
        lv_obj_set_scrollbar_mode(item, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

        /* ===== 左侧内容区（约 80% 宽）===== */
        lv_obj_t *left = lv_obj_create(item);
        lv_obj_set_size(left, lv_pct(80), lv_pct(100));
        lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_opa(left, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(left, 0, 0);
        lv_obj_set_style_pad_left(left, 18, 0);
        lv_obj_set_style_pad_right(left, 8, 0);
        lv_obj_set_style_pad_ver(left, 10, 0);
        lv_obj_set_scrollbar_mode(left, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        /* 左侧色条 */
        lv_obj_t *bar = lv_obj_create(left);
        lv_obj_set_size(bar, 4, lv_pct(70));
        lv_obj_align(bar, LV_ALIGN_LEFT_MID, -6, 0);
        lv_obj_set_style_bg_color(bar, c, 0);
        lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(bar, 0, 0);
        lv_obj_set_style_radius(bar, 2, 0);
        lv_obj_set_style_shadow_color(bar, c, 0);
        lv_obj_set_style_shadow_width(bar, 8, 0);
        lv_obj_set_style_shadow_opa(bar, LV_OPA_50, 0);
        lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);

        /* 时间标签 */
        lv_obj_t *time_lbl = lv_label_create(left);
        lv_label_set_text(time_lbl, meeting_time[i]);
        lv_obj_set_style_text_font(time_lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(time_lbl, c, 0);
        lv_obj_align(time_lbl, LV_ALIGN_TOP_LEFT, 8, 0);

        /* 内容标签 */
        lv_obj_t *content_lbl = lv_label_create(left);
        lv_label_set_text(content_lbl, meeting_content[i]);
        lv_obj_set_style_text_font(content_lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(content_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(content_lbl, LV_OPA_80, 0);
        lv_label_set_long_mode(content_lbl, LV_LABEL_LONG_DOT);
        lv_obj_set_width(content_lbl, lv_pct(90));
        lv_obj_align_to(content_lbl, time_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

        /* ===== 右侧分隔线 ===== */
        lv_obj_t *divider = lv_obj_create(item);
        lv_obj_set_size(divider, 1, lv_pct(70));
        lv_obj_align(divider, LV_ALIGN_RIGHT_MID, -lv_pct(20), 0);
        lv_obj_set_style_bg_color(divider, COLOR_WHITE, 0);
        lv_obj_set_style_bg_opa(divider, LV_OPA_20, 0);
        lv_obj_set_style_border_width(divider, 0, 0);
        lv_obj_set_style_radius(divider, 0, 0);
        lv_obj_clear_flag(divider, LV_OBJ_FLAG_CLICKABLE);

        /* ===== 右侧重要程度下拉区（约 20% 宽）===== */
        lv_obj_t *right = lv_obj_create(item);
        lv_obj_set_size(right, lv_pct(20), lv_pct(100));
        lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(right, 0, 0);
        lv_obj_set_style_pad_all(right, 6, 0);
        lv_obj_set_scrollbar_mode(right, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

        /* 重要程度下拉 */
        lv_obj_t *dd = lv_dropdown_create(right);
        lv_dropdown_set_options(dd,
            "Urgnt\n"   /* 0 重要   */
            "Impt\n"    /* 1 一般   */
            "Nrml");    /* 2 不重要 */
        lv_dropdown_set_selected(dd, meeting_level[i]);
        lv_obj_set_size(dd, lv_pct(100), 40);
        lv_obj_align(dd, LV_ALIGN_CENTER, 0, 0);

        /* 下拉框主体样式 */
        lv_obj_set_style_bg_color(dd, c, 0);
        lv_obj_set_style_bg_opa(dd, LV_OPA_30, 0);
        lv_obj_set_style_border_color(dd, c, 0);
        lv_obj_set_style_border_width(dd, 1, 0);
        lv_obj_set_style_border_opa(dd, LV_OPA_60, 0);
        lv_obj_set_style_radius(dd, 10, 0);
        lv_obj_set_style_text_color(dd, c, 0);
        lv_obj_set_style_text_font(dd, &lv_font_montserrat_20, 0);
        lv_obj_set_style_shadow_color(dd, c, 0);
        lv_obj_set_style_shadow_width(dd, 10, 0);
        lv_obj_set_style_shadow_opa(dd, LV_OPA_30, 0);
        lv_obj_set_style_pad_ver(dd, 0, 0);
        lv_obj_set_style_pad_hor(dd, 6, 0);

        /* 下拉展开列表样式 */
        lv_obj_t *list = lv_dropdown_get_list(dd);
        lv_obj_set_style_bg_color(list, lv_color_hex(0x3A49F2), 0);
        lv_obj_set_style_bg_opa(list, LV_OPA_90, 0);
        lv_obj_set_style_border_color(list, COLOR_WHITE, 0);
        lv_obj_set_style_border_width(list, 1, 0);
        lv_obj_set_style_border_opa(list, LV_OPA_50, 0);
        lv_obj_set_style_radius(list, 10, 0);
        lv_obj_set_style_text_color(list, COLOR_WHITE, 0);
        lv_obj_set_style_text_font(list, &lv_font_montserrat_20, 0);
        /* 选中高亮 */
        lv_obj_set_style_bg_color(list, COLOR_GLASS_TINT, LV_PART_SELECTED | LV_STATE_CHECKED);
        lv_obj_set_style_bg_opa(list, LV_OPA_40, LV_PART_SELECTED | LV_STATE_CHECKED);

        lv_obj_add_event_cb(dd, level_dropdown_cb, LV_EVENT_VALUE_CHANGED,
                            (void *)(uintptr_t)i);
    }
}

static void create_meeting_page(lv_obj_t *parent)
{
    /* 初始化顺序索引 */
    for(uint8_t i = 0; i < MESSAGE_MEETING_NUM; i++) meeting_order[i] = i;


    /* -------- 顶部标题卡片 -------- */
    lv_obj_t *header_card = create_glass_card(parent, lv_pct(92), 54);
    lv_obj_align(header_card, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_pad_hor(header_card, 16, 0);
    lv_obj_set_style_pad_ver(header_card, 0, 0);

    /* 标题文字 */
    lv_obj_t *title_lbl = lv_label_create(header_card);
    lv_label_set_text(title_lbl, "Messages For Meetings");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_lbl, COLOR_WHITE, 0);
    lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* 排序按钮 */
    /* Priority 排序按钮 */
    sort_priority_btn = lv_btn_create(header_card);
    lv_obj_set_size(sort_priority_btn, 110, 36);
    lv_obj_align(sort_priority_btn, LV_ALIGN_RIGHT_MID, -160, 0);
    lv_obj_set_style_radius(sort_priority_btn, 18, 0);
    lv_obj_set_style_bg_color(sort_priority_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_priority_btn, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_width(sort_priority_btn, 1, 0);
    lv_obj_set_style_border_opa(sort_priority_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_priority_btn, 0, 0);
    lv_obj_set_style_bg_opa(sort_priority_btn, LV_OPA_50, LV_STATE_PRESSED);

    lv_obj_t *sort_lbl = lv_label_create(sort_priority_btn);
    lv_label_set_text(sort_lbl, LV_SYMBOL_DOWN " Priority");
    lv_obj_set_style_text_font(sort_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sort_lbl, COLOR_WHITE, 0);
    lv_obj_center(sort_lbl);
    lv_obj_add_event_cb(sort_priority_btn, sort_btn_cb, LV_EVENT_CLICKED, NULL);

    /* Time 排序按钮 */
    sort_time_btn_obj = lv_btn_create(header_card);
    lv_obj_set_size(sort_time_btn_obj, 100, 36);
    lv_obj_align(sort_time_btn_obj, LV_ALIGN_RIGHT_MID, -54, 0);
    lv_obj_set_style_radius(sort_time_btn_obj, 18, 0);
    lv_obj_set_style_bg_color(sort_time_btn_obj, COLOR_GLASS_TINT, 0); // 初始同 Priority
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_30, 0);
    lv_obj_set_style_border_color(sort_time_btn_obj, COLOR_GLASS_TINT, 0);
    lv_obj_set_style_border_width(sort_time_btn_obj, 1, 0);
    lv_obj_set_style_border_opa(sort_time_btn_obj, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(sort_time_btn_obj, 0, 0);
    lv_obj_set_style_bg_opa(sort_time_btn_obj, LV_OPA_50, LV_STATE_PRESSED);

    lv_obj_t *sort_time_lbl = lv_label_create(sort_time_btn_obj);
    lv_label_set_text(sort_time_lbl, LV_SYMBOL_DOWN " Time");
    lv_obj_set_style_text_font(sort_time_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sort_time_lbl, COLOR_WHITE, 0);
    lv_obj_center(sort_time_lbl);
    lv_obj_add_event_cb(sort_time_btn_obj, sort_time_btn_cb, LV_EVENT_CLICKED, NULL);
    /* 刷新按钮保持不变，位置 ALIGN_RIGHT_MID 偏移 0 */


    /* 刷新按钮（右上角，玻璃风格小圆按钮） */
    lv_obj_t *refresh_btn = lv_btn_create(header_card);
    lv_obj_set_size(refresh_btn, 36, 36);
    lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(refresh_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(refresh_btn, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_20, 0);
    lv_obj_set_style_border_color(refresh_btn, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(refresh_btn, 1, 0);
    lv_obj_set_style_border_opa(refresh_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(refresh_btn, 0, 0);
    /* 按下态 */
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_40, LV_STATE_PRESSED);

    lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(refresh_icon, COLOR_WHITE, 0);
    lv_obj_center(refresh_icon);
    lv_obj_add_event_cb(refresh_btn, meeting_refresh_cb, LV_EVENT_CLICKED, NULL);

    /* -------- 列表容器（透明，flex 纵向，可滚动）-------- */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(92), lv_pct(76));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 10, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    meeting_list_obj = list;
    rebuild_meeting_list();   /* 首次填充 */



}

// Card 页 — 刷新回调
static void card_refresh_cb(lv_event_t *e)
{
    (void)e;
    rebuild_card_list();
}



static void rebuild_card_list(void)
{
    if(!card_list_obj) return;
    lv_obj_clean(card_list_obj);

    /* 检查是否有任何有效卡片 */
    bool any_valid = false;
    for(uint8_t i = 0; i < USER_CARD_NUM; i++) {
        if(user_cards[i].valid) { any_valid = true; break; }
    }

    /* 没有已交换卡片时显示占位提示 */
    if(!any_valid) {
        lv_obj_t *empty_card = lv_obj_create(card_list_obj);
        lv_obj_set_size(empty_card, lv_pct(100), 100);
        lv_obj_add_style(empty_card, &style_card, LV_PART_MAIN);
        lv_obj_set_scrollbar_mode(empty_card, LV_SCROLLBAR_MODE_OFF);

        lv_obj_t *empty_icon = lv_label_create(empty_card);
        lv_label_set_text(empty_icon, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(empty_icon, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(empty_icon, LV_OPA_30, 0);
        lv_obj_align(empty_icon, LV_ALIGN_CENTER, 0, -12);

        lv_obj_t *empty_lbl = lv_label_create(empty_card);
        lv_label_set_text(empty_lbl, "No exchanged cards yet");
        lv_obj_set_style_text_font(empty_lbl, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(empty_lbl, COLOR_WHITE, 0);
        lv_obj_set_style_text_opa(empty_lbl, LV_OPA_40, 0);
        lv_obj_align(empty_lbl, LV_ALIGN_CENTER, 0, 14);
        return;
    }

    /* 遍历有效卡片 */
    for(uint8_t i = 0; i < USER_CARD_NUM; i++)
    {
        if(!user_cards[i].valid) continue;

        user_card_t *u = &user_cards[i];
        lv_color_t gender_color = (u->gender == GENDER_MALE)
            ? lv_color_hex(0x64C8FF)   /* 男 — 天蓝 */
            : lv_color_hex(0xFF8FB0);  /* 女 — 樱粉 */

        /* ========== 动态计算卡片高度 ========== */
        lv_coord_t card_height = 140;  // 基础高度（头像+姓名+性别+分隔线+边距）
        card_height += u->position_count * 35;  // 每个职务约35px
        if(card_height < 200) card_height = 200;  // 最小高度

        /* ---------- 外层卡片容器 ---------- */
        lv_obj_t *card = lv_obj_create(card_list_obj);
        lv_obj_set_size(card, lv_pct(100), card_height);
        lv_obj_add_style(card, &style_card, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 16, 0);  // 16px内边距
        lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        /* ---------- 左侧性别色边条 ---------- */
        lv_obj_t *side_bar = lv_obj_create(card);
        lv_obj_set_size(side_bar, 6, card_height - 32);  // 上下留边
        lv_obj_align(side_bar, LV_ALIGN_LEFT_MID, -16, 0);  // 紧贴左边
        lv_obj_set_style_bg_color(side_bar, gender_color, 0);
        lv_obj_set_style_bg_opa(side_bar, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(side_bar, 0, 0);
        lv_obj_set_style_radius(side_bar, 3, 0);
        lv_obj_set_style_shadow_color(side_bar, gender_color, 0);
        lv_obj_set_style_shadow_width(side_bar, 10, 0);
        lv_obj_set_style_shadow_opa(side_bar, LV_OPA_50, 0);
        lv_obj_clear_flag(side_bar, LV_OBJ_FLAG_CLICKABLE);

        /* ---------- 左侧头像区（固定容器）---------- */
        lv_obj_t *avatar_cont = lv_obj_create(card);
        lv_obj_set_size(avatar_cont, 100, card_height -32);
        lv_obj_align(avatar_cont, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_style_bg_opa(avatar_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(avatar_cont, 0, 0);
        lv_obj_set_style_pad_all(avatar_cont, 0, 0);
        lv_obj_set_scrollbar_mode(avatar_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(avatar_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        /* 头像圆形 */
        lv_obj_t *avatar_circle = lv_obj_create(avatar_cont);
        lv_obj_set_size(avatar_circle, 80, 80);
        lv_obj_align(avatar_circle, LV_ALIGN_TOP_MID, 0, 10);  // 顶部居中，下移10px
        lv_obj_set_style_radius(avatar_circle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(avatar_circle, gender_color, 0);
        lv_obj_set_style_bg_opa(avatar_circle, LV_OPA_30, 0);
        lv_obj_set_style_border_color(avatar_circle, gender_color, 0);
        lv_obj_set_style_border_width(avatar_circle, 2, 0);
        lv_obj_set_style_border_opa(avatar_circle, LV_OPA_60, 0);
        lv_obj_set_style_pad_all(avatar_circle, 0, 0);
        lv_obj_set_style_clip_corner(avatar_circle, true, 0);
        lv_obj_set_scrollbar_mode(avatar_circle, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_scroll_dir(avatar_circle, LV_DIR_NONE);

        /* 头像图片 */
        lv_obj_t *avatar_img = lv_img_create(avatar_circle);
        lv_img_set_src(avatar_img, (u->gender == GENDER_MALE) ? &Man : &Woman);
        lv_obj_set_size(avatar_img, 80, 80);
        lv_img_set_size_mode(avatar_img, LV_IMG_SIZE_MODE_REAL);
        lv_img_set_zoom(avatar_img, 256);
        lv_img_set_antialias(avatar_img, true);
        lv_obj_center(avatar_img);

        /* ========== 关键修复：右侧信息区使用绝对定位 ========== */
        lv_obj_t *info_cont = lv_obj_create(card);
        // 固定宽度，不依赖动态计算
        lv_obj_set_size(info_cont, 480, card_height - 32);  // 假设总宽800，左侧占120
        lv_obj_align(info_cont, LV_ALIGN_LEFT_MID, 120, 0);  // 头像区100 + 间距20
        lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(info_cont, 0, 0);
        lv_obj_set_style_pad_all(info_cont, 0, 0);
        lv_obj_set_scrollbar_mode(info_cont, LV_SCROLLBAR_MODE_OFF);
        lv_obj_clear_flag(info_cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

        /* ========== 姓名（确保可见）========== */
        lv_obj_t *name_lbl = lv_label_create(info_cont);
        lv_label_set_text(name_lbl, u->name);
        lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_28, 0);  // 使用更大字体
        lv_obj_set_style_text_color(name_lbl, gender_color, 0);
        lv_obj_set_style_text_opa(name_lbl, LV_OPA_COVER, 0);  // 完全不透明
        lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(name_lbl, 460);
        lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 10);

        /* ========== 性别标签 ========== */
        lv_obj_t *gender_lbl = lv_label_create(info_cont);
        lv_label_set_text(gender_lbl, (u->gender == GENDER_MALE) ? "Male" : "Female");
        lv_obj_set_style_text_font(gender_lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(gender_lbl, COLOR_WHITE, 0);  // 改用白色更明显
        lv_obj_set_style_text_opa(gender_lbl, LV_OPA_70, 0);
        lv_obj_align_to(gender_lbl, name_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

        /* ========== 分隔线 ========== */
        lv_obj_t *div = lv_obj_create(info_cont);
        lv_obj_set_size(div, 450, 2);  // 固定宽度
        lv_obj_align_to(div, gender_lbl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_obj_set_style_bg_color(div, COLOR_WHITE, 0);
        lv_obj_set_style_bg_opa(div, LV_OPA_30, 0);
        lv_obj_set_style_border_width(div, 0, 0);
        lv_obj_set_style_radius(div, 1, 0);
        lv_obj_clear_flag(div, LV_OBJ_FLAG_CLICKABLE);

        /* ========== 职务列表 ========== */
        lv_obj_t *prev = div;
        for(uint8_t j = 0; j < u->position_count && j < POSITION_HOME_NUM; j++) {
            lv_obj_t *pos_lbl = lv_label_create(info_cont);

            // 使用简单格式，确保显示
            char buf[128];
            lv_snprintf(buf, sizeof(buf), "%s", u->position[j]);
            lv_label_set_text(pos_lbl, buf);

            lv_label_set_long_mode(pos_lbl, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(pos_lbl, 460);
            lv_obj_set_style_text_font(pos_lbl, &lv_font_montserrat_18, 0);
            lv_obj_set_style_text_color(pos_lbl, COLOR_WHITE, 0);
            lv_obj_set_style_text_opa(pos_lbl, LV_OPA_90, 0);  // 提高透明度
            lv_obj_align_to(pos_lbl, prev, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

            prev = pos_lbl;
        }
    }
}

static void create_card_page(lv_obj_t *parent)
{
    /* -------- 顶部标题卡片 -------- */
    lv_obj_t *header_card = create_glass_card(parent, lv_pct(92), 54);
    lv_obj_align(header_card, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_pad_hor(header_card, 14, 0);
    lv_obj_set_style_pad_ver(header_card, 0, 0);

    lv_obj_t *title_lbl = lv_label_create(header_card);
    lv_label_set_text(title_lbl, LV_SYMBOL_SHUFFLE "  Exchanged User Cards");
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title_lbl, COLOR_WHITE, 0);
    lv_obj_align(title_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* 刷新按钮 */
    lv_obj_t *refresh_btn = lv_btn_create(header_card);
    lv_obj_set_size(refresh_btn, 36, 36);
    lv_obj_align(refresh_btn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(refresh_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(refresh_btn, COLOR_WHITE, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_20, 0);
    lv_obj_set_style_border_color(refresh_btn, COLOR_WHITE, 0);
    lv_obj_set_style_border_width(refresh_btn, 1, 0);
    lv_obj_set_style_border_opa(refresh_btn, LV_OPA_50, 0);
    lv_obj_set_style_shadow_width(refresh_btn, 0, 0);
    lv_obj_set_style_bg_opa(refresh_btn, LV_OPA_40, LV_STATE_PRESSED);

    lv_obj_t *refresh_icon = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_color(refresh_icon, COLOR_WHITE, 0);
    lv_obj_center(refresh_icon);
    lv_obj_add_event_cb(refresh_btn, card_refresh_cb, LV_EVENT_CLICKED, NULL);

    /* -------- 列表容器 -------- */
    lv_obj_t *list = lv_obj_create(parent);
    lv_obj_set_size(list, lv_pct(92), lv_pct(76));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    lv_obj_set_style_pad_all(list, 0, 0);
    lv_obj_set_style_pad_row(list, 12, 0);
    lv_obj_set_layout(list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_OFF);

    card_list_obj = list;
    rebuild_card_list();
}


/* ========== UI 事件回调函数 ========== */

// PPT 上一页按钮回调
static void ppt_prev_cb(lv_event_t *e) {
    (void)e;
    if(ppt_current_page > 1) {
        ppt_current_page--;
        hw_ppt_prev_page();

        // 更新页码显示
        lv_obj_t *page_label = lv_event_get_user_data(e);
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "%d / %d", ppt_current_page, ppt_total_pages);
        lv_label_set_text(page_label, buf);
    }
}

// PPT 下一页按钮回调
static void ppt_next_cb(lv_event_t *e) {
    (void)e;
    if(ppt_current_page < ppt_total_pages) {
        ppt_current_page++;
        hw_ppt_next_page();

        // 更新页码显示
        lv_obj_t *page_label = lv_event_get_user_data(e);
        char buf[32];
        lv_snprintf(buf, sizeof(buf), "%d / %d", ppt_current_page, ppt_total_pages);
        lv_label_set_text(page_label, buf);
    }
}

/* ========== 开关控制回调 ========== */
static void switch_event_cb(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    uint32_t device_id = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    // 注意：颜色已在样式中通过 LV_STATE_CHECKED 自动切换，无需手动设置

    switch(device_id) {
        case 0: // 星闪
            ctrl_starflash_enabled = is_on;
            hw_starflash_set(is_on);
            break;
        case 1: // 蓝牙
            ctrl_bluetooth_enabled = is_on;
            hw_bluetooth_set(is_on);
            break;
        case 2: // NFC
            ctrl_nfc_enabled = is_on;
            hw_nfc_set(is_on);
            break;
        case 3: // WiFi
            ctrl_wifi_enabled = is_on;
            hw_wifi_set(is_on);
            break;
    }
}

// 亮度滑块回调
static void brightness_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *value_label = lv_event_get_user_data(e);

    int32_t value = lv_slider_get_value(slider);
    ctrl_brightness = (uint8_t)value;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)value);
    lv_label_set_text(value_label, buf);

    hw_brightness_set(ctrl_brightness);
}
// 音量滑块回调
static void volume_slider_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_obj_t *value_label = lv_event_get_user_data(e);

    int32_t value = lv_slider_get_value(slider);
    ctrl_volume = (uint8_t)value;

    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", (int)value);
    lv_label_set_text(value_label, buf);

    hw_volume_set(ctrl_volume);
}

/* ========== 辅助函数：创建开关卡片 ========== */
static lv_obj_t* create_switch_card(lv_obj_t *parent, const char *icon,
                                     const char *name, bool initial_state,
                                     uint32_t device_id, lv_color_t theme_color)
{
    /* 卡片容器 */
    lv_obj_t *card = lv_obj_create(parent);
    /* 修复：使用固定宽度而非百分比，确保两列布局 */
    lv_obj_set_size(card, 280, 100);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* 图标 */
    lv_obj_t *icon_lbl = lv_label_create(card);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_color(icon_lbl, theme_color, 0);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_28, 0);
    lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 名称 */
    lv_obj_t *name_lbl = lv_label_create(card);
    lv_label_set_text(name_lbl, name);
    lv_obj_set_style_text_color(name_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_opa(name_lbl, LV_OPA_80, 0);
    lv_obj_align(name_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    /* 开关 */
    lv_obj_t *sw = lv_switch_create(card);
    lv_obj_set_size(sw, 50, 28);
    lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, 0, 0);

    /* 修复：开关样式设置 - 分离主体和指示器样式 */
    // 主体（轨道）样式
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_50, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sw, 0, LV_PART_MAIN);

    // 指示器（背景色）- 关键修复：使用正确的状态组合
    // 关闭状态
    lv_obj_set_style_bg_color(sw, lv_color_hex(0x606060), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(sw, lv_color_hex(0x606060), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(sw, 10, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_opa(sw, LV_OPA_40, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    // 开启状态 - 关键修复：使用 LV_STATE_CHECKED
    lv_obj_set_style_bg_color(sw, theme_color, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_color(sw, theme_color, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_width(sw, 10, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_shadow_opa(sw, LV_OPA_40, LV_PART_INDICATOR | LV_STATE_CHECKED);

    // 旋钮（白色圆点）
    lv_obj_set_style_bg_color(sw, COLOR_WHITE, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, LV_PART_KNOB | LV_STATE_DEFAULT);

    /* 设置初始状态 */
    if(initial_state) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }

    /* 添加事件 */
    lv_obj_add_event_cb(sw, switch_event_cb, LV_EVENT_VALUE_CHANGED,
                        (void *)(uintptr_t)device_id);

    return card;
}

/* ========== 辅助函数：创建滑块卡片 ========== */
static lv_obj_t* create_slider_card(lv_obj_t *parent, const char *icon,
                                     const char *name, uint8_t initial_value,
                                     lv_event_cb_t callback, lv_color_t theme_color)
{
    /* 卡片容器 */
    lv_obj_t *card = lv_obj_create(parent);
    /* 修复：使用固定宽度 */
    lv_obj_set_size(card, 280, 120);
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_scrollbar_mode(card, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* 顶部：图标 + 数值 */
    lv_obj_t *top_cont = lv_obj_create(card);
    lv_obj_set_size(top_cont, lv_pct(100), 36);
    lv_obj_align(top_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_cont, 0, 0);
    lv_obj_set_style_pad_all(top_cont, 0, 0);
    lv_obj_clear_flag(top_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 图标 */
    lv_obj_t *icon_lbl = lv_label_create(top_cont);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_color(icon_lbl, theme_color, 0);
    lv_obj_set_style_text_font(icon_lbl, &lv_font_montserrat_28, 0);
    lv_obj_align(icon_lbl, LV_ALIGN_LEFT_MID, 0, 0);

    /* 数值标签 */
    lv_obj_t *value_lbl = lv_label_create(top_cont);
    char buf[8];
    lv_snprintf(buf, sizeof(buf), "%d%%", initial_value);
    lv_label_set_text(value_lbl, buf);
    lv_obj_set_style_text_color(value_lbl, theme_color, 0);
    lv_obj_set_style_text_font(value_lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(value_lbl, LV_ALIGN_RIGHT_MID, 0, 0);

    /* 名称 */
    lv_obj_t *name_lbl = lv_label_create(card);
    lv_label_set_text(name_lbl, name);
    lv_obj_set_style_text_color(name_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(name_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_opa(name_lbl, LV_OPA_70, 0);
    lv_obj_align(name_lbl, LV_ALIGN_TOP_LEFT, 0, 42);

    /* 滑块 */
    lv_obj_t *slider = lv_slider_create(card);
    lv_obj_set_size(slider, lv_pct(100), 10);
    lv_obj_align(slider, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, initial_value, LV_ANIM_OFF);

    /* 滑块样式 */
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(slider, LV_OPA_50, LV_PART_MAIN);
    lv_obj_set_style_radius(slider, 5, LV_PART_MAIN);

    lv_obj_set_style_bg_color(slider, theme_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(slider, 5, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_color(slider, theme_color, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_width(slider, 8, LV_PART_INDICATOR);
    lv_obj_set_style_shadow_opa(slider, LV_OPA_40, LV_PART_INDICATOR);

    lv_obj_set_style_bg_color(slider, COLOR_WHITE, LV_PART_KNOB);
    lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_KNOB);
    lv_obj_set_style_radius(slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
    lv_obj_set_style_pad_all(slider, 6, LV_PART_KNOB);
    lv_obj_set_style_shadow_color(slider, theme_color, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(slider, 12, LV_PART_KNOB);
    lv_obj_set_style_shadow_opa(slider, LV_OPA_30, LV_PART_KNOB);

    /* 添加事件 */
    lv_obj_add_event_cb(slider, callback, LV_EVENT_VALUE_CHANGED, value_lbl);

    return card;
}

/* ========== Controller 页面主函数（改进版）========== */
static void create_controller_page(lv_obj_t *parent)
{
    /* ==================== PPT 控制区 ==================== */
    lv_obj_t *ppt_card = create_glass_card(parent, lv_pct(92), 140);
    lv_obj_align(ppt_card, LV_ALIGN_TOP_MID, 0, 12);
    lv_obj_set_style_pad_all(ppt_card, 20, 0);

    /* PPT 标题 */
    lv_obj_t *ppt_title = lv_label_create(ppt_card);
    lv_label_set_text(ppt_title, LV_SYMBOL_PLAY "  PPT Controller");
    lv_obj_set_style_text_color(ppt_title, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(ppt_title, &lv_font_montserrat_24, 0);
    lv_obj_align(ppt_title, LV_ALIGN_TOP_LEFT, 0, 0);

    /* 页码显示 */
    lv_obj_t *page_label = lv_label_create(ppt_card);
    char page_buf[32];
    lv_snprintf(page_buf, sizeof(page_buf), "%d / %d", ppt_current_page, ppt_total_pages);
    lv_label_set_text(page_label, page_buf);
    lv_obj_set_style_text_color(page_label, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_text_font(page_label, &lv_font_montserrat_32, 0);
    lv_obj_align(page_label, LV_ALIGN_CENTER, 0, 10);

    /* 上一页按钮 */
    lv_obj_t *prev_btn = lv_btn_create(ppt_card);
    lv_obj_set_size(prev_btn, 100, 50);
    lv_obj_align(prev_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_radius(prev_btn, 15, 0);
    lv_obj_set_style_bg_color(prev_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_bg_opa(prev_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(prev_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_border_width(prev_btn, 1, 0);
    lv_obj_set_style_border_opa(prev_btn, LV_OPA_60, 0);
    lv_obj_set_style_shadow_color(prev_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_shadow_width(prev_btn, 10, 0);
    lv_obj_set_style_shadow_opa(prev_btn, LV_OPA_30, 0);
    lv_obj_set_style_bg_opa(prev_btn, LV_OPA_50, LV_STATE_PRESSED);

    lv_obj_t *prev_lbl = lv_label_create(prev_btn);
    lv_label_set_text(prev_lbl, LV_SYMBOL_LEFT "  Prev");
    lv_obj_set_style_text_color(prev_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(prev_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(prev_lbl);

    lv_obj_add_event_cb(prev_btn, ppt_prev_cb, LV_EVENT_CLICKED, page_label);

    /* 下一页按钮 */
    lv_obj_t *next_btn = lv_btn_create(ppt_card);
    lv_obj_set_size(next_btn, 100, 50);
    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_radius(next_btn, 15, 0);
    lv_obj_set_style_bg_color(next_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_bg_opa(next_btn, LV_OPA_30, 0);
    lv_obj_set_style_border_color(next_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_border_width(next_btn, 1, 0);
    lv_obj_set_style_border_opa(next_btn, LV_OPA_60, 0);
    lv_obj_set_style_shadow_color(next_btn, lv_color_hex(0x64FFDA), 0);
    lv_obj_set_style_shadow_width(next_btn, 10, 0);
    lv_obj_set_style_shadow_opa(next_btn, LV_OPA_30, 0);
    lv_obj_set_style_bg_opa(next_btn, LV_OPA_50, LV_STATE_PRESSED);

    lv_obj_t *next_lbl = lv_label_create(next_btn);
    lv_label_set_text(next_lbl, "Next  " LV_SYMBOL_RIGHT);
    lv_obj_set_style_text_color(next_lbl, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(next_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(next_lbl);

    lv_obj_add_event_cb(next_btn, ppt_next_cb, LV_EVENT_CLICKED, page_label);

    /* ==================== 设置中心标题 ==================== */
    lv_obj_t *settings_title_card = create_glass_card(parent, lv_pct(92), 50);
    lv_obj_align(settings_title_card, LV_ALIGN_TOP_MID, 0, 170);

    lv_obj_t *settings_title = lv_label_create(settings_title_card);
    lv_label_set_text(settings_title, LV_SYMBOL_SETTINGS "  Settings Center");
    lv_obj_set_style_text_color(settings_title, COLOR_WHITE, 0);
    lv_obj_set_style_text_font(settings_title, &lv_font_montserrat_24, 0);
    lv_obj_center(settings_title);

    /* ==================== 修复：设置卡片容器 ==================== */
    lv_obj_t *settings_cont = lv_obj_create(parent);
    /* 关键修复：确保容器宽度足够容纳两列卡片 */
    lv_obj_set_size(settings_cont, 600, lv_pct(47));  // 宽度：280*2 + 间距40 = 600
    lv_obj_align(settings_cont, LV_ALIGN_TOP_MID, 0, 232);
    lv_obj_set_style_bg_opa(settings_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(settings_cont, 0, 0);
    lv_obj_set_style_pad_all(settings_cont, 0, 0);
    lv_obj_set_style_pad_row(settings_cont, 12, 0);      // 行间距12px
    lv_obj_set_style_pad_column(settings_cont, 40, 0);   // 列间距40px（固定值）
    lv_obj_set_layout(settings_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_scrollbar_mode(settings_cont, LV_SCROLLBAR_MODE_OFF);

    /* ==================== 创建设置卡片（三行两列）==================== */

    /* 第一行 */
    create_switch_card(settings_cont, LV_SYMBOL_WIFI, "StarFlash",
                      ctrl_starflash_enabled, 0, lv_color_hex(0xFF6B9D));

    create_switch_card(settings_cont, LV_SYMBOL_BLUETOOTH, "Bluetooth",
                      ctrl_bluetooth_enabled, 1, lv_color_hex(0x64C8FF));

    /* 第二行 */
    create_switch_card(settings_cont, "NFC", "NFC",
                      ctrl_nfc_enabled, 2, lv_color_hex(0xFFD74E));

    create_switch_card(settings_cont, LV_SYMBOL_WIFI, "WiFi",
                      ctrl_wifi_enabled, 3, lv_color_hex(0x69FF7A));

    /* 第三行 */
    create_slider_card(settings_cont, LV_SYMBOL_EYE_OPEN, "Brightness",
                      ctrl_brightness, brightness_slider_cb, lv_color_hex(0xFFD74E));

    create_slider_card(settings_cont, LV_SYMBOL_VOLUME_MAX, "Volume",
                      ctrl_volume, volume_slider_cb, lv_color_hex(0x64FFDA));
}

/* 函数指针表，对应页面创建函数 */
typedef void (*page_create_fn)(lv_obj_t *);
static const page_create_fn page_creators[PAGE_COUNT] = {
    create_home_page,
    create_meeting_page,
    create_card_page,
    create_controller_page,
};

/* =====================================================================
 * 懒加载页面切换：首次切换才 create，之后直接 show/hide
 * ===================================================================== */
static void show_page(uint8_t index)
{
    /* 懒加载：首次才调用对应 create 函数 */
    if (!page_created[index]) {
        page_creators[index](pages[index]);
        page_created[index] = true;
    }

    /* 切换显隐 */
    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        if (i == index) lv_obj_clear_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
        else            lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* 更新 dock 按钮选中态 */
    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        if (i == index) {
            lv_obj_add_style(tab_btns[i], &style_dock_btn_active, LV_PART_MAIN);
        } else {
            lv_obj_remove_style(tab_btns[i], &style_dock_btn_active, LV_PART_MAIN);
        }
    }

    current_page = index;
}

/* Dock 按钮点击回调 */
static void tab_btn_event_cb(lv_event_t *e)
{
    uint32_t idx = (uint32_t)(uintptr_t)lv_event_get_user_data(e);
    show_page((uint8_t)idx);
}

//主程序
void lv_mainstart(void)
{

    //主要显示的API
    //HOME
    // 修改此处更换姓名
    usr_name ="User";
    home_time="12:20";
    home_date="2026/01/01    Monday";
    // 电量，整数 0~100
    batt_pct=70;
    // 职务信息
    position[0]="Chief Technology Officer (CTO)";
    position[1]="Vice President of Engineering";
    position[2]="Engineering Manager";
    position[3]="Senior Software Engineer";
    //Meeting
    //meeting_time
    meeting_time[0]="09:00 - 09:30";
    meeting_time[1]="10:00 - 11:00";
    meeting_time[2]="13:30 - 14:00";
    meeting_time[3]="15:00 - 16:30";
    meeting_time[4]="17:00 - 17:30";
    //meeting_content
    meeting_content[0]="Morning standup sync";
    meeting_content[1]="Product review with design team";
    meeting_content[2]="Client demo preparation";
    meeting_content[3]="Q3 roadmap planning session";
    meeting_content[4]="Daily wrap-up & retrospective";
    //level_text
    level_text[0]  = "Extremely Important";
    level_text[1]  = "Important";
    level_text[2]  = "Average";
    // 0=重要  1=一般  2=不重要
    meeting_level[0]=0;
    meeting_level[1]=1;
    meeting_level[2]=2;
    meeting_level[3]=0;
    meeting_level[4]=1;
    //Card 页初始化
    user_cards[0].valid          = true;
    user_cards[0].name           = "Alice Chen";
    user_cards[0].gender         = GENDER_FEMALE;
    user_cards[0].position[0]    = "Product Manager";
    user_cards[0].position[1]    = "UX Lead";
    user_cards[0].position_count = 2;

    user_cards[1].valid          = true;
    user_cards[1].name           = "Bob Zhang";
    user_cards[1].gender         = GENDER_MALE;
    user_cards[1].position[0]    = "Senior Engineer";
    user_cards[1].position[1]    = "Tech Lead";
    user_cards[1].position[2]    = "Architect";
    user_cards[1].position_count = 3;

    user_cards[2].valid          = false; // 未交换，显示占位
    user_cards[2].name           = "";
    user_cards[2].position_count = 0;
    //Controller
    ppt_current_page = 1;    // 当前页码
    ppt_total_pages = 20;    // 总页数




    setup_styles();

    //根屏幕：渐变背景，不接收点击（避免遮挡子控件）
    lv_obj_t *screen = lv_scr_act();
    lv_obj_add_style(screen, &style_screen_bg, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    /* ================================================================
     * 页面容器区（透明，不遮背景）
     * 占屏幕上方 85% 高度
     * ================================================================ */
    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        page_created[i] = false;

        pages[i] = lv_obj_create(screen);
        lv_obj_set_size(pages[i], lv_pct(100), lv_pct(85));
        lv_obj_align(pages[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_add_style(pages[i], &style_page, LV_PART_MAIN);  // 透明容器
        lv_obj_set_scrollbar_mode(pages[i], LV_SCROLLBAR_MODE_OFF);

        /* 默认全部隐藏，等 show_page(0) 来显示第一页 */
        lv_obj_add_flag(pages[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* ================================================================
     * Dock 栏 — Apple Liquid Glass 风格
     *
     * 结构：
     *   screen
     *   └─ dock_wrap（胶囊容器，完全透明背景的外层定位对象）
     *      └─ dock_pill（磨砂玻璃胶囊，真正的视觉容器）
     *         └─ [btn × PAGE_COUNT]（flex row）
     *
     * 关键：dock_wrap 背景透明，只负责定位；
     *        dock_pill 负责视觉；两者背景不叠加遮挡渐变背景。
     * ================================================================ */

    /* 外层定位容器（透明） */
    lv_obj_t *dock_wrap = lv_obj_create(screen);
    lv_obj_set_size(dock_wrap, lv_pct(100), 86);
    lv_obj_align(dock_wrap, LV_ALIGN_BOTTOM_MID, 0, -10); // 距底部 10px
    lv_obj_set_style_bg_opa(dock_wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dock_wrap, 0, 0);
    lv_obj_set_style_pad_all(dock_wrap, 0, 0);
    lv_obj_set_scrollbar_mode(dock_wrap, LV_SCROLLBAR_MODE_OFF);
    /* 不接收事件（让子控件自行处理） */
    lv_obj_clear_flag(dock_wrap, LV_OBJ_FLAG_CLICKABLE);

    /* 磨砂玻璃胶囊（实际视觉 dock） */
    lv_obj_t *dock_pill = lv_obj_create(dock_wrap);
    /* 宽度 = 按钮数 * (按钮宽+间距) + 左右内边距，此处自适应 */
    lv_obj_set_height(dock_pill, 70);
    lv_obj_set_width(dock_pill, LV_SIZE_CONTENT);   // 自适应内容宽
    lv_obj_align(dock_pill, LV_ALIGN_CENTER, 0, 0); // 水平居中
    lv_obj_add_style(dock_pill, &style_dock_wrap, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(dock_pill, LV_SCROLLBAR_MODE_OFF);
    /* Flex 横排 */
    lv_obj_set_layout(dock_pill, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dock_pill, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(dock_pill, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(dock_pill, 14, 0);
    lv_obj_set_style_pad_hor(dock_pill, 20, 0);
    lv_obj_set_style_pad_ver(dock_pill, 8, 0);

    static const lv_color_t icon_colors[PAGE_COUNT] = {
        LV_COLOR_MAKE(0xC7, 0x35, 0x17),  // Home    — 珊瑚橙
        LV_COLOR_MAKE(0x05, 0x91, 0x1F),  // Meeting — 薄荷绿
        LV_COLOR_MAKE(0xFF, 0xD7, 0x4E),  // Card    — 琥珀黄
        LV_COLOR_MAKE(0x11, 0x09, 0x91),  // Ctrl    — 天蓝
    };

    /* 按钮创建 */
    for (uint8_t i = 0; i < PAGE_COUNT; i++) {
        lv_obj_t *btn = lv_btn_create(dock_pill);
        lv_obj_set_size(btn, 90, 54);   // 更宽更高，图标+文字两行更宽松
        lv_obj_add_style(btn, &style_dock_btn_normal, LV_PART_MAIN);
        /* 按下态：稍微降低透明度反馈 */
        lv_obj_set_style_bg_opa(btn, LV_OPA_20, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, COLOR_WHITE, LV_STATE_PRESSED);

        /* 按钮内容：图标 + 文字竖排 */
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, page_icons[i]);
        lv_obj_set_style_text_color(icon, icon_colors[i], 0);
        lv_obj_set_style_text_opa(icon, LV_OPA_COVER, 0);

        /* 图标下方极小文字，营造 iOS dock 感，同时消除 page_names unused warning */
        lv_obj_t *name_l = lv_label_create(btn);
        lv_label_set_text(name_l, page_names[i]);
        lv_obj_set_style_text_font(name_l, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_color(name_l, icon_colors[i], 0);
        lv_obj_set_style_text_opa(name_l, LV_OPA_70, 0);

        lv_obj_add_event_cb(btn, tab_btn_event_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        tab_btns[i] = btn;
    }
    // 初始显示第一页（懒加载会在此时调用 create_person_page）
    show_page(0);
}


// ========== 硬件接口函数（占位，待实现）==========

// PPT 页码获取接口（供硬件调用）
void hw_ppt_get_page_info(uint16_t *current, uint16_t *total) {
    if(current) *current = ppt_current_page;
    if(total) *total = ppt_total_pages;
}

// PPT 页码设置接口（供硬件调用）
void hw_ppt_set_page_info(uint16_t current, uint16_t total) {
    if(current > 0 && current <= total) {
        ppt_current_page = current;
    }
    if(total > 0) {
        ppt_total_pages = total;
    }
    printf("[HW] PPT page info updated: %d / %d\n", ppt_current_page, ppt_total_pages);
}


// PPT 控制接口
static void hw_ppt_prev_page(void) {
    printf("[HW] PPT: Previous page requested, current page: %d\n", ppt_current_page);
    // TODO: 实际硬件控制代码
}

static void hw_ppt_next_page(void) {
    printf("[HW] PPT: Next page requested, current page: %d\n", ppt_current_page);
    // TODO: 实际硬件控制代码
}

// 星闪控制接口
static void hw_starflash_set(bool enable) {
    printf("[HW] StarFlash: %s\n", enable ? "Enabled" : "Disabled");
    // TODO: 实际硬件控制代码
}

// 蓝牙控制接口
static void hw_bluetooth_set(bool enable) {
    printf("[HW] Bluetooth: %s\n", enable ? "Enabled" : "Disabled");
    // TODO: 实际硬件控制代码
}

// NFC 控制接口
static void hw_nfc_set(bool enable) {
    printf("[HW] NFC: %s\n", enable ? "Enabled" : "Disabled");
    // TODO: 实际硬件控制代码
}

// WiFi 控制接口
static void hw_wifi_set(bool enable) {
    printf("[HW] WiFi: %s\n", enable ? "Enabled" : "Disabled");
    // TODO: 实际硬件控制代码
}

// 亮度控制接口
static void hw_brightness_set(uint8_t value) {
    printf("[HW] Brightness: %d%%\n", value);
    // TODO: 实际硬件控制代码
}

// 音量控制接口
static void hw_volume_set(uint8_t value) {
    printf("[HW] Volume: %d%%\n", value);
    // TODO: 实际硬件控制代码
}
