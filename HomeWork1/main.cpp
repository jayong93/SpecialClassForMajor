#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>

using namespace std;
thread_local int threadId;
volatile int sum, t_count;
volatile bool flags[16];
volatile int labels[16];

void BackeryLock() {
	flags[threadId] = true;
	labels[threadId] = (*max_element(labels, labels + t_count)) + 1;
	flags[threadId] = false;
	for (int i = 0; i < t_count; ++i) {
		if (i == threadId) continue;
		while (flags[i]);
		while (labels[i] && ((labels[i] < labels[threadId]) || ((labels[i] == labels[threadId]) && (i < threadId))));
	}
}
void BackeryUnlock() {
	labels[threadId] = 0;
}

struct ThreadProcess {
	virtual void operator()() = 0;
	virtual void PrintLog() = 0;
};

struct NoLock : public ThreadProcess {
	virtual void operator()() { sum += 2; }
	virtual void PrintLog() { cout << "Start no-lock version\n"; }
} noLock;

struct UseMutexLock : public ThreadProcess {
	mutex lock;
	virtual void operator()() { lock.lock(); sum += 2; lock.unlock(); }
	virtual void PrintLog() { cout << "Start mutex lock version\n"; }
} mutexLock;

struct UseBackeryLock : public ThreadProcess {
	virtual void operator()() { BackeryLock(); sum += 2; BackeryUnlock(); }
	virtual void PrintLog() { cout << "Start backery lock version\n"; }
} backeryLock;

void ThreadFunc(ThreadProcess* p, int id) {
	threadId = id;
	for (int i = 0; i < 50000000 / t_count; ++i) {
		(*p)();
	}
}

int main() {
	ThreadProcess* processes[] = { &noLock, &mutexLock, &backeryLock };
	for (auto p : processes) {
		p->PrintLog();
		for (t_count = 1; t_count <= 16; t_count *= 2) {
			for (int i = 0; i < 16; ++i) { flags[i] = false; labels[i] = 0; }
			sum = 0;
			vector<thread*> t_v;
			auto start_t = chrono::high_resolution_clock::now();
			for (auto i = 0; i < t_count; ++i) {
				t_v.emplace_back(new thread{ ThreadFunc, p, i });
			}
			for (auto t : t_v) {
				t->join();
				delete t;
			}
			auto du = chrono::high_resolution_clock::now() - start_t;
			cout << t_count << " Thread, " << "Time : " << chrono::duration_cast<chrono::milliseconds>(du).count();
			cout << "\tSum = " << sum << endl;
		}
	}
	system("pause");
}