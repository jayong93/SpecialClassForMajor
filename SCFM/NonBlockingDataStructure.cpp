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

	Node() : next{ nullptr } {}
	Node(int key) : key{ key }, next{ nullptr } {}
	~Node() {}
};

class CSet {
	Node head, tail;
	mutex lock;
public:
	CSet() : head{ 0x80000000 }, tail{ 0x7fffffff } { head.next = &tail; }
	bool add(int x) {
		Node *pred, *curr;
		pred = &head;

		lock_guard<mutex> lck(lock);
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key == x) { return false; }
		else {
			auto e = new Node{ x };
			e->next = curr;
			pred->next = e;
			return true;
		}
	}
	bool remove(int x) {
		Node *pred, *curr;
		pred = &head;
		lock_guard<mutex> lck(lock);
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) { return false; }
		else {
			pred->next = curr->next;
			delete curr;
			return true;
		}
	}
	bool contains(int x) {
		Node *pred, *curr;
		pred = &head;
		lock_guard<mutex> lck(lock);
		curr = pred->next;
		while (curr->key < x) {
			pred = curr;
			curr = curr->next;
		}

		if (curr->key != x) { return false; }
		else { return true; }
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
} myCSet;

void benchMark(int num_thread) {
	for (int i = 0; i < NUM_TEST / num_thread; ++i) {
		switch (rand()%3)
		{
		case 0:
			myCSet.add(rand() % RANGE);
			break;
		case 1:
			myCSet.remove(rand() % RANGE);
			break;
		case 2:
			myCSet.contains(rand() % RANGE);
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
		myCSet.clear();
		threads.clear();

		auto start_t = chrono::high_resolution_clock::now();
		generate_n(back_inserter(threads), thread_num, [thread_num]() {return thread{ benchMark, thread_num }; });
		for_each(threads.begin(), threads.end(), [](auto& t) {t.join(); });
		auto du = chrono::high_resolution_clock::now() - start_t;

		myCSet.dump(20);

		cout << thread_num << "Threads, Time = ";
		cout << chrono::duration_cast<chrono::milliseconds>(du).count() << "ms \n";
	}

	system("pause");
}