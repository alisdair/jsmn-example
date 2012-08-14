#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "jsmn.h"

#include "log.h"
#include "buf.h"

char URL[] = "https://api.github.com/users/alisdair";
char *KEYS[] = { "name", "location", "public_repos", "hireable" };

#define BUFFER_SIZE 32768
#define JSON_TOKENS 256

static size_t fetch_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
    buf_t *buf = (buf_t *) userp;
    size_t total = size * nmemb;

    if (buf->limit - buf->len < total)
    {
        buf = buf_size(buf, buf->limit + total);
        log_null(buf);
    }

    buf_concat(buf, buffer, total);

    return total;
}

char * fetch_json(char *url)
{
    CURL *curl = curl_easy_init();
    log_null(curl);

    curl_easy_setopt(curl, CURLOPT_URL, url);

    buf_t *buf = buf_size(NULL, BUFFER_SIZE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fetch_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buf);

    struct curl_slist *hs = curl_slist_append(NULL, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        log_die("curl_easy_perform failed: %s", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
    curl_slist_free_all(hs);

    char *js = buf_tostr(buf);
    free(buf);

    return js;
}

jsmntok_t * parse_json(char *js)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    size_t n = JSON_TOKENS;
    jsmntok_t *tokens = malloc(sizeof(jsmntok_t) * n);
    log_null(tokens);

    int ret = jsmn_parse(&parser, js, tokens, n);

    while (ret == JSMN_ERROR_NOMEM)
    {
        n = n * 2 + 1;
        tokens = realloc(tokens, sizeof(jsmntok_t) * n);
        log_null(tokens);
        ret = jsmn_parse(&parser, js, tokens, n);
    }

    if (ret == JSMN_ERROR_INVAL)
        log_die("jsmn_parse: invalid JSON string");
    if (ret == JSMN_ERROR_PART)
        log_die("jsmn_parse: truncated JSON string");

    return tokens;
}

#define TOKEN_STREQ(js, t, s) \
            (strncmp(js+(t)->start, s, (t)->end - (t)->start) == 0 \
                      && strlen(s) == (t)->end - (t)->start)

char * token_tostr(char *js, jsmntok_t *t)
{
    js[t->end] = '\0';
    return js + t->start;
}

int main(void)
{
    char *js = fetch_json(URL);

    jsmntok_t *tokens = parse_json(js);

    /* The GitHub user API response is a single object. */
    typedef enum { START, KEY, PRINT, SKIP, END } parse_state;
    parse_state state = START;

    size_t object_tokens = 0;

    for (size_t i = 0, j = 1; j > 0; i++, j--)
    {
        jsmntok_t *t = &tokens[i];

        // Should never reach uninitialized tokens
        log_assert(t->start != -1 && t->end != -1);

        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
            j += t->size;

        switch (state)
        {
            case START:
                if (t->type != JSMN_OBJECT)
                    log_die("Invalid response: root element must be an object.");

                state = KEY;
                object_tokens = t->size;

                if (object_tokens % 2 != 0)
                    log_die("Invalid response: object must have even number of children.");

                break;

            case KEY:
                object_tokens--;

                if (t->type != JSMN_STRING)
                    log_die("Invalid response: object keys must be strings.");

                state = SKIP;

                for (size_t i = 0; i < sizeof(KEYS)/sizeof(char *); i++)
                {
                    if (TOKEN_STREQ(js, t, KEYS[i]))
                    {
                        printf("%s: ", KEYS[i]);
                        state = PRINT;
                        break;
                    }
                }

                break;

            case SKIP:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                object_tokens--;
                state = KEY;

                if (object_tokens == 0)
                    state = END;

                break;

            case PRINT:
                if (t->type != JSMN_STRING && t->type != JSMN_PRIMITIVE)
                    log_die("Invalid response: object values must be strings or primitives.");

                char *str = token_tostr(js, t);
                puts(str);

                object_tokens--;
                state = KEY;

                if (object_tokens == 0)
                    state = END;

                break;

            case END:
                // Just consume the tokens
                break;

            default:
                log_die("Invalid state %u", state);
        }
    }

    return 0;
}
