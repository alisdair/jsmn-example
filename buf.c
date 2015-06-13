/* buf: a sized buffer type. */

#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "log.h"

buf_t * buf_size(buf_t *buf, size_t len)
{
    if (buf == NULL)
    {
        buf = malloc(sizeof(buf_t));
        log_null(buf);
        buf->data = NULL;
        buf->len = 0;
    }

    buf->data = realloc(buf->data, len);
    log_null(buf->data);
    
    if (buf->len > len)
        buf->len = len;
    buf->limit = len;

    return buf;
}

void buf_push(buf_t *buf, uint8_t c)
{
    log_null(buf);

    log_assert(buf->len < buf->limit);

    buf->data[buf->len++] = c;
}

void buf_concat(buf_t *dst, uint8_t *src, size_t len)
{
    log_null(dst);
    log_null(src);

    log_assert(dst->len + len <= dst->limit);

    for (size_t i = 0; i < len; i++)
        dst->data[dst->len++] = src[i];
}

char * buf_tostr(buf_t *buf)
{
    log_null(buf);

    char *str = malloc(buf->len + 1);
    log_null(str);

    memcpy(str, buf->data, buf->len);
    str[buf->len] = '\0';

    return str;
}
