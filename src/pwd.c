u8 pwd[PWD_MAXLEN];

u8* GetPassword(){
    return pwd;
}
u8 SetPassword(u8* new_pwd){
    strcpy(pwd, new_pwd);
    if(flash_write() == 0){
        return PWD_SET;
    }else{
        return PWD_FLASH_ERROR;
    }
}