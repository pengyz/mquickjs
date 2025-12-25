/*
 * JS Module to C Structure Generator
 *
 * Copyright (c) 2017-2025 Fabrice Bellard
 * Copyright (c) 2017-2025 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>

#include "cutils.h"
#include "list.h"
#include "mquickjs_build.h"

static unsigned JSW = 4; // override this with -m64

typedef struct {
    char *str;
    int offset;
} AtomDef;

typedef struct {
    AtomDef *tab;
    int count;
    int size;
    int offset;
} AtomList;

typedef struct {
    struct list_head link;
    const JSClassDef *class1;
    int class_idx;
    char *finalizer_name;
    char *class_id;
} ClassDefEntry;

typedef struct {
    AtomList atom_list;
    int cur_offset;
    int sorted_atom_table_offset;
    int global_object_offset;
    struct list_head class_list;
} BuildContext;

static char *cvt_name(char *buf, size_t buf_size, const char *str)
{
    size_t i, len = strlen(str);
    assert(len < buf_size);
    if (len == 0) {
        strcpy(buf, "empty");
    } else {
        strcpy(buf, str);
        for(i = 0; i < len; i++) {
            if (buf[i] == '<' || buf[i] == '>' || buf[i] == '-')
                buf[i] = '_';
        }
    }
    return buf;
}

static BOOL is_ascii_string(const char *buf, size_t len)
{
    size_t i;
    for(i = 0; i < len; i++) {
        if ((uint8_t)buf[i] > 0x7f)
            return FALSE;
    }
    return TRUE;
}

static BOOL is_numeric_string(const char *buf, size_t len)
{
    return (!strcmp(buf, "NaN") ||
            !strcmp(buf, "Infinity") ||
            !strcmp(buf, "-Infinity"));
}

static int find_atom(AtomList *s, const char *str)
{
    int i;
    for(i = 0; i < s->count; i++) {
        if (!strcmp(str, s->tab[i].str))
            return i;
    }
    return -1;
}

static int add_atom(AtomList *s, const char *str)
{
    int i;
    AtomDef *e;
    i = find_atom(s, str);
    if (i >= 0)
        return s->tab[i].offset;
    if ((s->count + 1) > s->size) {
        s->size = max_int(s->count + 1, s->size * 3 / 2);
        s->tab = realloc(s->tab, sizeof(s->tab[0]) * s->size);
    }
    e = &s->tab[s->count++];
    e->str = strdup(str);
    e->offset = s->offset;
    s->offset += 1 + ((strlen(str) + JSW) / JSW);
    return s->count - 1;
}

static int atom_cmp(const void *p1, const void *p2)
{
    const AtomDef *a1 = (const AtomDef *)p1;
    const AtomDef *a2 = (const AtomDef *)p2;
    return strcmp(a1->str, a2->str);
}

/* js_atom_table must be propertly aligned because the property hash
   table uses the low bits of the atom pointer value */
#define ATOM_ALIGN 64

static void dump_atoms(BuildContext *ctx)
{
    AtomList *s = &ctx->atom_list;
    int i, j, k, l, len, len1, is_ascii, is_numeric;
    uint64_t v;
    const char *str;
    AtomDef *sorted_atoms;
    char buf[256];

    sorted_atoms = malloc(sizeof(sorted_atoms[0]) * s->count);
    memcpy(sorted_atoms, s->tab, sizeof(sorted_atoms[0]) * s->count);
    qsort(sorted_atoms, s->count, sizeof(sorted_atoms[0]), atom_cmp);

    printf("  /* atom_table */\n");
    for(i = 0; i < s->count; i++) {
        str = s->tab[i].str;
        len = strlen(str);
        is_ascii = is_ascii_string(str, len);
        is_numeric = is_numeric_string(str, len);
        printf("  (JS_MTAG_STRING << 1) | (1 << JS_MTAG_BITS) | (%d << (JS_MTAG_BITS + 1)) | (%d << (JS_MTAG_BITS + 2)) | (%d << (JS_MTAG_BITS + 3)), /* \"%s\" (offset=%d) */\n",
               is_ascii, is_numeric, len, str, ctx->cur_offset);
        len1 = (len + JSW) / JSW;
        for(j = 0; j < len1; j++) {
            l = min_uint32(JSW, len - j * JSW);
            v = 0;
            for(k = 0; k < l; k++)
                v |= (uint64_t)(uint8_t)str[j * JSW + k] << (k * 8);
            printf("  0x%0*" PRIx64 ",\n", JSW * 2, v);
        }
        assert(ctx->cur_offset == s->tab[i].offset);
        ctx->cur_offset += len1 + 1;
    }
    printf("\n");

    ctx->sorted_atom_table_offset = ctx->cur_offset;

    printf("  /* sorted atom table (offset=%d) */\n", ctx->cur_offset);
    printf("  JS_VALUE_ARRAY_HEADER(%d),\n", s->count);
    for(i = 0; i < s->count; i++) {
        AtomDef *e = &sorted_atoms[i];
        printf("  JS_ROM_VALUE(%d), /* %s */\n",
               e->offset, cvt_name(buf, sizeof(buf), e->str));
    }
    ctx->cur_offset += s->count + 1;
    printf("\n");

    free(sorted_atoms);
}

// Function to free atom list memory
static void free_atom_list(AtomList *s)
{
    for (int i = 0; i < s->count; i++) {
        free(s->tab[i].str);
    }
    free(s->tab);
}

static int usage(const char *name)
{
    fprintf(stderr, "usage: %s {-m32 | -m64} <module_name> <js_file>\n", name);
    fprintf(stderr,
            "    create a ROM file for a user JS module\n"
            "--help       list options\n"
            "-m32         force generation for a 32 bit target\n"
            "-m64         force generation for a 64 bit target\n"
            "<module_name> name of the module to be used in generated struct\n"
            "<js_file>     path to the JavaScript file to convert\n"
            );
    return 1;
}

int main(int argc, char **argv)
{
    int i;
    unsigned jsw;
    BuildContext ss, *s = &ss;
    char *module_name = NULL;
    char *js_file_path = NULL;
    
#if INTPTR_MAX >= INT64_MAX
    jsw = 8;
#else
    jsw = 4;
#endif    
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-m64")) {
            jsw = 8;
        } else if (!strcmp(argv[i], "-m32")) {
            jsw = 4;
        } else if (!strcmp(argv[i], "--help")) {
            return usage(argv[0]);
        } else if (argv[i][0] != '-' && module_name == NULL) {
            module_name = argv[i];
        } else if (argv[i][0] != '-' && js_file_path == NULL) {
            js_file_path = argv[i];
        } else {
            fprintf(stderr, "invalid argument '%s'\n", argv[i]);
            return usage(argv[0]);
        }
    }

    if (!module_name || !js_file_path) {
        fprintf(stderr, "missing module name or JS file path\n");
        return usage(argv[0]);
    }

    JSW = jsw;
    
    memset(s, 0, sizeof(*s));
    init_list_head(&s->class_list);

    // Add predefined atoms that might be used in user JS
    add_atom(&s->atom_list, "null");
    add_atom(&s->atom_list, "undefined");
    add_atom(&s->atom_list, "true");
    add_atom(&s->atom_list, "false");
    add_atom(&s->atom_list, "globalThis");
    add_atom(&s->atom_list, module_name);  // Add module name as an atom

    // Add atoms from the JS file content (simplified - just read the file and extract words)
    FILE *file = fopen(js_file_path, "r");
    if (file) {
        char content[4096];
        size_t bytes_read = fread(content, 1, sizeof(content) - 1, file);
        fclose(file);
        content[bytes_read] = '\0';

        // Basic tokenization to extract identifiers and strings
        char *content_copy = strdup(content);  // Make a copy for strtok since it modifies the string
        char *token = strtok(content_copy, " \t\n\r(){}[];,.!?=+-*/&|^~%<>?:\\\"");
        while (token != NULL) {
            // Check if it looks like an identifier (starts with letter/underscore/dollar)
            if (isalpha(token[0]) || token[0] == '_' || token[0] == '$') {
                add_atom(&s->atom_list, token);
            }
            token = strtok(NULL, " \t\n\r(){}[];,.!?=+-*/&|^~%<>?:\\\"");
        }
        free(content_copy);
    } else {
        fprintf(stderr, "Could not open JS file: %s\n", js_file_path);
        return 1;
    }

    printf("/* this file is automatically generated - do not edit */\n\n");
    printf("#include \"mquickjs_priv.h\"\n\n");
    
    printf("static const uint%u_t __attribute((aligned(%d))) js_%s_table[] = {\n",
           JSW * 8, ATOM_ALIGN, module_name);

    dump_atoms(s);

    // We'll need to set the global object offset to the current position
    s->global_object_offset = s->cur_offset;

    printf("};\n\n");

    // Generate the module definition
    printf("const JSSTDLibraryDef js_%s = {\n", module_name);
    printf("  js_%s_table,\n", module_name);
    printf("  NULL,  // User modules typically don't define new C functions\n");
    printf("  NULL,  // No custom finalizers\n");
    printf("  %d,    // table length\n", s->cur_offset);
    printf("  %d,    // alignment\n", ATOM_ALIGN);
    printf("  %d,    // sorted atoms offset\n", s->sorted_atom_table_offset);
    printf("  %d,    // global object offset\n", s->global_object_offset);
    printf("  0,     // class count\n");
    printf("};\n\n");

    // Free allocated memory
    free_atom_list(&s->atom_list);

    return 0;
}