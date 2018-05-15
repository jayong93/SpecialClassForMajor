#include <iostream>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
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
	Node* next;

	Node() : next{ nullptr } {}
	Node(int key) : key{ key }, next{ nullptr } {}
	~Node() {}
};

class CQUEUE {
	Node *head, *tail;
	mutex enqLock, deqLock;
public:
	CQUEUE() : head{ new Node{0} }, tail{ head } {}

	void Enqueue(int x) {
		Node* e{ new Node{x} };
		unique_lock<mutex> lg{ enqLock };
		tail->next = e;
		tail = e;
	}

	int Dequeue() {
		unique_lock<mutex> lg{ deqLock };
		if (nullptr == head->next) return -1;
		int val = head->next->key;
		auto tmp = head;
		head = head->next;
		delete tmp;
		return val;
	}

	void clear() {
		while (head->next != nullptr) {
			Node *tmp = head;
			head = head->next;
			delete tmp;
		}
	}

	void dump(size_t count) {
		auto& ptr = head->next;
		cout << count << " Result : ";
		for (auto i = 0; i < count; ++i) {
			if (nullptr == ptr) break;
			cout << ptr->key << ", ";
			ptr = ptr->next;
		}
		cout << "\n";
	}
} myQueue;

void benchMark(int num_thread) {
	for (int i = 0; i < NUM_TEST / num_thread; ++i) {
		if ((rand() % 2) || i < 1000 / num_thread) { // Dequeue할 때 Queue가 비어있는 상태를 막기 위해 미리 1000개의 요소를 Enqueue
			myQueue.Enqueue(i);
		}
		else {
			myQueue.Dequeue();
		}
	}
}

int main() {
	vector<thread> threads;

	for (auto thread_num = 1; thread_num <= 8; thread_num *= 2) {
		myQueue.clear();
		threads.clear();

		auto start_t = chrono::high_resolution_clock::now();
		generate_n(back_inserter(threads), thread_num, [thread_num]() {return thread{ benchMark, thread_num }; });
		for (auto& t : threads) { t.join(); }
		auto du = chrono::high_resolution_clock::now() - start_t;

		myQueue.dump(10);

		cout << thread_num << "Threads, Time = ";
		cout << chrono::duration_cast<chrono::milliseconds>(du).count() << "ms\n";
	}

	system("pause");
}