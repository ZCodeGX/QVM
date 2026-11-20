#include <stdlib.h>
#include <string.h>
#include "qvm/string.h"

qvm_string *qvm_string_create(const char *data, uint32_t len, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!data && len > 0) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }

    qvm_string *s = (qvm_string *)malloc(sizeof *s);
    if (!s) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }

    char *buf = NULL;
    if (len > 0) {
        buf = (char *)malloc(len + 1);
        if (!buf) {
            free(s);
            if (err) *err = QVM_ERR_OUT_OF_MEMORY;
            return NULL;
        }
        memcpy(buf, data, len);
        buf[len] = '\0';
    }

    s->data      = buf;
    s->len       = len;
    s->ref_count = 1;
    return s;
}

qvm_string *qvm_string_create_from_cstr(const char *cstr, qvm_error *err) {
    if (!cstr) return qvm_string_create(NULL, 0, err);
    return qvm_string_create(cstr, (uint32_t)strlen(cstr), err);
}

qvm_string *qvm_string_retain(qvm_string *s) {
    if (s) s->ref_count++;
    return s;
}

void qvm_string_release(qvm_string *s) {
    if (!s) return;
    if (s->ref_count > 1) {
        s->ref_count--;
        return;
    }
    free((void *)s->data);
    free(s);
}

qvm_string *qvm_string_concat(const qvm_string *a, const qvm_string *b, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!a || !b) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    uint32_t new_len = a->len + b->len;
    char *buf = (char *)malloc(new_len + 1);
    if (!buf) {
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    if (a->len > 0) memcpy(buf, a->data, a->len);
    if (b->len > 0) memcpy(buf + a->len, b->data, b->len);
    buf[new_len] = '\0';

    qvm_string *s = (qvm_string *)malloc(sizeof *s);
    if (!s) {
        free(buf);
        if (err) *err = QVM_ERR_OUT_OF_MEMORY;
        return NULL;
    }
    s->data      = buf;
    s->len       = new_len;
    s->ref_count = 1;
    return s;
}

qvm_string *qvm_string_slice(const qvm_string *s, uint32_t start, uint32_t end, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!s) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    if (end > s->len) end = s->len;
    if (start > end) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return NULL;
    }
    return qvm_string_create(s->data + start, end - start, err);
}

uint32_t qvm_string_len(const qvm_string *s) {
    return s ? s->len : 0;
}

int32_t qvm_string_char_at(const qvm_string *s, uint32_t index, qvm_error *err) {
    if (err) *err = QVM_OK;
    if (!s) {
        if (err) *err = QVM_ERR_INVALID_ARGUMENT;
        return -1;
    }
    if (index >= s->len) {
        if (err) *err = QVM_ERR_OUT_OF_BOUNDS;
        return -1;
    }
    return (int32_t)(unsigned char)s->data[index];
}

int qvm_string_compare(const qvm_string *a, const qvm_string *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    uint32_t min_len = a->len < b->len ? a->len : b->len;
    if (min_len > 0) {
        int cmp = memcmp(a->data, b->data, min_len);
        if (cmp != 0) return cmp;
    }
    if (a->len < b->len) return -1;
    if (a->len > b->len) return 1;
    return 0;
}

int qvm_string_equals(const qvm_string *a, const qvm_string *b) {
    return qvm_string_compare(a, b) == 0;
}
