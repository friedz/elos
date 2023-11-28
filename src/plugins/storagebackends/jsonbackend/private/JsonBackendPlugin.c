// SPDX-License-Identifier: MIT

#define _GNU_SOURCE
#include <ctype.h>
#include <elos/libelosplugin/libelosplugin.h>
#include <fcntl.h>
#include <glob.h>
#include <safu/log.h>
#include <safu/mutex.h>
#include <safu/time.h>
#include <samconf/samconf.h>
#include <stdlib.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include "JsonBackend.h"

static inline char *_upperString(const char *src) {
    size_t i;
    size_t srcLen = strlen(src);
    char *dest = safuAllocMem(NULL, sizeof(char) * (srcLen + 1));
    for (i = 0; i < srcLen; i++) {
        dest[i] = toupper(src[i]);
    }
    dest[srcLen] = '\0';
    return dest;
}

static inline const char *_getStorageFileEnv(const char *backendName) {
    const char *prefix = "ELOS_STORAGE_BACKEND_";
    const char *postfix = "_FILE";
    const size_t prefixLen = strlen(prefix);
    const size_t postfixLen = strlen(postfix);
    const size_t nameLen = strlen(backendName);
    char *envVar = safuAllocMem(NULL, sizeof(char) * (prefixLen + postfixLen + nameLen + 1));
    char *upperName = _upperString(backendName);
    strcpy(envVar, prefix);
    strcpy(envVar + prefixLen, upperName);
    strcpy(envVar + prefixLen + nameLen, postfix);
    const char *ret = safuGetEnvOr(envVar, "");
    free(envVar);
    free(upperName);
    return ret;
}

static inline int _mapStrToFlag(const char *str) {
    int retVal = 0;
    if (strcmp(str, "O_NOATIME") == 0) {
        retVal = O_NOATIME;
    } else if (strcmp(str, "O_DSYNC") == 0) {
        retVal = O_DSYNC;
    } else if (strcmp(str, "O_DIRECT") == 0) {
        retVal = O_DIRECT;
    } else if (strcmp(str, "O_SYNC") == 0) {
        retVal = O_SYNC;
    } else {
        safuLogWarnF("Invalid Open Flag found: %s", str);
    }
    return retVal;
}

static inline int _getFlags(const elosPlugin_t *plugin) {
    int flags = 0;
    const samconfConfig_t *flagArray;

    samconfConfigStatusE_t retVal = samconfConfigGet(plugin->config, "Config/Flags", &flagArray);
    if (retVal == SAMCONF_CONFIG_OK) {
        if (flagArray->type != SAMCONF_CONFIG_VALUE_ARRAY) {
            return 0;
        }

        for (size_t i = 0; i < flagArray->childCount; i++) {
            samconfConfig_t *entry = flagArray->children[i];
            if (entry->type != SAMCONF_CONFIG_VALUE_STRING) {
                continue;
            }
            char *flagStr = entry->value.string;
            safuLogDebugF("Open-Flag %s detected", flagStr);
            flags = flags | _mapStrToFlag(flagStr);
        }
    } else if (retVal == SAMCONF_CONFIG_ERROR) {
        safuLogErr("Failed to read flags due to configuration error.");
    }

    return flags;
}

static inline size_t _initializeCount(elosJsonBackend_t *jsonBackend) {
    size_t prevCounts = 0;
    char *currentDate = safuGetCurrentDateString(jsonBackend->dateFormat);
    if (currentDate == NULL) {
        currentDate = safuAllocMem(NULL, sizeof(char));
        currentDate[0] = '\0';
    }
    bool pathHasCount = false;
    char *pattern = elosCompleteStoragePath(jsonBackend->storageFilePattern, jsonBackend->pathSizeLimit, "%i+",
                                            currentDate, &pathHasCount);
    if (!pathHasCount) {
        return 0;
    }
    glob_t fileVector;
    int retVal = glob(pattern, 0, NULL, &fileVector);
    free(pattern);
    free(currentDate);
    switch (retVal) {
        case 0:
            prevCounts = fileVector.gl_pathc;
            break;
        case GLOB_NOSPACE:
            safuLogWarn("Failed to search for preexisting logs, due to insufficient memory. Defaulting to count 0");
            break;
        case GLOB_ABORTED:
            safuLogWarn("Failed to search for preexisting logs, defaulting to count 0");
            break;
        case GLOB_NOMATCH:
            break;
        default:
            safuLogWarn("glob returned invalid code, defaulting to 0");
            break;
    }
    return prevCounts;
}

static inline int _getPathSizeLimit(elosPlugin_t *plugin) {
    int limit = ELOS_JSON_LOGGING_PATH_LIMIT_DEFAULT;
    samconfConfigStatusE_t retVal = samconfConfigGetInt32(plugin->config, "Config/PathSizeLimit", &limit);
    if (retVal != SAMCONF_CONFIG_OK && retVal != SAMCONF_CONFIG_NOT_FOUND) {
        safuLogWarn("Invalid Value for PathSizeLimit");
    }
    return limit;
}

static inline char *_getDateFormat(elosPlugin_t *plugin) {
    char *format;
    samconfConfigStatusE_t retVal = samconfConfigGetString(plugin->config, "Config/DateFormat", (const char **)&format);
    if (retVal != SAMCONF_CONFIG_OK && retVal != SAMCONF_CONFIG_NOT_FOUND) {
        safuLogWarn("Invalid Value for DateFormat");
    }
    if (format == NULL) {
        format = "%Y%m%d";
    }
    return format;
}

static inline size_t _getFileSize(elosPlugin_t *plugin) {
    int32_t size;
    samconfConfigStatusE_t retVal = samconfConfigGetInt32(plugin->config, "Config/MaxSize", &size);
    if (retVal == SAMCONF_CONFIG_OK) {
        safuLogDebugF("Using filesize limit %i", size);
        return size;
    }
    safuLogDebug("Using default filesize limit ");
    return ELOS_JSON_LOGGER_MAX_FILE_SIZE;
}

static safuResultE_t _pluginLoad(elosPlugin_t *plugin) {
    safuResultE_t result = SAFU_RESULT_FAILED;

    if (plugin == NULL) {
        safuLogErr("Null parameter given");
    } else {
        result = elosJsonBackendNew((elosStorageBackend_t **)&plugin->data);
        if (result != SAFU_RESULT_OK) {
            safuLogErr("elosJsonBackendNew failed");
        }
    }

    elosStorageBackend_t *backend = (elosStorageBackend_t *)plugin->data;
    elosJsonBackend_t *jsonBackend = (elosJsonBackend_t *)backend->backendData;
    jsonBackend->storageFilePattern = (char *)_getStorageFileEnv(plugin->config->key);
    if (strcmp(jsonBackend->storageFilePattern, "") == 0) {
        samconfConfigStatusE_t retVal = samconfConfigGetString(plugin->config, "Config/StoragePath",
                                                               (const char **)&jsonBackend->storageFilePattern);
        if ((retVal != SAMCONF_CONFIG_OK) || (strcmp(jsonBackend->storageFilePattern, "") == 0)) {
            jsonBackend->storageFilePattern = STORAGE_LOCATION;
        }
    }

    jsonBackend->additionalFlags = _getFlags(plugin);
    jsonBackend->dateFormat = _getDateFormat(plugin);
    jsonBackend->maxFileSize = _getFileSize(plugin);
    jsonBackend->pathSizeLimit = _getPathSizeLimit(plugin);
    jsonBackend->count = _initializeCount(jsonBackend);

    return result;
}

static safuResultE_t _pluginStart(elosPlugin_t *plugin) {
    safuResultE_t result = SAFU_RESULT_OK;

    if (plugin == NULL) {
        safuLogErr("Null parameter given");
        result = SAFU_RESULT_FAILED;
    } else {
        result = elosJsonBackendStart(plugin->data);
        if (result != SAFU_RESULT_OK) {
            safuLogErr("elosJsonBackendStart failed");
        }

        result = elosPluginReportAsStarted(plugin);
        if (result == SAFU_RESULT_FAILED) {
            safuLogErr("elosPluginReportAsStarted failed");
        } else {
            result = elosPluginStopTriggerWait(plugin);
            if (result == SAFU_RESULT_FAILED) {
                safuLogErr("elosPluginStopTriggerWait failed");
            }
        }
    }

    return result;
}

static safuResultE_t _pluginStop(elosPlugin_t *plugin) {
    safuResultE_t result = SAFU_RESULT_OK;

    if (plugin == NULL) {
        safuLogErr("Null parameter given");
        result = SAFU_RESULT_FAILED;
    } else {
        safuLogDebugF("Stopping Plugin '%s'", plugin->config->key);

        result = elosJsonBackendShutdown(plugin->data);
        plugin->data = NULL;
        if (result != SAFU_RESULT_OK) {
            safuLogWarn("elosJsonBackendShutdown failed (likely resulting in a memory leak)");
        }

        result = elosPluginStopTriggerWrite(plugin);
        if (result == SAFU_RESULT_FAILED) {
            safuLogErr("elosPluginStopTriggerWrite failed");
        }
    }

    return result;
}

static safuResultE_t _pluginUnload(elosPlugin_t *plugin) {
    safuResultE_t result = SAFU_RESULT_FAILED;

    if (plugin == NULL) {
        safuLogErr("Null parameter given");
    } else {
        safuLogDebugF("Unloading Plugin '%s'", plugin->config->key);
        result = SAFU_RESULT_OK;
    }

    return result;
}

elosPluginConfig_t elosPluginConfig = {
    .type = PLUGIN_TYPE_STORAGEBACKEND,
    .load = _pluginLoad,
    .unload = _pluginUnload,
    .start = _pluginStart,
    .stop = _pluginStop,
};
