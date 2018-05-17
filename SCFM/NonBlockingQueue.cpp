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

struct StampedPointer {
	Node* volatile ptr;
	unsigned int stamp;

	StampedPointer() : ptr{ nullptr }, stamp{ 0 } {}
	explicit StampedPointer(Node* p, unsigned int s = 0) : ptr{ p }, stamp{ s } {}
	bool operator!=(const StampedPointer& p) { return (ptr != p.ptr) || (stamp != p.stamp); }
};

bool CAS(Node* volatile * ptr, Node* old_value, Node* new_value) {
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_uintptr_t*>(ptr), reinterpret_cast<uintptr_t*>(&old_value), reinterpret_cast<uintptr_t>(new_value));
}

bool StampedCAS(StampedPointer* ptr, StampedPointer old_value, StampedPointer new_value) {
	new_value.stamp = old_value.stamp + 1; // ABA 문제를 해결하기 위해 꼭 필요.
	return atomic_compare_exchange_strong(reinterpret_cast<atomic_llong*>(ptr), reinterpret_cast<long long*>(&old_value), *reinterpret_cast<long long*>(&new_value));
}

class LFQUEUE {
	StampedPointer head, tail;
public:
	LFQUEUE() : head{ new Node{0} }, tail{ head } {}

	void Enqueue(int x) {
		Node* e{ new Node{x} };
		StampedPointer se{ e };
		while (true) {
			StampedPointer last{ tail };
			StampedPointer next{ last.ptr->next };
			if (last != tail) continue;
			if (nullptr == next.ptr) {
				if (CAS(&(last.ptr->next), nullptr, e)) {
					StampedCAS(&tail, last, se);
					return;
				}
			}
			else StampedCAS(&tail, last, next);
		}
	}

	int Dequeue() {
		while (true) {
			StampedPointer first{ head };
			StampedPointer last{ tail };
			StampedPointer next{ first.ptr->next };
			if (first != head) continue;
			if (first.ptr == last.ptr) {
				if (nullptr == next.ptr) return -1;
				StampedCAS(&tail, last, next);
				continue;
			}

			int val = next.ptr->key;
			if (false == StampedCAS(&head, first, next)) continue;
			delete first.ptr;
			return val;
		}
	}

	void clear() {
		while (head.ptr->next != nullptr) {
			Node *tmp = head.ptr;
			head.ptr = head.ptr->next;
			delete tmp;
		}
		tail = head;
	}

	void dump(size_t count) {
		auto& ptr = head.ptr->next;
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

	for (auto thread_num = 1; thread_num <= 128; thread_num *= 2) {
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