#include "ucs2.h"

static u8 ucs2_rv[5]; /// Memory to store returned value for fromUCS2 function

void strToCP1251(u8 *dst, u8 *src){
	u8 l = strlen((const char *)src), i;
	for(i = 0; i < l / 4; i++){
        u8 tmp = src[(i + 1) * 4];
        src[(i + 1) * 4] = '\0';
		dst[i] = toCP1251((u8 *)(src + i * 4));
        src[(i + 1) * 4] = tmp;
	}
}

void strToUCS2(u8 *dst, u8 *src){
	u8 l = strlen((const char *)src), i, j;
	for(i = 0; i < l; i++){
        u8 *b = toUCS2(src[i]);
        for(j = 0; j < 4; j++){
           dst[i*4 + j] = b[j];
        }
	}
    dst[i*4] = 0;
}

/*!
    \brief Converts UCS-2 symbols with the codes within 0x0020..0x007E and 0x0410..0x0440 to CP1251.
		\param[in] c String like "0028"
*/
u8 toCP1251(u8 *c){
	u8 l = strlen((char const *)c);
	u16 t = 0;
	for(u8 i = 0; i < l; i++){
		t *= 16;
		t += hextodig(c[i]);
	}
	if(ENG_BEGIN <= t && t <= ENG_END){
		return t; // english characters in Windows 1251 the same as in Unicode
	}else if(UNICODE_RUS_BEGIN <= t && t <= UNICODE_RUS_END){
		return CP1251_RUS_BEGIN + (t - UNICODE_RUS_BEGIN);
	}
	return 0x20; // It must not be executed. Return space character if there is no such symbol in table.
}

/*!
    \brief Converts CP1251 symbols with codes within 0x20..0x7E and 0xC0..0xFF to UCS-2.
		\param[in] c CP1251 character code
*/
u8 *toUCS2(u8 c){
	u16 t;
	if(ENG_BEGIN <= c && c <= ENG_END){
		t = c; // english characters in Windows 1251 the same as in Unicode
	}else if(CP1251_RUS_BEGIN <= c && c <= CP1251_RUS_END){
		t = UNICODE_RUS_BEGIN + (c - CP1251_RUS_BEGIN);
	}else{
		t = 0x0020;// space charachter
	};
	for(u8 i = 3; i != 0xFF; i--){
		ucs2_rv[i] = digtohex(t % 16);
		t /= 16;
	}
	ucs2_rv[4] = '\0';
	return ucs2_rv;
}

/*!
    \brief Converts characters of hexademical digits '0'..'1','A'..'F' to an appropriate number.
		\param[in] c Character
*/
static u8 hextodig(u8 c){
	if(c >= '0' && c <= '9'){
		return c - '0';
	}else if(c >= 'A' && c <= 'F'){
		return 10 + (c - 'A');
	}else{
		return 0;
	}
}

/*!
    \brief Converts number 0..15 to an appropriate symbol '0'..'F' of hexademical digit
		\param[in] c Number
*/
static u8 digtohex(u8 n){
	if(n <= 9){
		return '0' + n;
	}else if (n >= 10 && n <= 15){
		return 'A' + (n - 10);
	}else{
		return 0;
	}
}