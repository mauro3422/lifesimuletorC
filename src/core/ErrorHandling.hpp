#ifndef ERROR_HANDLING_HPP
#define ERROR_HANDLING_HPP

#include "raylib.h"
#include <string>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

/**
 * Unified Error Handling System (Phase 30)
 * Standardizes how the simulation reports and reacts to issues.
 */

enum class ErrorSeverity {
    WARNING, // Recoverable, log and continue
    ERROR,   // Significant issue, may lead to degraded state
    FATAL    // Unrecoverable, save state (if possible) and exit
};

class ErrorHandler {
public:
    static void handle(ErrorSeverity severity, const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        int raylibLevel;
        const char* prefix;

        switch (severity) {
            case ErrorSeverity::WARNING:
                raylibLevel = LOG_WARNING;
                prefix = "[WARNING]";
                break;
            case ErrorSeverity::ERROR:
                raylibLevel = LOG_ERROR;
                prefix = "[ERROR]";
                break;
            case ErrorSeverity::FATAL:
                raylibLevel = LOG_FATAL;
                prefix = "[FATAL]";
                break;
            default:
                raylibLevel = LOG_INFO;
                prefix = "[DEBUG]";
                break;
        }

        TraceLog(raylibLevel, "%s %s", prefix, buffer);

        if (severity == ErrorSeverity::FATAL) {
            // Future: Trigger state save/dump here
            // exit(1); // Not using exit(1) immediately to allow raylib to clean up if called from main thread
            // Instead, we can set a global flag or just trap if debugging
#ifdef DEBUG
            // __builtin_trap(); 
#endif
            // For now, follow user's prompt: exit(1)
            exit(1);
        }
    }
};

#endif // ERROR_HANDLING_HPP
