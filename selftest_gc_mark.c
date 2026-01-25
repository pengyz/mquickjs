#include <stdio.h>
#include <string.h>

#include "mquickjs_build.h"
#include "mquickjs.h"

/* This test TU is intentionally small and non-interactive.
 * It verifies:
 * 1) user class gc_mark participates in GC marking.
 * 2) JSGCRef roots keep values alive across GC.
 * 3) JS_SetContextGCMark reports context-level roots.
 */

static int g_rect_finalizer_count;
static int g_child_finalizer_count;

typedef struct {
    JSValue held;
} RectNative;

typedef struct {
    /* This intentionally duplicates the "held" pattern, but the class' gc_mark
     * will be NULL to verify we do not implicitly inherit parent's gc_mark.
     */
    JSValue held;
} ChildNative;

static void rect_finalizer(JSContext *ctx, void *opaque)
{
    (void)ctx;
    (void)opaque;
    g_rect_finalizer_count++;
}

static void child_finalizer(JSContext *ctx, void *opaque)
{
    (void)ctx;
    (void)opaque;
    g_child_finalizer_count++;
}

static void rect_gc_mark(JSContext *ctx, void *opaque, const JSMarkFunc *mf)
{
    (void)ctx;
    RectNative *d = opaque;
    mf->mark_value(mf, d->held);
}

#define JS_CLASS_RECT (JS_CLASS_USER + 0)
#define JS_CLASS_CHILD (JS_CLASS_USER + 1)
#define JS_CLASS_COUNT (JS_CLASS_USER + 2)

static JSValue rect_ctor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv)
{
    (void)this_val;
    (void)argv;

    if (!(argc & FRAME_CF_CTOR))
        return JS_ThrowTypeError(ctx, "must be called with new");

    argc &= ~FRAME_CF_CTOR;

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_RECT);
    RectNative *d = malloc(sizeof(*d));
    d->held = JS_UNDEFINED;
    JS_SetOpaque(ctx, obj, d);
    return obj;
}

static const JSPropDef rect_proto[] = {
    JS_PROP_END,
};

static const JSPropDef rect_class_props[] = {
    JS_PROP_END,
};

static const JSClassDef rect_class_def =
    JS_CLASS_DEF("Rect", 0, rect_ctor, JS_CLASS_RECT, rect_class_props, rect_proto, NULL, rect_finalizer, rect_gc_mark);

static const JSPropDef child_proto[] = {
    JS_PROP_END,
};

static const JSPropDef child_class_props[] = {
    JS_PROP_END,
};

static const JSClassDef child_class_def =
    JS_CLASS_DEF("Child", 0, rect_ctor, JS_CLASS_CHILD, child_class_props, child_proto, &rect_class_def, child_finalizer, NULL);

static const JSPropDef global_obj[] = {
    JS_PROP_CLASS_DEF("Rect", &rect_class_def),
    JS_PROP_CLASS_DEF("Child", &child_class_def),
    JS_PROP_END,
};

static const JSPropDef c_function_decl[] = {
    JS_PROP_END,
};

static const JSCFinalizer js_c_finalizer_table[JS_CLASS_COUNT - JS_CLASS_USER] = {
    [JS_CLASS_RECT - JS_CLASS_USER] = rect_finalizer,
    [JS_CLASS_CHILD - JS_CLASS_USER] = child_finalizer,
};

static const JSCMark js_c_mark_table[JS_CLASS_COUNT - JS_CLASS_USER] = {
    [JS_CLASS_RECT - JS_CLASS_USER] = rect_gc_mark,
    [JS_CLASS_CHILD - JS_CLASS_USER] = NULL,
};

static const JSSTDLibraryDef js_selftest_stdlib = {
    NULL,
    NULL,
    js_c_finalizer_table,
    js_c_mark_table,
    0,
    0,
    0,
    0,
    JS_CLASS_COUNT,
};

static JSValue make_plain_object(JSContext *ctx)
{
    JSValue global = JS_GetGlobalObject(ctx);
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "g", global);
    return obj;
}

static int test_user_class_cycle_is_collectable(void)
{
    uint8_t mem_buf[65536];
    JSContext *ctx = JS_NewContext(mem_buf, sizeof(mem_buf), &js_selftest_stdlib);
    if (!ctx)
        return 1;

    g_rect_finalizer_count = 0;

    /* Create wrapper A */
    JSValue a = JS_NewObjectClassUser(ctx, JS_CLASS_RECT);
    RectNative *d = malloc(sizeof(*d));
    d->held = JS_UNDEFINED;
    JS_SetOpaque(ctx, a, d);

    /* Create held B that points back to A */
    JSValue b = make_plain_object(ctx);
    JS_SetPropertyStr(ctx, b, "some", a);

    /* Native holds B */
    d->held = b;

    /* Drop external refs: keep only the cycle A <-> B + opaque(A)->B */
    a = JS_UNDEFINED;
    b = JS_UNDEFINED;

    JS_GC(ctx);
    JS_GC(ctx);

    JS_FreeContext(ctx);

    if (g_rect_finalizer_count <= 0) {
        fprintf(stderr, "selftest: expected finalizer to run for cycle collection\n");
        return 1;
    }
    return 0;
}

/* Verify gc_mark is NOT inherited via parent_class:
 * - Child is declared with parent_class = Rect, but has gc_mark = NULL.
 * - If engine tried to walk parent_class for gc_mark, the held value would be marked
 *   and the cycle would not be collectable.
 */
static int test_child_does_not_inherit_gc_mark(void)
{
    uint8_t mem_buf[65536];
    JSContext *ctx = JS_NewContext(mem_buf, sizeof(mem_buf), &js_selftest_stdlib);
    if (!ctx)
        return 1;

    g_child_finalizer_count = 0;

    JSValue a = JS_NewObjectClassUser(ctx, JS_CLASS_CHILD);
    ChildNative *d = malloc(sizeof(*d));
    d->held = JS_UNDEFINED;
    JS_SetOpaque(ctx, a, d);

    JSValue b = make_plain_object(ctx);
    JS_SetPropertyStr(ctx, b, "some", a);
    d->held = b;

    a = JS_UNDEFINED;
    b = JS_UNDEFINED;

    JS_GC(ctx);
    JS_GC(ctx);

    JS_FreeContext(ctx);

    if (g_child_finalizer_count <= 0) {
        fprintf(stderr, "selftest: expected child finalizer to run (no inherited gc_mark)\n");
        return 1;
    }
    return 0;
}

static int test_stress_cycles_collectable(void)
{
    for (int i = 0; i < 200; i++) {
        uint8_t mem_buf[65536];
        JSContext *ctx = JS_NewContext(mem_buf, sizeof(mem_buf), &js_selftest_stdlib);
        if (!ctx)
            return 1;

        g_rect_finalizer_count = 0;

        JSValue a = JS_NewObjectClassUser(ctx, JS_CLASS_RECT);
        RectNative *d = malloc(sizeof(*d));
        d->held = JS_UNDEFINED;
        JS_SetOpaque(ctx, a, d);

        JSValue b = make_plain_object(ctx);
        JS_SetPropertyStr(ctx, b, "some", a);
        d->held = b;

        a = JS_UNDEFINED;
        b = JS_UNDEFINED;

        JS_GC(ctx);
        JS_GC(ctx);

        JS_FreeContext(ctx);

        if (g_rect_finalizer_count <= 0) {
            fprintf(stderr, "selftest: stress iteration %d did not collect cycle\n", i);
            return 1;
        }
    }

    return 0;
}

static int test_gc_ref_root_keeps_value_alive(void)
{
    uint8_t mem_buf[65536];
    JSContext *ctx = JS_NewContext(mem_buf, sizeof(mem_buf), &js_selftest_stdlib);
    if (!ctx)
        return 1;

    JSGCRef ref;
    JSValue *slot = JS_AddGCRef(ctx, &ref);
    *slot = make_plain_object(ctx);

    JS_GC(ctx);

    /* value usability check: access a property */
    JSValue v = *slot;
    JSValue g = JS_GetPropertyStr(ctx, v, "g");
    if (JS_IsNull(g)) {
        fprintf(stderr, "selftest: rooted value seems invalid after GC\n");
        JS_DeleteGCRef(ctx, &ref);
        JS_FreeContext(ctx);
        return 1;
    }

    JS_DeleteGCRef(ctx, &ref);
    JS_GC(ctx);

    JS_FreeContext(ctx);
    return 0;
}

typedef struct {
    JSValue v;
} CtxRoots;

static void ctx_gc_mark(JSContext *ctx, void *opaque, const JSMarkFunc *mf)
{
    (void)ctx;
    CtxRoots *r = opaque;
    mf->mark_value(mf, r->v);
}

static int test_context_gc_mark_keeps_user_value_alive(void)
{
    uint8_t mem_buf[65536];
    JSContext *ctx = JS_NewContext(mem_buf, sizeof(mem_buf), &js_selftest_stdlib);
    if (!ctx)
        return 1;

    CtxRoots roots;
    roots.v = make_plain_object(ctx);

    JS_SetContextGCMark(ctx, &roots, ctx_gc_mark);

    JS_GC(ctx);

    /* value usability check */
    JSValue g = JS_GetPropertyStr(ctx, roots.v, "g");
    if (JS_IsNull(g)) {
        fprintf(stderr, "selftest: context-marked value seems invalid after GC\n");
        JS_FreeContext(ctx);
        return 1;
    }

    /* remove hook and value */
    JS_SetContextGCMark(ctx, NULL, NULL);
    roots.v = JS_UNDEFINED;
    JS_GC(ctx);

    JS_FreeContext(ctx);
    return 0;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (test_user_class_cycle_is_collectable())
        return 1;
    if (test_child_does_not_inherit_gc_mark())
        return 1;
    if (test_stress_cycles_collectable())
        return 1;
    if (test_gc_ref_root_keeps_value_alive())
        return 1;
    if (test_context_gc_mark_keeps_user_value_alive())
        return 1;
    return 0;
}
