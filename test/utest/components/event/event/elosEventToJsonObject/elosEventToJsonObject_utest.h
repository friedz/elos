// SPDX-License-Identifier: MIT
#ifndef ELOS_EVENTTOJSONOBJECT_UTEST_H
#define ELOS_EVENTTOJSONOBJECT_UTEST_H

#include <cmocka_mocks/mock_jsonc.h>
#include <cmocka_mocks/mock_libc.h>
#include <limits.h>
#include <stdlib.h>

#include "event_utest.h"
#include "safu/mock_safu.h"

TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectEventNullError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectJobjNullError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectClassificationError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectDateError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectHardwareIdError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectMessageCodeError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectPayloadError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSeverityError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSourceAddObjectError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSourceError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectJsonObjectError)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSuccessNullEventSource)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSuccessSeverityOnly)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSuccessValidTimeValues)
TEST_CASE_FUNC_PROTOTYPES(elosTestElosEventToJsonObjectSuccessZeroInitialized)

typedef struct elosTestState {
    json_object *result;
    elosEvent_t *event;
} elosTestState_t;

#endif /* ELOS_EVENTTOJSONOBJECT_UTEST_H */