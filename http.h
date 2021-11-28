#ifndef __HTTP__
#define __HTTP__

#define RES_TYPE_INVALID (0 << 1)
#define RES_TYPE_HTML (1 << 1)
#define RES_TYPE_TEXT (2 << 1)
#define RES_TYPE_JSON (3 << 1)

#define HTTP_ERR 0
#define HTTP_OK  1

typedef struct httpResponse {
    char *body;
    unsigned int bodylen;
    unsigned int status_code;
    int content_type;
} httpResponse;

void httpResponseRelease(httpResponse *response);
void httpPrintResponse(httpResponse *response);

httpResponse *httpMakeGetRequest(char *url, char *additional_headers);
httpResponse *curlHttpGet(char *url);

#endif
