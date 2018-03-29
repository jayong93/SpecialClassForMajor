#include <chrono>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>

using namespace std;

bool CAS(atomic_int* addr, int expected, int newVal) {
	return atomic_compare_exchange_strong(addr, &expected, newVal);
}

struct CASObject {
	atomic<int> obj{ 0 };

	void lock() { while (false == CAS(&obj, 0, 1)); }
	void unlock() { while (false == CAS(&obj, 1, 0)); }
};

volatile int sum;
CASObject myLock;

void thread_func(int threadNum) {
	for (auto i = 0; i < 50000000 / threadNum; ++i) {
		myLock.lock();
		sum += 2;
		myLock.unlock();
	}
}

int main() {
	for (auto t_count = 1; t_count <= 8; t_count *= 2) {
		sum = 0;
		vector<thread*> t_v;
		auto start_t = chrono::high_resolution_clock::now();
		for (auto i = 0; i < t_count; ++i) {
			t_v.emplace_back(new thread{ thread_func, t_count });
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