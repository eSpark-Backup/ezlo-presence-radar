#include "debug.h"

#include <stdio.h>
#include <string.h>

static void put_idump(uint8_t *buff, uint32_t ofs, uint32_t cnt)
{
    int n;

    printf("\n%08lX ", ofs);

    for (n = 0; n < cnt; n++)
    {
        printf(" %02X", buff[n]);
    }

    if (cnt < 16)
    {
        do
        {
            ets_printf("   ");

        } while (++n < 16);
    }

    char temp_buff[17] = {0};

    memcpy(temp_buff, buff, cnt);
    temp_buff[16] = 0;

    printf("%c%c", 0x09, 0x09);

    for (n = 0; n < cnt; n++)
    {
        if ((buff[n] < 0x20) || (buff[n] > 0x80))
        {
            printf(".");
        }
        else
        {
            printf("%c", buff[n]);
        }
    }
}

void __dump(const char *file_name, uint32_t line, void *_buff, uint32_t ofs, uint32_t cnt)
{
    unsigned char *buff = _buff;
    int lines = cnt >> 4;
    int l;

    printf("%s (%d): Total Size: %d\r\n", file_name, line, cnt);

    for (l = 0; l < lines; l++)
    {
        put_idump(&buff[l * 16], ofs + l * 16, 16);
    }

    if (cnt & 0x0F)
    {
        put_idump(&buff[l * 16], ofs + l * 16, cnt & 0x0F);
    }

    printf("\n\n");
    fflush(stdout);
}
