#include <sys/stat.h>

#include <ctype.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlstring.h>
#include <libxml/xpath.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cstr.h"
#include "htmlgrep.h"
#include "list.h"

list *
parse_html(cstr *html, char *classname)
{
    list *l = NULL;

    htmlDocPtr doc = htmlReadMemory((char *)html, cstrlen(html), "noname", NULL,
            HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (!doc) {
        fprintf(stderr, "Failed to parse HTML.\n");
        return NULL;
    }

    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (!context) {
        fprintf(stderr, "Failed to create XPath context.\n");
        xmlFreeDoc(doc);
        return NULL;
    }

    // XPath expression to find div elements with class 'myClass'
    cstr *xpath = cstrnew();
    xpath = cstrCatPrintf(xpath, "//span[contains(@class, '%s')]", classname);

    xmlXPathObjectPtr result = xmlXPathEvalExpression(xpath, context);
    if (!result) {
        fprintf(stderr, "Failed to evaluate XPath expression.\n");
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);
        return NULL;
    }

    xmlNodeSetPtr nodeset = result->nodesetval;
    if (!nodeset) {
        fprintf(stderr, "No result from XPath expression.\n");
    } else {
        l = listNew();
        listSetFreedata(l, cstrRelease);

        printf("Found %d element(s):\n", nodeset->nodeNr);
        for (int i = 0; i < nodeset->nodeNr; i++) {
            xmlChar *content = xmlNodeGetContent(nodeset->nodeTab[i]);
            int diff = 0;
            while (!isalnum(*content)) {
                content++;
                diff++;
            }
            unsigned int len = strlen((char *)content);
            listAddHead(l, cstrDupRaw((char *)content, len, len + 10));
            content -= diff;
            xmlFree(content);
        }
    }

    xmlXPathFreeObject(result);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    return l;
}

static int
sortstring(void *str1, void *str2)
{
    cstr *pp1 = str1;
    cstr *pp2 = str2;
    unsigned int len1 = cstrlen(pp1);
    unsigned int len2 = cstrlen(pp2);
    unsigned int minlen = len1 > len2 ? len2 : len1;

    return memcmp(pp1, pp2, minlen);
}

list *
htmlGetMatches(cstr *html, char *classname)
{
    list *l = parse_html(html, classname);
    listQSort(l, sortstring);
    return l;
}

cstr *
htmlConcatList(list *l)
{
    if (l->len == 0) {
        return NULL;
    }
    cstr *new = cstrnew();
    lNode *ln = l->root;
    size_t count = l->len;

    do {
        cstr *data = (cstr *)ln->data;
        data = cstrPutChar(data, '\n');
        new = cstrCatLen(new, data, cstrlen(data));
        ln = ln->next;
        count--;
    } while (count);

    return new;
}
