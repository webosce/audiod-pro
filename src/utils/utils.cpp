// Copyright (c) 2012-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <cstdlib>
#include <cstring>

#include "utils.h"
#include "messageUtils.h"
#include "log.h"
#include "main.h"

static GHookList *sInitList         = NULL;
static GHookList *sModuleStartList  = NULL;
static GHookList *sControlStartList = NULL;
static GHookList *sServiceStartList = NULL;
static GHookList *sCancelSubscriptionCallbackList = NULL;

static GMainLoop *sCurrentLoop = NULL;
static LSHandle *sCurrentHandle =NULL;

static void
hookInit(gpointer func)
{
    InitFunction function = (InitFunction) func;
    int ret = function ();
    if (ret != 0)
        g_error ("%s: Could not run init function %p", __FUNCTION__, func);
}

static void
hookStart(gpointer func)
{
    StartFunction function = (StartFunction) func;
    int ret = function (sCurrentLoop, sCurrentHandle);
    if (ret != 0)
        g_error ("%s: Could not run start function %p", __FUNCTION__, func);
}

guint64 getCurrentTimeInMs ()
{
    struct timespec now;
    ::clock_gettime(CLOCK_MONOTONIC, &now);
    return guint64(now.tv_sec) * 1000ULL + guint64(now.tv_nsec) / 1000000ULL;
}

void registerInitFunction (InitFunction function)
{
    if (NULL == sInitList)
    {
       sInitList = (GHookList*) malloc (sizeof (GHookList));
       g_hook_list_init(sInitList, sizeof (GHook));
    }

    GHook *hook = g_hook_alloc (sInitList);
    hook->func = (gpointer) hookInit;
    hook->data = (gpointer) function;

    g_hook_append(sInitList, hook);
}

void registerModuleFunction (StartFunction function)
{

    if (NULL == sModuleStartList)
    {
       sModuleStartList = (GHookList*) malloc (sizeof (GHookList));
       g_hook_list_init(sModuleStartList, sizeof (GHook));
    }

    GHook *hook = g_hook_alloc (sModuleStartList);
    hook->func = (gpointer) hookStart;
    hook->data = (gpointer) function;

    g_hook_append(sModuleStartList, hook);
}

void registerControlFunction (StartFunction function)
{

    if (NULL == sControlStartList)
    {
       sControlStartList = (GHookList*) malloc (sizeof (GHookList));
       g_hook_list_init(sControlStartList, sizeof (GHook));
    }

    GHook *hook = g_hook_alloc (sControlStartList);
    hook->func = (gpointer) hookStart;
    hook->data = (gpointer) function;


    g_hook_append(sControlStartList, hook);
}

void registerServiceFunction (StartFunction function)
{

    if (NULL == sServiceStartList)
    {
       sServiceStartList = (GHookList*) malloc (sizeof (GHookList));
       g_hook_list_init(sServiceStartList, sizeof (GHook));
    }

    GHook *hook = g_hook_alloc (sServiceStartList);
    hook->func = (gpointer) hookStart;
    hook->data = (gpointer) function;

    g_hook_append(sServiceStartList, hook);
}

static LSMessage * sCancelSubscription_LSMessage = NULL;
static LSMessageJsonParser * sCancelSubscription_MessageParser = NULL;

static void
hookCancelSubscription(gpointer func)
{
    CancelSubscriptionCallback function = (CancelSubscriptionCallback) func;
    if (VERIFY(sCancelSubscription_LSMessage && sCancelSubscription_MessageParser))
        function (sCancelSubscription_LSMessage, *sCancelSubscription_MessageParser);
}

void registerCancelSubscriptionCallback (CancelSubscriptionCallback function)
{

    if (NULL == sCancelSubscriptionCallbackList)
    {
        sCancelSubscriptionCallbackList = (GHookList*) malloc (sizeof (GHookList));
       g_hook_list_init(sCancelSubscriptionCallbackList, sizeof (GHook));
    }

    GHook *hook = g_hook_alloc (sCancelSubscriptionCallbackList);
    hook->func = (gpointer) hookCancelSubscription;
    hook->data = (gpointer) function;

    g_hook_append(sCancelSubscriptionCallbackList, hook);
}

static bool
_cancelSubscription (LSHandle *lshandle, LSMessage *message, void *ctx)
{
    LSMessageJsonParser    msg(message, SCHEMA_ANY);
    if (!msg.parse(__FUNCTION__))
        return true;

    LogIndent    indentLogs("| ");

    sCancelSubscription_MessageParser = &msg;
    sCancelSubscription_LSMessage = message;

    if (sCancelSubscriptionCallbackList)
        g_hook_list_invoke (sCancelSubscriptionCallbackList, FALSE);

    sCancelSubscription_MessageParser = NULL;
    sCancelSubscription_LSMessage = NULL;

    return true;
}

void oneFreeForAll()
{
    g_hook_list_clear(sInitList);
    free(sInitList);
    g_hook_list_clear(sServiceStartList);
    free(sServiceStartList);
    g_hook_list_clear(sModuleStartList);
    free(sModuleStartList);
    g_hook_list_clear(sControlStartList);
    free(sControlStartList);
    g_hook_list_clear(sCancelSubscriptionCallbackList);
    free(sCancelSubscriptionCallbackList);
}

void registerCancelSubscription(LSHandle *handle) {
    bool result;
    CLSError lserror;

    result = LSSubscriptionSetCancelFunction (handle,
                                              _cancelSubscription,
                                              NULL, &lserror);
    if (!result)
        lserror.Print(__FUNCTION__, __LINE__);
}

void oneInitForAll(GMainLoop *loop, LSHandle *handle)
{
    sCurrentLoop   = loop;
    sCurrentHandle = handle;

    g_debug ("%s: calling all init functions", __FUNCTION__);
    if (NULL != sInitList)
        g_hook_list_invoke (sInitList, FALSE);

    g_debug ("%s: calling all start module functions", __FUNCTION__);
    if (NULL != sModuleStartList)
        g_hook_list_invoke (sModuleStartList, FALSE);

    g_debug ("%s: calling all start control functions", __FUNCTION__);
    if (NULL != sControlStartList)
        g_hook_list_invoke (sControlStartList, FALSE);

    g_debug ("%s: calling all start service functions", __FUNCTION__);
    if (NULL != sServiceStartList)
        g_hook_list_invoke (sServiceStartList, FALSE);


    registerCancelSubscription(GetPalmService());
    sCurrentLoop   = NULL;
    sCurrentHandle = NULL;

    g_debug("oneInitForAll: complete!");
}

bool ServiceRegisterCategory(const char *category, LSMethod *methods,
        LSSignal *signal, void *category_user_data)
{
    bool result;
    CLSError lserror;
    g_message("%s: Registering Service for '%s' category", __FUNCTION__, category);
    result = LSRegisterCategory (GetPalmService(), category,
            methods, signal, NULL, &lserror);
    if (!result)
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return false;
    }

    if (!LSCategorySetData(GetPalmService(), category, category_user_data, &lserror))
    {
        lserror.Print(__FUNCTION__, __LINE__);
        return false;
    }

    return result;
}

