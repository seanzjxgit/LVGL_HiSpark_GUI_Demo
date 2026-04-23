#ifndef LV_MAINSTART_H
#define LV_MAINSTART_H

#ifdef __cplusplus
extern "C" {
#endif

void lv_mainstart(void);

#ifdef __cplusplus
}
#endif
#include <stdint.h>

#define POSITION_HOME_NUM   4
#define MESSAGE_MEETING_NUM 5
/* 本机用户ID（和 nfc_checkin.h 里保持一致）*/
#define THIS_DEVICE_USER_ID   "000001"

extern char     *usr_name;
extern char     *home_time;
extern char     *home_date;
extern uint8_t   batt_pct;

extern char     *position[POSITION_HOME_NUM];

extern char     *meeting_time[MESSAGE_MEETING_NUM];
extern char     *meeting_content[MESSAGE_MEETING_NUM];
extern uint8_t   meeting_level[MESSAGE_MEETING_NUM];
extern uint8_t   meeting_order[MESSAGE_MEETING_NUM];

extern char     *level_text[3];





#endif
