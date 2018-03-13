#include <chrono>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>

using namespace std;

int t_count;
atomic<int> sum;
mutex myLock;

void thread_func() {
	volatile int local_sum = 0;
	for (auto i = 0; i < 50000000 / t_count; ++i) {
		local_sum += 2;
		// sum = sum + 2;   // 이 코드는 올바른 결과를 내지 않는다. atomic한 연산들을 합쳐도 전체가 atomic한건 아니다.
	}
	myLock.lock();
	sum += local_sum;
	myLock.unlock();
}

int main() {
	for (t_count = 1; t_count <= 16; t_count *= 2) {
		sum = 0;
		vector<thread*> t_v;
		auto start_t = chrono::high_resolution_clock::now();
		for (auto i = 0; i < t_count; ++i) {
			t_v.emplace_back(new thread{ thread_func });
		}
		for (auto t : t_v) {
			t->join();
			delete t;
		}
		auto du = chrono::high_resolution_clock::now() - start_t;
		cout << "Time : " << chrono::duration_cast<chrono::milliseconds>(du).count();
		cout << "\tSum = " << sum << endl;
	}
	char a;
	cin >> a;
}