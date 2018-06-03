#include <iostream>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <atomic>

using namespace std;

static constexpr int NUM_TEST = 4000000;
static constexpr int RANGE = 1000;
static constexpr int MAX_LEVEL{ 10 };

class SKNode {
public:
	SKNode() : value{ 0 }, top_level{ 0 } {
		for (auto i = 0; i < MAX_LEVEL; ++i) next[i] = nullptr;
	}
	SKNode(int x) : value{ x } { for (auto i = 0; i < MAX_LEVEL; ++i) next[i] = nullptr; }
	SKNode(int x, int top) : value{ x }, top_level{ top } { for (auto i = 0; i < MAX_LEVEL; ++i) next[i] = nullptr; }

	int value;
	int top_level;
	SKNode* next[MAX_LEVEL];
};

class SKSet {
	mutex lock;
	SKNode head, tail;
public:
	SKSet() : head{ 0x80000000 }, tail{ 0x7fffffff } {
		head.top_level = MAX_LEVEL - 1;
		tail.top_level = MAX_LEVEL - 1;
		for (auto& p : head.next) { p = &tail; }
	}

	bool add(int x) {
		SKNode *pred[MAX_LEVEL], *curr[MAX_LEVEL];
		unique_lock<mutex> lg{ lock };

		find(x, pred, curr);

		if (curr[0]->value == x) {
			return false;
		}
		else {
			auto top = 0;
			while (rand() % 2 == 1) {
				top += 1;
				if (top >= MAX_LEVEL - 1) break;
			}

			auto node = new SKNode{ x, top };
			// Ãß°¡
			return true;
		}
	}

	bool remove(int x) {

	}

	void find(int x, SKNode* preds[], SKNode* currs[]) {
		preds[MAX_LEVEL - 1] = &head;
		for (auto level = MAX_LEVEL - 1; level >= 0; --level) {
			currs[level] = preds[level]->next[level];
			while (currs[level] != nullptr && currs[level]->value < x) {
				preds[level] = currs[level];
				currs[level] = currs[level]->next[level];
			}
			if (level > 0)
				preds[level - 1] = preds[level];
		}
	}

	bool contains(int x) {

	}

	void clear() {
		while (head.next[0] != &tail) {
			auto tmp = head.next[0];
			head.next[0] = tmp->next[0];
			delete tmp;
		}
		for (auto i = 0; i < MAX_LEVEL; ++i) {
			head.next[i] = &tail;
		}
	}

	void dump(size_t count) {
		auto ptr = head.next[0];
		cout << count << " Result : ";
		for (auto i = 0; i < count && ptr != &tail; ++i) {
			cout << ptr->value << " ";
			ptr = ptr->next[0];
		}
		cout << "\n";
	}
} mySet;

void benchMark(int num_thread) {
	for (int i = 0; i < NUM_TEST / num_thread; ++i) {
		switch (rand() % 3)
		{
		case 0:
			mySet.add(rand() % RANGE);
			break;
		case 1:
			mySet.remove(rand() % RANGE);
			break;
		case 2:
			mySet.contains(rand() % RANGE);
			break;
		default:
			cout << "Error\n";
			exit(1);
		}
	}
}

int main() {
	vector<thread> threads;

	for (auto thread_num = 1; thread_num <= 16; thread_num *= 2) {
		mySet.clear();
		threads.clear();

		auto start_t = chrono::high_resolution_clock::now();
		generate_n(back_inserter(threads), thread_num, [thread_num]() {return thread{ benchMark, thread_num }; });
		for (auto& t : threads) { t.join(); }
		auto du = chrono::high_resolution_clock::now() - start_t;

		//nodePool.Recycle();
		mySet.dump(20);

		cout << thread_num << "Threads, Time = ";
		cout << chrono::duration_cast<chrono::milliseconds>(du).count() << "ms \n";
	}

	system("pause");
}