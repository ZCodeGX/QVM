#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "qvm/qvm.h"
#include "qvm/string.h"

static void test_create_destroy(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("hello", &err);
    assert(s != NULL && err == QVM_OK);
    assert(qvm_string_len(s) == 5);
    assert(memcmp(s->data, "hello", 5) == 0);
    qvm_string_release(s);
}

static void test_empty_string(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("", &err);
    assert(s != NULL);
    assert(qvm_string_len(s) == 0);
    qvm_string_release(s);

    qvm_string *n = qvm_string_create_from_cstr(NULL, &err);
    assert(n != NULL);
    assert(qvm_string_len(n) == 0);
    qvm_string_release(n);
}

static void test_concat(void) {
    qvm_error err;
    qvm_string *a = qvm_string_create_from_cstr("hello", &err);
    qvm_string *b = qvm_string_create_from_cstr(" world", &err);
    qvm_string *c = qvm_string_concat(a, b, &err);
    assert(c != NULL);
    assert(qvm_string_len(c) == 11);
    assert(memcmp(c->data, "hello world", 11) == 0);
    qvm_string_release(a);
    qvm_string_release(b);
    qvm_string_release(c);
}

static void test_slice(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("hello world", &err);
    qvm_string *sub = qvm_string_slice(s, 6, 11, &err);
    assert(sub != NULL);
    assert(qvm_string_len(sub) == 5);
    assert(memcmp(sub->data, "world", 5) == 0);
    qvm_string_release(s);
    qvm_string_release(sub);
}

static void test_slice_invalid(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("hello", &err);
    qvm_string *sub = qvm_string_slice(s, 3, 2, &err);
    assert(sub == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    qvm_string_release(s);
}

static void test_char_at(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("abc", &err);
    assert(qvm_string_char_at(s, 0, &err) == 'a');
    assert(qvm_string_char_at(s, 1, &err) == 'b');
    assert(qvm_string_char_at(s, 2, &err) == 'c');
    assert(qvm_string_char_at(s, 3, &err) == -1);
    assert(err == QVM_ERR_OUT_OF_BOUNDS);
    qvm_string_release(s);
}

static void test_compare(void) {
    qvm_error err;
    qvm_string *a = qvm_string_create_from_cstr("abc", &err);
    qvm_string *b = qvm_string_create_from_cstr("abd", &err);
    qvm_string *c = qvm_string_create_from_cstr("abc", &err);

    assert(qvm_string_compare(a, b) < 0);
    assert(qvm_string_compare(b, a) > 0);
    assert(qvm_string_compare(a, c) == 0);
    assert(qvm_string_equals(a, c) == 1);
    assert(qvm_string_equals(a, b) == 0);

    qvm_string_release(a);
    qvm_string_release(b);
    qvm_string_release(c);
}

static void test_ref_count(void) {
    qvm_error err;
    qvm_string *s = qvm_string_create_from_cstr("test", &err);
    assert(s->ref_count == 1);
    qvm_string_retain(s);
    assert(s->ref_count == 2);
    qvm_string_release(s);
    assert(s->ref_count == 1);
    qvm_string_release(s);
}

static void test_null_args(void) {
    qvm_error err;
    assert(qvm_string_concat(NULL, NULL, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_string_slice(NULL, 0, 1, &err) == NULL);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_string_char_at(NULL, 0, &err) == -1);
    assert(err == QVM_ERR_INVALID_ARGUMENT);
    assert(qvm_string_len(NULL) == 0);
    qvm_string_release(NULL);
}

int main(void) {
    test_create_destroy();
    test_empty_string();
    test_concat();
    test_slice();
    test_slice_invalid();
    test_char_at();
    test_compare();
    test_ref_count();
    test_null_args();
    printf("test_string: OK\n");
    return 0;
}
