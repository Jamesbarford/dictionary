#include <sys/stat.h>

#include <ctype.h>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/parser.h>
#include <libxml2/libxml/tree.h>
#include <libxml2/libxml/xmlstring.h>
#include <libxml2/libxml/xpath.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aostr.h"
#include "htmlgrep.h"
#include "list.h"

list *
parse_html(aoStr *html, char *classname)
{
    list *l = NULL;
    htmlDocPtr doc = htmlReadMemory(html->data, aoStrLen(html), "noname", NULL,
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
    aoStr *xpath = aoStrAlloc(512);
    aoStrCatPrintf(xpath, "//span[contains(@class, '%s')]", classname);

    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar *)xpath->data,
            context);
    aoStrRelease(xpath);

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
        listSetFreedata(l, (void (*)(void *))aoStrRelease);

        printf("Found %d element(s):\n", nodeset->nodeNr);
        for (int i = 0; i < nodeset->nodeNr; i++) {
            xmlChar *content = xmlNodeGetContent(nodeset->nodeTab[i]);
            int diff = 0;
            while (!isalnum(*content)) {
                content++;
                diff++;
            }
            unsigned int len = strlen((char *)content);
            listAddHead(l, aoStrDupRaw((char *)content, len, len + 10));
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
    aoStr *pp1 = str1;
    aoStr *pp2 = str2;
    unsigned int len1 = aoStrLen(pp1);
    unsigned int len2 = aoStrLen(pp2);
    unsigned int minlen = len1 > len2 ? len2 : len1;

    return memcmp(pp1, pp2, minlen);
}

list *
htmlGetMatches(aoStr *html, char *classname)
{
    list *l = parse_html(html, classname);
    listQSort(l, sortstring);
    return l;
}

aoStr *
htmlConcatList(list *l)
{
    if (l->len == 0) {
        return NULL;
    }
    aoStr *new = aoStrAlloc(512);
    lNode *ln = l->root;
    size_t count = l->len;

    do {
        aoStr *data = (aoStr *)ln->data;
        aoStrPutChar(data, '\n');
        aoStrPutChar(data, '\n');
        aoStrCatLen(new, data->data, data->len);
        ln = ln->next;
        count--;
    } while (count);

    return new;
}
