#include <thread>
#include <string>

#include "ThreadLog.h"

void call_3() {
    TRACK_CALL();  // do not use TRACK_CALL_X(), or compile will fail 
    {
        TRACK("sleep", "%d", 1);
        sleep(1);
    }
    LOG_INFO("this is call_3");
}

void call_2(int a, float b) {
    TRACK_CALL("a:%d, b:%.1f", a, b);
    sleep(1);
    call_3();
}

void call_1(int a, float b, std::string c, bool d) {
    TRACK_CALL_X(a, b, c, d);
    sleep(1);
    call_2(a, b);
}

int main() {
    std::thread t_1 = std::thread([&] {
        call_1(1, 2.0, "three", true);
    });

    std::thread t_2 = std::thread([&] {
        call_1(2, 3.0, "four", false);
    });

    std::thread t_3 = std::thread([&] {
        call_1(3, 4.0, "five", false);
    });

    t_1.join();
    t_2.join();
    t_3.join();


    return 0;
}
