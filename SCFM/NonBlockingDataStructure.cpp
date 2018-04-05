#include <iostream>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>
#include <iterator>
#include <chrono>

using namespace std;

static constexpr int NUM_TEST = 4000000;
static constexpr int RANGE = 1000;

struct Node {
public:
	int key;
	Node* next;
	mutex m_lock;

	Node() : next{ nullptr } {}
	Node(int key) : key{ key }, next{ nullptr } {}
	~Node() {}
	void lock() { m_lock.lock(); }
	void unlock() { m_lock.unlock(); }
};

class FSet {
	Node head, tail;
public:
	FSet() : head{ 0x80000000 }, tail{ 0x7fffffff } { head.next = &tail; }

	bool add(int x) {
		Node *pred, *curr;
		head.lock();
		pred = &head;
		pred->next->lock();
		curr = pred->next;

		while (curr->key < x) {
			pred->unlock();
			pred = curr;
			curr->next->lock();
			curr = curr->next;
		}

		if (curr->key == x) { pred->unlock(); curr->unlock(); return false; }
		else {
			auto e = new Node{ x };
			e->next = curr;
			pred->next = e;

			pred->unlock(); curr->unlock();
			return true;
		}
	}

	bool remove(int x) {
		Node *pred, *curr;

		head.lock();
		pred = &head;
		pred->next->lock();
		curr = pred->next;

		while (curr->key < x) {
			pred->unlock();
			pred = curr;
			curr->next->lock();
			curr = curr->next;
		}

		if (curr->key != x) { pred->unlock(); curr->unlock(); return false; }
		else {
			pred->next = curr->next;
			pred->unlock(); curr->unlock();
			delete curr;
			return true;
		}
	}

	bool contains(int x) {
		Node *pred, *curr;
		head.lock();
		pred = &head;
		pred->next->lock();
		curr = pred->next;

		while (curr->key < x) {
			pred->unlock();
			pred = curr;
			curr->next->lock();
			curr = curr->next;
		}

		if (curr->key != x) { pred->unlock(); curr->unlock(); return false; }
		else { pred->unlock(); curr->unlock(); return true; }
	}

	void clear() {
		while (head.next != &tail) {
			auto temp = head.next;
			head.next = temp->next;
			delete temp;
		}
	}

	void dump(size_t count) {
		auto ptr = head.next;
		cout << count << " Result : ";
		for (auto i = 0; i < count && ptr != &tail; ++i) {
			cout << ptr->key << " ";
			ptr = ptr->next;
		}
		cout << "\n";
	}
} myFSet;

void benchMark(int num_thread) {
	for (int i = 0; i < NUM_TEST / num_thread; ++i) {
		switch (rand()%3)
		{
		case 0:
			myFSet.add(rand() % RANGE);
			break;
		case 1:
			myFSet.remove(rand() % RANGE);
			break;
		case 2:
			myFSet.contains(rand() % RANGE);
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
		myFSet.clear();
		threads.clear();

		auto start_t = chrono::high_resolution_clock::now();
		generate_n(back_inserter(threads), thread_num, [thread_num]() {return thread{ benchMark, thread_num }; });
		for_each(threads.begin(), threads.end(), [](auto& t) {t.join(); });
		auto du = chrono::high_resolution_clock::now() - start_t;

		myFSet.dump(20);

		cout << thread_num << "Threads, Time = ";
		cout << chrono::duration_cast<chrono::milliseconds>(du).count() << "ms \n";
	}

	system("pause");
}