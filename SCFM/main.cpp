#include <chrono>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>

using namespace std;

int t_count, sum;
mutex myLock;

void thread_func() {
	for (auto i = 0; i < 50000000 / t_count; ++i) {
		myLock.lock();
		sum = sum + 2;
		myLock.unlock();
	}
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