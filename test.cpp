#include <thread>

#include "ThreadLog.h"

void call_3()
{
	TRACK_CALL("no arg");
	{
		TRACK("handle call sleep","%d",1);
		sleep(1);
	}
	LOG_INFO("this is call_3");
}

void call_2()
{
	TRACK_CALL();
	sleep(1);
	call_3();
}

void call_1()
{
	TRACK_CALL();
	sleep(1);
	call_2();
}

int main()
{
	
	std::thread t_1 = std::thread([&]{
		call_1();
	});
	
	std::thread t_2 = std::thread([&]{
		call_1();
	});
	
	std::thread t_3 = std::thread([&]{
		call_1();
	});
	
	t_1.join();
	t_2.join();
	t_3.join();
	
	
	return 0;
}
