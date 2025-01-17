#include <thread>
#include <string>

#include "ThreadLog.h"

void func_3() {
    LOG_CALL_0();
    {
        LOG_SCOPE("sleep(%d)", 1);
        sleep(1);
    }
    LOG_INFO("this is call_3");
}

void func_2(int a, float b) {
    LOG_CALL_X("a:%d, b:%.1f", a, b);
    sleep(1);
    func_3();
}

void func_1(int a, float b, std::string c, bool d) {
    LOG_CALL(a, b, c, d);
    sleep(1);
    func_2(a, b);
}

int main() {
    LOG_CALL_0();

    std::thread t_1 = std::thread([&] {
        func_1(1, 2.0, "three", true);
    });

    std::thread t_2 = std::thread([&] {
        func_1(2, 3.0, "four", false);
    });

    std::thread t_3 = std::thread([&] {
        func_1(3, 4.0, "five", false);
    });

    t_1.join();
    t_2.join();
    t_3.join();

    LOG_DBUG("TEST LOG_DBUG");
    LOG_WARN("TEST LOG_WARN");
    LOG_ERROR("TEST LOG_ERROR");
    return 0;
}
