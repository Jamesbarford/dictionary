#ifndef __HTML_GREP_H__
#define __HTML_GREP_H__

#include "cstr.h"
#include "list.h"

list *htmlGetMatches(cstr *html, char *classname);
cstr *htmlConcatList(list *l);

#endif
