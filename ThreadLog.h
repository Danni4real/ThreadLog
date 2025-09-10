#ifndef THREADLOG_H
#define THREADLOG_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include <mutex>
#include <string>
#include <vector>
#include <sstream>

#define ModuleName "MyModule" // set customized log title

// comment out next line if you do NOT want to save log to file
#define SAVE_LOG_TO_FILE
#define LOG_FILE "/tmp/MyModule.log" // old log will be saved as /tmp/MyModule.log.1 /tmp/MyModule.log.2 /tmp/MyModule.log.3 ...
#define LOG_ROTATE_NUM 5
#define LOG_FILE_SIZE_LIMIT (1*1024*1024) // bytes

#define ERROR_LEVEL 0
#define WARN_LEVEL  1
#define INFO_LEVEL  2
#define DEBUG_LEVEL 3

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __THREADID__ (int)gettid()

#if defined (SAVE_LOG_TO_FILE)
#define LOG(fmt, ...) RotateLog::get_instance().log(fmt, ##__VA_ARGS__)
#else
#define LOG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

class RotateLog {
public:
    static RotateLog& get_instance() {
        static RotateLog instance;
        return instance;
    }
    
    RotateLog() {
        if (!open_log_file()) {
            fprintf(stderr,"RotateLog::RotateLog() open_log_file() failed!\n");
        }
    }

    void log(const char *fmt, ...) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);

        if (get_file_size(LOG_FILE) >= LOG_FILE_SIZE_LIMIT) {
            if (!rotate_logs()) {
                fprintf(stderr,"RotateLog::log() rotate_logs() failed!\n");
                return;
            }
        }

        if (m_file_ptr) {
            if (access(LOG_FILE, F_OK) != 0) {
                fprintf(stderr,"RotateLog::log() access() failed!\n");

                if (!close_log_file()) {
                    fprintf(stderr,"RotateLog::log() close_log_file() failed!\n");
                    return;
                }

                if (!open_log_file()) {
                    fprintf(stderr,"RotateLog::log() open_log_file() failed!\n");
                    return;
                }
            }
        } else {
            if (!open_log_file()) {
                fprintf(stderr,"RotateLog::log() open_log_file() failed!\n");
                return;
            }
        }

        va_start(args, fmt);
        vfprintf(m_file_ptr, fmt, args);
        fflush(m_file_ptr);
        va_end(args);
    }

private:
    FILE *m_file_ptr {nullptr};

    long get_file_size(const char *filename) {
        struct stat st;
        if (stat(filename, &st) == 0)
            return st.st_size;

        return 0;
    }

    bool rotate_logs() {
        if (!close_log_file()) {
            fprintf(stderr,"RotateLog::rotate_logs() close_log_file() failed!\n");
            return false;
        }

        char old_name[256], new_name[256];

        snprintf(old_name, sizeof(old_name), LOG_FILE ".%d", LOG_ROTATE_NUM);

        if (access(old_name, F_OK) == 0) {
            if (unlink(old_name) != 0) {
                fprintf(stderr,"RotateLog::rotate_logs() unlink() failed!\n");
                return false;
            }
        }

        for (int i = LOG_ROTATE_NUM - 1; i >= 1; --i) {
            snprintf(old_name, sizeof(old_name), LOG_FILE ".%d", i);
            snprintf(new_name, sizeof(new_name), LOG_FILE ".%d", i + 1);

            if (access(old_name, F_OK) == 0) {
                if (rename(old_name, new_name) != 0) {
                    fprintf(stderr,"RotateLog::rotate_logs() rename() failed!\n");
                    return false;
                }
            }
        }

        if (access(LOG_FILE, F_OK) == 0) {
            if (rename(LOG_FILE, LOG_FILE ".1") != 0) {
                fprintf(stderr,"RotateLog::rotate_logs() rename() failed!\n");
                return false;
            }
        }

        if (!open_log_file()) {
            fprintf(stderr,"RotateLog::rotate_logs() open_log_file() failed!\n");
            return false;
        }

        return true;
    }

    bool open_log_file() {
        m_file_ptr = fopen(LOG_FILE, "a");
        if (m_file_ptr == nullptr) {
            fprintf(stderr,"fopen() log file failed!\n");
            return false;
        }

        return true;
    }

    bool close_log_file() {
        if (m_file_ptr == nullptr)
            return true;

        if (fclose(m_file_ptr) != 0) {
            fprintf(stderr,"fclose() log file failed!\n");
            return false;
        }

        m_file_ptr = nullptr;
        return true;
    }
};

inline std::string to_str(const std::string &str) {
    return str;
}

inline std::string to_str(const char *chars) {
    return chars;
}

inline std::string to_str(bool b) {
    return b? "true": "false";
}

template<typename T>
std::string to_str(const T &t) {
    return std::to_string(t);
}

inline std::vector<std::string> split_str(const std::string &str, char delimiter) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }

    return result;
}

inline std::string catenate(const std::string &names) {
    return names;
}

template<typename... Args>
class ArgList {
    std::vector<std::string> arg_names;
    std::tuple<Args...> arg_values;
    std::vector<std::string> arg_values_strs;

public:
    ArgList(Args... values) : arg_values(values...) {
    }

    void set_names(std::vector<std::string> names) {
        arg_names = names;
    }

    std::string string() {
        std::string str;
        std::apply([this](const auto &... elems) {
            ((arg_values_strs.push_back(to_str(elems))), ...);
        }, arg_values);
        for (int i = 0; i < arg_names.size(); i++) {
            str += arg_names[i];
            str += "=";
            str += arg_values_strs[i];
            str += ",";
        }
        if (!str.empty()) {
            str.pop_back();
        }
        return str;
    }
};

class LogLevel {
public:
    static int get() { return level; }

    static void set(int l) { level = l; }

private:
    inline static int level = DEBUG_LEVEL; // set customized log level
};

class PrintLock {
public:
    static std::recursive_mutex &get() {
        static std::recursive_mutex lock;
        return lock;
    }
};

class ThreadColor {
public:
    enum Color {
        Green,
        Pink,
        Teal,
        BoldOrange,
        BoldGreen,
        BoldYellow,
        BoldBlue,
        BoldPink,
        BoldTeal
    };

    static ThreadColor &getInstance() {
        static thread_local ThreadColor t;

        return t;
    }

    void set() {
        switch (my_color) {
            case Green:
                LOG("\033[0;32m");
                break;
            case Pink:
                LOG("\033[0;35m");
                break;
            case Teal:
                LOG("\033[0;36m");
                break;
            case BoldOrange:
                LOG("\033[31;1m");
                break;
            case BoldGreen:
                LOG("\033[32;1m");
                break;
            case BoldYellow:
                LOG("\033[33;1m");
                break;
            case BoldBlue:
                LOG("\033[34;1m");
                break;
            case BoldPink:
                LOG("\033[35;1m");
                break;
            case BoldTeal:
                LOG("\033[36;1m");
                break;
            default:
                LOG("\033[0;37");
                break;
        }
    }

    void reset() { LOG("\033[0m"); }

private:
    int my_color;

    ThreadColor() {
        static int last_color = Green;

        static std::mutex lock;
        std::lock_guard l(lock);

        last_color = ++last_color > BoldTeal ? Green : last_color;

        my_color = last_color;
    }
};

class ThreadDepthKeeper {
public:
    ThreadDepthKeeper() {
        if (LogLevel::get() >= INFO_LEVEL) {
            (*getDepth())++;
        }
    }

    ~ThreadDepthKeeper() {
        if (LogLevel::get() >= INFO_LEVEL) {
            std::lock_guard l(PrintLock::get());

            struct tm *now;
            struct timespec ts{};
            clock_gettime(CLOCK_REALTIME, &ts);
            now = localtime(&ts.tv_sec);
            LOG("%02d:%02d:%02d:%03ld [%s][INFO]: ", now->tm_hour,
                    now->tm_min, now->tm_sec, ts.tv_nsec / 1000000, ModuleName);

            ThreadColor::getInstance().set();

            LOG("%d:%s", __THREADID__,
                    std::string((*getDepth() - 1) * 2, ' ').c_str());

            LOG("  } %s\n", mDepthName.c_str());

            ThreadColor::getInstance().reset();

            if (*getDepth() == 1) {
                LOG("%02d:%02d:%02d:%03ld [%s][INFO]:\n", now->tm_hour,
                        now->tm_min, now->tm_sec, ts.tv_nsec / 1000000, ModuleName);
            }

            fflush(stderr);

            (*getDepth())--;
        }
    }

    void setDepthName(const std::string &name) {
        if (LogLevel::get() >= INFO_LEVEL) {
            mDepthName = name;
        }
    }

    static uint *getDepth() {
        static thread_local uint depth = 0;

        return &depth;
    }

private:
    std::string mDepthName;
};

// a hack for fprintf with one arg
inline int fprintf(FILE *) {
  return 0;
}

// passes no arg
#define LOG_CALL_0(...)      \
    LOG_CALL_X("");          \

// passes arg list, LOG_CALL will print arg names and arg values in human-readable way
#define LOG_CALL(...)                                            \
    auto arg_list = ArgList(__VA_ARGS__);                        \
    arg_list.set_names(split_str(catenate(#__VA_ARGS__), ','));  \
    LOG_CALL_X(arg_list.string().c_str());                       \

// use printf way
#define LOG_CALL_X(...)                                                        \
  ThreadDepthKeeper thread_depth_keeper;                                       \
  do {                                                                         \
    if (LogLevel::get() >= INFO_LEVEL) {                                       \
      struct tm *now;                                                          \
      struct timespec ts{};                                                    \
      clock_gettime(CLOCK_REALTIME,&ts);                                       \
      now = localtime(&ts.tv_sec);                                             \
      {                                                                        \
        std::lock_guard l(PrintLock::get());                                   \
        LOG("%02d:%02d:%02d:%03ld [%s][INFO]: ", now->tm_hour,                 \
                now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName);     \
        ThreadColor::getInstance().set();                                      \
        std::string depth_name = "";                                           \
        std::string func_name = __PRETTY_FUNCTION__;                           \
        func_name = func_name.substr(0, func_name.find('('));                  \
        func_name = func_name.substr(func_name.find_last_of(' ') + 1);         \
        depth_name += func_name;                                               \
        thread_depth_keeper.setDepthName(depth_name);                          \
        LOG("%d:%s", __THREADID__,                                             \
            std::string((*ThreadDepthKeeper::getDepth()-1) * 2, ' ').c_str()); \
        LOG("  %s(", depth_name.c_str());                                      \
        LOG(__VA_ARGS__);                                                      \
        LOG(") { ----%s:%d\n", __FILENAME__, __LINE__);                        \
        fflush(stderr);                                                        \
        ThreadColor::getInstance().reset();                                    \
      }                                                                        \
    }                                                                          \
  } while (0)

// track the thread when enters a block of code
#define LOG_SCOPE(...)                                                         \
  ThreadDepthKeeper thread_depth_keeper;                                       \
  do {                                                                         \
    if (LogLevel::get() >= INFO_LEVEL) {                                       \
      struct tm *now;                                                          \
      struct timespec ts{};                                                    \
      clock_gettime(CLOCK_REALTIME,&ts);                                       \
      now = localtime(&ts.tv_sec);                                             \
      {                                                                        \
        std::lock_guard l(PrintLock::get());                                   \
        PRINT_PLAIN("INFO", "{\n")                                             \
        LOG("%02d:%02d:%02d:%03ld [%s][INFO]: ", now->tm_hour,                 \
                now->tm_min, now->tm_sec, ts.tv_nsec/1000000,ModuleName);      \
        ThreadColor::getInstance().set();                                      \
        std::string depth_name = "";                                           \
        thread_depth_keeper.setDepthName(depth_name);                          \
        LOG("%d:%s", __THREADID__,                                             \
            std::string((*ThreadDepthKeeper::getDepth()-1) * 2, ' ').c_str()); \
        LOG("    ");                                                           \
        LOG(__VA_ARGS__);                                                      \
        LOG("  ----%s:%d\n", __FILENAME__, __LINE__);                          \
        fflush(stderr);                                                        \
        ThreadColor::getInstance().reset();                                    \
      }                                                                        \
    }                                                                          \
  } while (0)

#define PRINT_PLAIN(type, ...)                                                     \
    {                                                                              \
        struct tm *now;                                                            \
        struct timespec ts{};                                                      \
        clock_gettime(CLOCK_REALTIME,&ts);                                         \
        now = localtime(&ts.tv_sec);                                               \
                                                                                   \
        LOG("%02d:%02d:%02d:%03ld [%s][%s]: ", now->tm_hour,                       \
            now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName,type);        \
                                                                                   \
        ThreadColor::getInstance().set();                                          \
                                                                                   \
        LOG("%d:%s", __THREADID__,                                                 \
            std::string((*ThreadDepthKeeper::getDepth()) * 2, ' ').c_str());       \
        LOG(__VA_ARGS__);                                                          \
        fflush(stderr);                                                            \
                                                                                   \
        ThreadColor::getInstance().reset();                                        \
}

#define PRINT(type, ...)                                                       \
  {                                                                            \
    struct tm *now;                                                            \
    struct timespec ts{};                                                      \
    clock_gettime(CLOCK_REALTIME,&ts);                                         \
    now = localtime(&ts.tv_sec);                                               \
                                                                               \
    LOG("%02d:%02d:%02d:%03ld [%s][%s]: ", now->tm_hour,                       \
            now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName,type);    \
                                                                               \
    ThreadColor::getInstance().set();                                          \
                                                                               \
    LOG("%d:%s  \"", __THREADID__,                                             \
            std::string((*ThreadDepthKeeper::getDepth()) * 2, ' ').c_str());   \
    LOG(__VA_ARGS__);                                                          \
    LOG("\" ----%s:%d\n", __FILENAME__, __LINE__);                             \
    fflush(stderr);                                                            \
                                                                               \
    ThreadColor::getInstance().reset();                                        \
  }

#define LOG_ERROR(...)                                 \
  do {                                                 \
    if (LogLevel::get() >= ERROR_LEVEL) {              \
      std::lock_guard ll(PrintLock::get());            \
      LOG("\e[31m");                                   \
      PRINT(" ERR", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_WARN(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= WARN_LEVEL) {               \
      std::lock_guard ll(PrintLock::get());            \
      LOG("\e[33m");                                   \
      PRINT("WARN", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_INFO(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= INFO_LEVEL) {               \
      std::lock_guard ll(PrintLock::get());            \
      PRINT("INFO", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_DBUG(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= DEBUG_LEVEL) {              \
      std::lock_guard ll(PrintLock::get());            \
      PRINT("DBUG", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#endif  // THREADLOG_H
