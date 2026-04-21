#include "logger.h"
#include "paths.h"
#include "fs.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static FILE* g_logFile = NULL;

static const char* basename(const char* path) {
    const char* p = strrchr(path, '/');
    if (p) return p + 1;
    p = strrchr(path, '\\');
    if (p) return p + 1;
    return path;
}

static void rotate_logs(void) {
    if (!FS_FileExists(LOG_FILE_CURRENT))
        return;

    char old_path[256];
    char new_path[256];

    for (int i = 4; i >= 1; i--) {
        snprintf(old_path, sizeof(old_path), "%s/Switchelf.log.%d", LOGS_DIR, i);
        snprintf(new_path, sizeof(new_path), "%s/Switchelf.log.%d", LOGS_DIR, i + 1);
        if (FS_FileExists(old_path)) {
            remove(new_path);
            rename(old_path, new_path);
        }
    }

    snprintf(new_path, sizeof(new_path), "%s/Switchelf.log.1", LOGS_DIR);
    remove(new_path);
    rename(LOG_FILE_CURRENT, new_path);
}

void Log_Init(void) {
    FS_RecursiveMakeDir(LOGS_DIR);
    rotate_logs();

    g_logFile = fopen(LOG_FILE_CURRENT, "w");
    if (g_logFile) {
        fprintf(g_logFile, "Log started.\n");
        fflush(g_logFile);
    }
}

void Log_Term(void) {
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}

void Log_Write(const char* level, const char* file, int line, const char* fmt, ...) {
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    char timestamp[32] = "---------- --:--:--";
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    if (tm_info) {
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    const char* filename = basename(file);

    printf("[%s] [%s] [%s:%d] %s\n", timestamp, level, filename, line, buf);

    if (g_logFile) {
        fprintf(g_logFile, "[%s] [%s] [%s:%d] %s\n", timestamp, level, filename, line, buf);
        fflush(g_logFile);
    }
}
