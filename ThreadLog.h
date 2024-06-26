
#ifndef THREADLOG_H
#define THREADLOG_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/timeb.h>

#include <mutex>
#include <string>

#define ModuleName "UserDefine" // set customized log title

#define ERROR_LEVEL 0
#define WARN_LEVEL  1
#define INFO_LEVEL  2
#define DEBUG_LEVEL 3

#define __FILENAME__ \
  (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define __THREADID__ (int)syscall(SYS_gettid)

class LogLevel {
 public:
  static int get() { return level; }

  static void set(int l) { level = l; }

 private:
  inline static int level = DEBUG_LEVEL; // set customized log level
};

class PrintLock {
 public:
  static std::mutex &get() {
    static std::mutex lock;
    return lock;
  }
};

class ThreadColor {
 public:
  enum Color {
    Green,
    Yellow,
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
        fprintf(stderr, "\033[0;32m");
        break;
      case Yellow:
        fprintf(stderr, "\033[0;33m");
        break;
      case Pink:
        fprintf(stderr, "\033[0;35m");
        break;
      case Teal:
        fprintf(stderr, "\033[0;36m");
        break;
      case BoldOrange:
        fprintf(stderr, "\033[31;1m");
        break;
      case BoldGreen:
        fprintf(stderr, "\033[32;1m");
        break;
      case BoldYellow:
        fprintf(stderr, "\033[33;1m");
        break;
      case BoldBlue:
        fprintf(stderr, "\033[34;1m");
        break;
      case BoldPink:
        fprintf(stderr, "\033[35;1m");
        break;
      case BoldTeal:
        fprintf(stderr, "\033[36;1m");
        break;
      default:
        fprintf(stderr, "\033[0;37");
        break;
    }
  }

  void reset() { fprintf(stderr, "\033[0m"); }

 private:
  int my_color;

  ThreadColor() {
    static int last_color = Green;

    static std::mutex lock;
    std::lock_guard<std::mutex> l(lock);

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
      std::lock_guard<std::mutex> l(PrintLock::get());

      struct tm *now;
      struct timespec ts{};
      clock_gettime(CLOCK_REALTIME,&ts);
      now = localtime(&ts.tv_sec);
      fprintf(stderr, "%02d:%02d:%02d:%03ld [%s][TRAC]: ", now->tm_hour,
              now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName);

      ThreadColor::getInstance().set();

      fprintf(stderr, "%d:%s", __THREADID__,
              std::string((*getDepth()-1) * 2, ' ').c_str());

      fprintf(stderr, "--%s()\n", mDepthName.c_str());

      ThreadColor::getInstance().reset();

      if (*getDepth() == 1) {
          fprintf(stderr, "%02d:%02d:%02d:%03ld [%s][TRAC]:\n", now->tm_hour,
                  now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName);
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
inline int fprintf (FILE*) {}

// track the thread when calls a function
#define TRACK_CALL(...)                                                        \
  ThreadDepthKeeper thread_depth_keeper;                                       \
  do {                                                                         \
    if (LogLevel::get() >= INFO_LEVEL) {                                       \
      struct tm *now;                                                          \
      struct timespec ts{};                                                    \
      clock_gettime(CLOCK_REALTIME,&ts);                                       \
      now = localtime(&ts.tv_sec);                                             \
      {                                                                        \
        std::lock_guard<std::mutex> l(PrintLock::get());                       \
        fprintf(stderr, "%02d:%02d:%02d:%03ld [%s][TRAC]: ", now->tm_hour,     \
                now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName);     \
        ThreadColor::getInstance().set();                                      \
        std::string depth_name = "handle call: ";                              \
        std::string func_name = __PRETTY_FUNCTION__;                           \
        func_name = func_name.substr(0, func_name.find('('));                  \
        func_name = func_name.substr(func_name.find_last_of(' ') + 1);         \
        depth_name += func_name;                                               \
        thread_depth_keeper.setDepthName(depth_name);                          \
        fprintf(                                                               \
            stderr, "%d:%s", __THREADID__,                                     \
            std::string((*ThreadDepthKeeper::getDepth()-1) * 2, ' ').c_str()); \
        fprintf(stderr, "++%s(", depth_name.c_str());                          \
        fprintf(stderr, ##__VA_ARGS__);                                        \
        fprintf(stderr, ") ----%s:%d\n", __FILENAME__, __LINE__);              \
        fflush(stderr);                                                        \
        ThreadColor::getInstance().reset();                                    \
      }                                                                        \
    }                                                                          \
  } while (0)

// track the thread when enters a block of code
#define TRACK(name,...)                                                        \
  ThreadDepthKeeper thread_depth_keeper;                                       \
  do {                                                                         \
    if (LogLevel::get() >= INFO_LEVEL) {                                       \
      struct tm *now;                                                          \
      struct timespec ts{};                                                    \
      clock_gettime(CLOCK_REALTIME,&ts);                                       \
      now = localtime(&ts.tv_sec);                                             \
      {                                                                        \
        std::lock_guard<std::mutex> l(PrintLock::get());                       \
        fprintf(stderr, "%02d:%02d:%02d:%03ld [%s][TRAC]: ", now->tm_hour,     \
                now->tm_min, now->tm_sec, ts.tv_nsec/1000000,ModuleName);      \
        ThreadColor::getInstance().set();                                      \
        std::string depth_name = name;                                         \
        thread_depth_keeper.setDepthName(depth_name);                          \
        fprintf(                                                               \
            stderr, "%d:%s", __THREADID__,                                     \
            std::string((*ThreadDepthKeeper::getDepth()-1) * 2, ' ').c_str()); \
        fprintf(stderr, "++%s(", depth_name.c_str());                          \
        fprintf(stderr, ##__VA_ARGS__);                                        \
        fprintf(stderr, ") ----%s:%d\n", __FILENAME__, __LINE__);              \
        fflush(stderr);                                                        \
        ThreadColor::getInstance().reset();                                    \
      }                                                                        \
    }                                                                          \
  } while (0)

#define PRINT(type, ...)                                                       \
  {                                                                            \
    struct tm *now;                                                            \
    struct timespec ts{};                                                      \
    clock_gettime(CLOCK_REALTIME,&ts);                                         \
    now = localtime(&ts.tv_sec);                                               \
                                                                               \
    fprintf(stderr, "%02d:%02d:%02d:%03ld [%s][%s]: ", now->tm_hour,           \
            now->tm_min, now->tm_sec, ts.tv_nsec/1000000, ModuleName,type);    \
                                                                               \
    ThreadColor::getInstance().set();                                          \
                                                                               \
    fprintf(stderr, "%d:%s\"", __THREADID__,                                   \
            std::string((*ThreadDepthKeeper::getDepth()) * 2, ' ').c_str());   \
    fprintf(stderr, ##__VA_ARGS__);                                            \
    fprintf(stderr, "\" ----%s:%d\n", __FILENAME__, __LINE__);                 \
    fflush(stderr);                                                            \
                                                                               \
    ThreadColor::getInstance().reset();                                        \
  }

#define LOG_ERROR(...)                                 \
  do {                                                 \
    if (LogLevel::get() >= ERROR_LEVEL) {              \
      std::lock_guard<std::mutex> ll(PrintLock::get());\
      fprintf(stderr, "\e[31m");                       \
      PRINT(" ERR", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_WARN(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= WARN_LEVEL) {               \
      std::lock_guard<std::mutex> ll(PrintLock::get());\
      fprintf(stderr, "\e[33m");                       \
      PRINT("WARN", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_INFO(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= INFO_LEVEL) {               \
      std::lock_guard<std::mutex> ll(PrintLock::get());\
      PRINT("INFO", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#define LOG_DBUG(...)                                  \
  do {                                                 \
    if (LogLevel::get() >= DEBUG_LEVEL) {              \
      std::lock_guard<std::mutex> ll(PrintLock::get());\
      PRINT("DBUG", __VA_ARGS__)                       \
    }                                                  \
  } while (0)

#endif  // THREADLOG_H
