#include <vector>
#include <iostream>
#include <thread>
#include <unistd.h>

void worker(void *arg)
{
	int cnt = 0;
	while(true) {
		std::cout << std::this_thread::get_id() << "    " << cnt++ << std::endl;
		// for(int i = 0; i < 100000000; i++){}
        sleep(1);
	}
	
}
int main()
{

}