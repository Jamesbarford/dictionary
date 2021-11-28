#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "panic.h"

httpResponse *_httpCreateResponse() {
    httpResponse *res;

    if ((res = malloc(sizeof(httpResponse))) == NULL)
        return NULL;

    res->body = NULL;
    res->bodylen = 0;
    /* Start in error state */
    res->status_code = 404;

    return res;
}

void httpResponseRelease(httpResponse *response) {
    if (response) {
        free(response->body);
        free(response);
    }
}

static int _httpGetContentType(char *type) {
    if (strncmp(type, "application/json", 16) == 0)
        return RES_TYPE_JSON;
    if (strncmp(type, "text/html", 9) == 0)
        return RES_TYPE_HTML;
    if (strncmp(type, "text", 4) == 0)
        return RES_TYPE_TEXT;

    return RES_TYPE_INVALID;
}

void httpPrintResponse(httpResponse *response) {
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
        response->status_code, response->bodylen, content_type, response->body);
}

static size_t curlRead(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    httpResponse *resp = (httpResponse *)userp;

    char *ptr = realloc(resp->body, resp->bodylen + realsize + 1);
    if (ptr == NULL) {
        warning("CURL error: not enough memory\n");
        return 0;
    }

    resp->body = ptr;
    memcpy(&(resp->body[resp->bodylen]), contents, realsize);
    resp->bodylen += realsize;
    resp->body[resp->bodylen] = 0;

    return realsize;
}

httpResponse *curlHttpGet(char *url) {
    CURL *curl;
    CURLcode res;
    httpResponse *response;
    char *contenttype = NULL;

    if ((response = _httpCreateResponse()) == NULL)
        return NULL;

    response->body = malloc(1);
    response->bodylen = 0;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlRead);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            warning("Failed to make request: %s\n", curl_easy_strerror(res));
            return response;
        } else {
            res = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contenttype);
            response->status_code = 200;
            response->content_type = _httpGetContentType(contenttype);
            curl_easy_cleanup(curl);
            return response;
        }
    }

    return response;
}
