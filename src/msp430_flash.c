#include "msp430_flash.h"

/*!
    \brief Writes the data in flash

    The function writes given data in available space in flash.
    Boundaries of available space are set in header by user.
    The function checks the written data and returns 0 if it's OK.
    Otherwise the functions returns 2. If the allocated space in
    flash isn't enough the function returns 1.
*/
u8 flash_write(u8 *src, u16 num){
    // If allocated space in flash isn't enough
    if(num > END_AVAILABLAE_SPACE - BEGIN_AVAILABLE_SPACE + 1){
        return 1;
    }

    // Number of segments to erase before writing the data
    u16 snr = num / SEG_SIZE;
    if(num - snr * SEG_SIZE > 0) snr++;

    // Erasing necessary segments
    for(u16 i = 0; i < snr; i++){
        while(FCTL3 & BUSY); // Wait untill the flash is busy
        FCTL3 = FWPW | ((FCTL3 & ~LOCK) & 0xFF); // Unlock main flash
        FCTL1 = FWPW + ERASE; // Set segment erase operation
        *((u8 *)(BEGIN_AVAILABLE_SPACE + i*0x0200)) = 0x00; // Dummy read
        while(FCTL3 & BUSY); // Wait untill the flash is busy
        FCTL3 = FWPW + LOCK; // Lock main flash again
    }

    // Writing byte-by-byte TODO: with words and long words it will be faster
    for(u16 i = 0; i < num; i++){
        FCTL3 = FWPW | ((FCTL3 & ~LOCK) & 0xFF); // Unlock main flash
        FCTL1 = FWKEY + WRT; // Set WRT bit for write operation
        *((u8 *)(BEGIN_AVAILABLE_SPACE + i)) = *(src+i);
        FCTL1 = FWKEY; // Clear WRT flag
        FCTL3 = FWPW + LOCK; // Lock main flash again
    }

    // Compare written data with the original
    for(u16 i = 0; i < num; i++){
        if(*((u8 *)(BEGIN_AVAILABLE_SPACE + i)) != *(src+i)){
            return 2;
        }
    }

    return 0;
}

/*!
    \brief Read the required number of bytes from allocated flash space.
*/
u8 flash_read(u8 *dst, u16 num){
    // If allocated space in flash isn't enough
    if(num > END_AVAILABLAE_SPACE - BEGIN_AVAILABLE_SPACE + 1){
        return 1;
    }

    // Read the data
    for(u16 i = 0; i < num; i++){
        *(dst + i) = *((u8 *)(BEGIN_AVAILABLE_SPACE + i));
    }

    return 0;
}
