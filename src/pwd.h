#ifndef _PWD_H_
#define _PWD_H_

#define PWD_MAXLEN 4*16

#define PWD_SET (0)
#define PWD_FLASH_ERROR (1)

extern u8 pwd[PWD_MAXLEN];

u8* GetPassword();
u16 SetPassword(u8* new_pwd);

#endif