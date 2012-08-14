#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include "jsmn.h"

#include "json.h"
#include "log.h"
#include "buf.h"

char URL[] = "https://api.twitter.com/1/trends/1.json";

int main(void)
{
    char *js = json_fetch(URL);

    jsmntok_t *tokens = json_tokenise(js);

    /* The Twitter trends API response is in this format:
     *
     * [
     *   {
     *      ...,
     *      "trends": [
     *          {
     *              ...,
     *              "name": "TwitterChillers",
     *          },
     *          ...,
     *      ],
     *      ...,
     *   }
     * ]
     *
     */
    typedef enum {
        START,
        WRAPPER, OBJECT,
        TRENDS, ARRAY,
        TREND, NAME,
        SKIP,
        STOP
    } parse_state;
    parse_state state = START, next = STOP;

    size_t object_tokens = 0;
    size_t skip_tokens = 0;
    size_t trends = 0;
    size_t trend_tokens = 0;

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
                if (t->type != JSMN_ARRAY)
                    log_die("Invalid response: root element must be array.");
                if (t->size != 1)
                    log_die("Invalid response: array must have one element.");

                state = WRAPPER;
                break;

            case WRAPPER:
                if (t->type != JSMN_OBJECT)
                    log_die("Invalid response: wrapper must be an object.");

                state = OBJECT;
                object_tokens = t->size;
                break;

            case OBJECT:
                object_tokens--;

                // Keys are odd-numbered tokens within the object
                if (object_tokens % 2 == 1)
                {
                    if (t->type == JSMN_STRING && json_token_streq(js, t, "trends"))
                        state = TRENDS;
                }
                else if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
                {
                    state = SKIP;
                    next = OBJECT;
                    skip_tokens = t->size;
                }

                // Last object value
                if (object_tokens == 0)
                    state = STOP;

                break;

            case SKIP:
                skip_tokens--;

                if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
                    skip_tokens += t->size;

                if (skip_tokens == 0)
                    state = next;

                break;

            case TRENDS:
                if (t->type != JSMN_ARRAY)
                    log_die("Unknown trends value: expected array.");

                trends = t->size;
                state = ARRAY;
                next = ARRAY;

                // No trends found
                if (trends == 0)
                    state = STOP;

                break;

            case ARRAY:
                trends--;

                trend_tokens = t->size;
                state = TREND;

                // Empty trend object
                if (trend_tokens == 0)
                    state = STOP;

                // Last trend object
                if (trends == 0)
                    next = STOP;

                break;

            case TREND:
            case NAME:
                trend_tokens--;

                // Keys are odd-numbered tokens within the object
                if (trend_tokens % 2 == 1)
                {
                    if (t->type == JSMN_STRING && json_token_streq(js, t, "name"))
                        state = NAME;
                }
                // Only care about values in the NAME state
                else if (state == NAME)
                {
                    if (t->type != JSMN_STRING)
                        log_die("Invalid trend name.");

                    char *str = json_token_tostr(js, t);
                    puts(str);

                    state = TREND;
                }
                else if (t->type == JSMN_ARRAY || t->type == JSMN_OBJECT)
                {
                    state = SKIP;
                    next = TREND;
                    skip_tokens = t->size;
                }

                // Last object value
                if (trend_tokens == 0)
                    state = next;

                break;

            case STOP:
                // Just consume the tokens
                break;

            default:
                log_die("Invalid state %u", state);
        }
    }

    return 0;
}
