#ifndef QVM_H
#define QVM_H

#include "qvm/config.h"
#include "qvm/context.h"
#include "qvm/error.h"
#include "qvm/lifecycle.h"
#include "qvm/limits.h"
#include "qvm/types.h"
#include "qvm/value.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *qvm_version(void);

#ifdef __cplusplus
}
#endif

#endif /* QVM_H */