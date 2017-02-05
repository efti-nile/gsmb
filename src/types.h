#ifndef _TYPES_H_
#define _TYPES_H_

#define CIRBUF_SIZE 300

#define SMS_QUEUE_MAXSIZE (10+1)
#define SMS_TEXT_MAXLEN (200+1)
#define SMS_TELNUM_LEN (44+1)

#define VALVE_NAME_MAXLEN (10+1)

#define LED_ON     P5OUT |= BIT3
#define LED_OFF    P5OUT &= ~BIT3
#define LED_TOGGLE P5OUT ^= BIT3

typedef unsigned long int u32;
typedef signed long int s32;

typedef unsigned short int u16;
typedef signed short int s16;

typedef unsigned char u8;
typedef signed char s8;

struct State_TypeDef{
    // Telephone number of the last received SMS command
    u8 TelNumOfSourceOfRequest[SMS_TELNUM_LEN];
    // Controller address in the RS-485 network
    u8 controller_address;
    // During valve command execution name of the current group stored in this string
    u8 current_valves_group[VALVE_NAME_MAXLEN];
    // Flags
    u8 request_close_valves;
    u8 request_open_valves;
    u8 leak_prev;
    u8 leak_now;
    u8 check_battery_prev;
    u8 check_battery_now;
    u8 link_lost_prev;
    u8 link_lost_now;
    u8 leak_removed_prev;
    u8 leak_removed_now;
    u8 link_ok_with_main_controller_prev;
    u8 link_ok_with_main_controller_now;
    u8 battery_ok_in_gsm_extender_prev;
    u8 battery_ok_in_gsm_extender_now;
    u8 ext_supply_ok_prev;
    u8 ext_supply_ok_now;
    u8 initialization_in_progress;
    // Timers and timeouts
    u16 sms_timer;
    u16 close_valves_timeout;
    u16 open_valves_timeout;
    u16 controller_link_timeout;
    u16 leak_flag_timeout;
    u16 leak_removed_flag_timeout;
    u16 link_lost_flag_timeout;

    // BURNER STATES
    u8 request_burner_switch_off;
    u8 request_burner_switch_on;
    u8 request_sen_get;
    u8 request_night_mode_on;
    u8 request_night_mode_off;
    u8 request_recv_setiings;

    u8 request_set_day_temp;
    u8 temp_day_max;
    u8 temp_day_min;

    u8 request_set_night_temp;
    u8 temp_night_max;
    u8 temp_night_min;

    u8 request_set_night_time;
    u8 night_begin;
    u8 night_end;
};

struct InPack_TypeDef{
    u8 Length;
    u8 COMMAND;
    u8 Optional[SMS_TEXT_MAXLEN];
    u8 crc;
};

struct OutPack_TypeDef{
    u8 Length;
    u8 COMMAND;
	u8 Optional[20];
    u8 crc;
};

struct SMS_Queque_TypeDef{
    struct SMS_TypeDef{
        u8 LifeTime;
        u8 TelNum[SMS_TELNUM_LEN];
        u8 *SmsText;
    }List[SMS_QUEUE_MAXSIZE];
    u8 FirstItem;
    u8 LastItem;
    u8 NumItems;
};

// Telephone dirictory size
#define TELDIR_TELNUM_LEN (44+1) // Length of the one telephone number (UCS2 coding)
#define TELDIR_SIZE 10 // Number of slots for telephone numbers
#define TELDIR_PWD_MAXLEN (4*10+1)

struct TelDir_TypeDef{
    u8 NumItems;
    u8 List[TELDIR_SIZE][TELDIR_TELNUM_LEN];
	u8 BalanceTelNum[TELDIR_TELNUM_LEN];
    u8 isBalanceTelNumSet;
    u8 Iterator;
    u8 Pwd[TELDIR_PWD_MAXLEN];
};

// SMS pool
#define SMSPOOL_SMS_MAXLEN (160 + 1)
#define SMSPOOL_SIZE 3

struct SmsPool_Item_TypeDef{
    u8 Text[SMSPOOL_SMS_MAXLEN];
    u8 Qty;
};

#endif