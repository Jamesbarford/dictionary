#ifndef __HTML_GREP_H__
#define __HTML_GREP_H__

#include "aostr.h"
#include "list.h"

list *htmlGetMatches(aoStr *html, char *classname);
aoStr *htmlConcatList(list *l);

#endif
