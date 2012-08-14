#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "jsmn.h"

#include "log.h"
#include "buf.h"

char URL[] = "https://api.twitter.com/1/trends/1.json";

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

    enum { START, TRENDS, ARRAY, OBJECT, NAME, END } state = START, next = END;
    size_t trends = 0;
    size_t trend_tokens = 0;

    for (size_t i = 0, j = tokens[i].size; j > 0; i++, j--)
    {
        jsmntok_t *t = &tokens[i];

        // Should never reach uninitialized tokens
        log_assert(t->start != -1 && t->end != -1);

        if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
            j += t->size;

        switch (state)
        {
            case START:
                if (t->type == JSMN_STRING && TOKEN_STREQ(js, t, "trends"))
                    state = TRENDS;
                break;

            case TRENDS:
                if (t->type != JSMN_ARRAY)
                    log_die("Unknown trends value: expected array.");

                trends = t->size;
                state = ARRAY;
                next = ARRAY;

                // No trends found
                if (trends == 0)
                    state = END;

                break;

            case ARRAY:
                trends--;

                trend_tokens = t->size;
                state = OBJECT;

                // Empty trend object
                if (trend_tokens == 0)
                    state = END;

                // Last trend object
                if (trends == 0)
                    next = END;

                break;

            case OBJECT:
            case NAME:
                trend_tokens--;

                // Keys are odd-numbered tokens within the object
                if (trend_tokens % 2 == 1)
                {
                    if (t->type == JSMN_STRING && TOKEN_STREQ(js, t, "name"))
                        state = NAME;
                }
                // Only care about values in the NAME state
                else if (state == NAME)
                {
                    if (t->type != JSMN_STRING)
                        log_die("Invalid trend name.");

                    char *str = token_tostr(js, t);
                    puts(str);

                    state = OBJECT;
                }

                // Last object value
                if (trend_tokens == 0)
                    state = next;

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
