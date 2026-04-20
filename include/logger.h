#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void Log_Init(void);
void Log_Term(void);
void Log_Write(const char* level, const char* file, int line, const char* fmt, ...);

#ifdef __cplusplus
}
#endif

#define LOG_E(fmt, ...) Log_Write("ERROR", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_W(fmt, ...) Log_Write("WARN ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_I(fmt, ...) Log_Write("INFO ", __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef DEBUG
#define LOG_D(fmt, ...) Log_Write("DEBUG", __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_D(fmt, ...) ((void)0)
#endif

#endif
