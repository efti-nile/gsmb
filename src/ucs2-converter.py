while True:
    str = input()
    str2 = ''
    for c in str:
        str2 = str2 + '0' + hex(ord(c))[2:]
    print(str2.upper())
    
