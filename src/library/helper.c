#include "helper.h"

void reverse(uint8_t* p_buf, uint32_t size)
{
    int     i, j;
    uint8_t tmp;

    for (i=0, j=size-1; i<j; i++, j--)
    {
        tmp = p_buf[i];
        p_buf[i] = p_buf[j];
        p_buf[j] = tmp;
    }
}
