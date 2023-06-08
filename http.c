#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "aostr.h"
#include "http.h"
#include "panic.h"

httpResponse *
_httpCreateResponse()
{
    httpResponse *res;

    if ((res = malloc(sizeof(httpResponse))) == NULL)
        return NULL;

    res->body = NULL;
    res->bodylen = 0;
    /* Start in error state */
    res->status_code = 404;

    return res;
}

void
httpResponseRelease(httpResponse *response)
{
    if (response) {
        aoStrRelease(response->body);
        free(response);
    }
}

static int
_httpGetContentType(char *type)
{
    if (strncmp(type, "application/json", 16) == 0)
        return RES_TYPE_JSON;
    if (strncmp(type, "text/html", 9) == 0)
        return RES_TYPE_HTML;
    if (strncmp(type, "text", 4) == 0)
        return RES_TYPE_TEXT;

    return RES_TYPE_INVALID;
}

void
httpPrintResponse(httpResponse *response)
{
    if (response == NULL) {
        printf("(null)\n");
        return;
    }
    char *content_type;
    content_type = "text";

    switch (response->content_type) {
    case RES_TYPE_HTML:
        content_type = "html";
        break;
    case RES_TYPE_INVALID:
        content_type = "invalid";
        break;
    case RES_TYPE_JSON:
        content_type = "json";
        break;
    case RES_TYPE_TEXT:
        content_type = "text";
        break;
    }

    printf("status code: %d\n"
           "body length: %d\n"
           "content type: %s\n"
           "body: %s\n",
            response->status_code, response->bodylen, content_type,
            response->body->data);
}

static size_t
httpRequestWriteCallback(char *ptr, size_t size, size_t nmemb, void **userdata)
{
    aoStr **str = (aoStr **)userdata;
    size_t rbytes = size * nmemb;
    aoStrCatLen(*str, ptr, rbytes);

    return rbytes;
}

httpResponse *
curlHttpGet(char *url)
{
    CURL *curl;
    CURLcode res;
    httpResponse *httpres;
    char *contenttype = NULL;
    aoStr *respbody = aoStrAlloc(512);

    if ((httpres = _httpCreateResponse()) == NULL)
        return NULL;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                &httpRequestWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respbody);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            warning("Failed to make request: %s\n", curl_easy_strerror(res));
            return httpres;
        } else {
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contenttype);
            httpres->status_code = 200;
            httpres->content_type = _httpGetContentType(contenttype);
            httpres->body = respbody;
            httpres->bodylen = respbody->len;
            curl_easy_cleanup(curl);
            printf("REQ DONE\n");
            return httpres;
        }
    }

    return httpres;
}
