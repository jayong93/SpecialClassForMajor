#include <iostream>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <memory>

using namespace std;

static constexpr int NUM_TEST = 10000000;
static constexpr int RANGE = 1000;

struct Node {
public:
	int key;
	Node * volatile next;

	Node() : next{ nullptr } {}
	Node(int key) : key{ key }, next{ nullptr } {}
	~Node() {}
};

class CStack {
	Node* top;
	mutex lock;
public:
	CStack() : top{ nullptr } {}

	void Push(int x) {
		auto e = new Node{ x };
		unique_lock<mutex> lg{ lock };
		e->next = top;
		top = e;
	}

	int Pop() {
		unique_lock<mutex> lg{ lock };
		if (nullptr == top) return 0;
		int tmp = top->key;
		Node* ptr = top;
		top = top->next;
		delete ptr;
		return tmp;
	}

	void clear() {
		if (nullptr == top) return;
		while (top->next != nullptr) {
			Node *tmp = top;
			top = top->next;
			delete tmp;
		}
	}

	void dump(size_t count) {
		auto& ptr = top;
		cout << count << " Result : ";
		for (auto i = 0; i < count; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << "\n";
	}
} myStack;

void benchMark(int num_thread) {
	for (int i = 1; i <= NUM_TEST / num_thread; ++i) {
		if ((rand() % 2) || i <= 1000 / num_thread) {
			myStack.Push(i);
		}
		else {
			myStack.Pop();
		}
	}
}

int main() {
	vector<thread> threads;

	for (auto thread_num = 1; thread_num <= 16; thread_num *= 2) {
		myStack.clear();
		threads.clear();

		auto start_t = chrono::high_resolution_clock::now();
		generate_n(back_inserter(threads), thread_num, [thread_num]() {return thread{ benchMark, thread_num }; });
		for (auto& t : threads) { t.join(); }
		auto du = chrono::high_resolution_clock::now() - start_t;

		myStack.dump(10);

		cout << thread_num << "Threads, Time = ";
		cout << chrono::duration_cast<chrono::milliseconds>(du).count() << "ms\n";
	}

	system("pause");
}