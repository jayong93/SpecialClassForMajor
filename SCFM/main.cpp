#include <chrono>
#include <vector>
#include <thread>
#include <iostream>
#include <mutex>
#include <atomic>

using namespace std;

int t_count;
int sum;
mutex myLock;

volatile bool flag[2] = { false, false };
volatile int victim;
thread_local int myID;

void p_lock() {
	int other = 1 - myID;
	flag[myID] = true;
	victim = myID;
	while (flag[other] && (victim == myID));
}

void p_unlock() {
	flag[myID] = false;
}

void thread_func(int id) {
	myID = id;
	for (auto i = 0; i < 50000000 / t_count; ++i) {
		p_lock();
		sum = sum + 2;
		p_unlock();
	}
}

int main() {
	for (t_count = 2; t_count <= 2; t_count *= 2) {
		sum = 0;
		vector<thread*> t_v;
		auto start_t = chrono::high_resolution_clock::now();
		for (auto i = 0; i < t_count; ++i) {
			t_v.emplace_back(new thread{ thread_func, i });
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