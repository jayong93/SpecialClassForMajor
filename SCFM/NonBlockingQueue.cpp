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
	atomic_llong data;

	StampedPointer() : data{ 0 } {}
	explicit StampedPointer(Node* p, unsigned int s = 0) : data{((long long)s << 32) | (long long)p} {}
	StampedPointer(const StampedPointer& o) : data{ o.data.load() } {}

	bool operator!=(const StampedPointer& p) { return data != p.data; }
	StampedPointer& operator=(const StampedPointer& p) { data = p.data.load(); return *this; }
	Node* GetPtr() const { return (Node*)(data.load() & 0xffffffff); }
	unsigned int GetStamp() const { return (unsigned int)(data.load() & ((long long)0xffffffff << 32)); }
	void SetPtr(Node* ptr) { data |= 0xffffffff; data &= (long long)ptr; }
	void SetStamp(unsigned int stamp) { data |= ((long long)0xffffffff << 32); data &= ((long long)stamp << 32); }
};

bool CAS(Node* volatile * ptr, Node* old_value, Node* new_value) {
	return atomic_compare_exchange_strong(reinterpret_cast<volatile atomic_uintptr_t*>(ptr), reinterpret_cast<uintptr_t*>(&old_value), reinterpret_cast<uintptr_t>(new_value));
}

bool StampedCAS(StampedPointer* ptr, const StampedPointer& old_value, const StampedPointer& new_value) {
	auto ov = old_value;
	StampedPointer nv{ new_value.GetPtr(), ov.GetStamp() + 1 };
	return atomic_compare_exchange_strong(reinterpret_cast<atomic_llong*>(ptr), reinterpret_cast<long long*>(&ov), *reinterpret_cast<long long*>(&nv));
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
			StampedPointer next{ last.GetPtr()->next };
			if (last != tail) continue;
			if (nullptr == next.GetPtr()) {
				if (CAS(&(last.GetPtr()->next), nullptr, e)) {
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
			StampedPointer next{ first.GetPtr()->next };
			if (first != head) continue;
			if (nullptr == next.GetPtr()) return -1;
			if (first.GetPtr() == last.GetPtr()) {
				StampedCAS(&tail, last, next);
				continue;
			}

			int val = next.GetPtr()->key;
			if (false == StampedCAS(&head, first, next)) continue;
			delete first.GetPtr();
			return val;
		}
	}

	void clear() {
		while (head.GetPtr()->next != nullptr) {
			Node *tmp = head.GetPtr();
			head.SetPtr(head.GetPtr()->next);
			delete tmp;
		}
		tail = head;
	}

	void dump(size_t count) {
		auto& ptr = head.GetPtr()->next;
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