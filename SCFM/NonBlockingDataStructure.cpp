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

struct LFNode {
public:
	int key;
	unsigned int next;

	LFNode() : next{ 0 } {}
	LFNode(int key) : key{ key }, next{ 0 } {}
	LFNode* GetNext() {
		return reinterpret_cast<LFNode*>(next & 0xFFFFFFFE);
	}

	LFNode* GetNextWithMark(bool& mark) {
		int temp = next;
		mark = ((temp % 2) == 1);
		return reinterpret_cast<LFNode*>(temp & 0xFFFFFFFE);
	}

	void SetNext(LFNode* ptr) {
		next = reinterpret_cast<unsigned>(ptr);
	}

	bool CAS(unsigned old_value, unsigned new_value) {
		return atomic_compare_exchange_strong(reinterpret_cast<atomic_uint*>(&next), &old_value, new_value);
	}

	bool CAS(LFNode* old_next, LFNode* new_next, bool old_mark, bool new_mark) {
		unsigned int old_value = reinterpret_cast<unsigned int>(old_next);
		if (old_mark) old_value |= 0x1;
		else old_value &= 0xFFFFFFFE;

		unsigned new_value = reinterpret_cast<unsigned int>(new_next);
		if (new_mark) new_value |= 0x1;
		else new_value &= 0xFFFFFFFE;

		return CAS(old_value, new_value);
	}

	bool TryMark() {
		// �ٸ� �����尡 ���� ��ŷ�ϸ� �����ؾ� ��.
		unsigned old_value = next & 0xFFFFFFFE;
		unsigned new_value = old_value | 0x1;
		return CAS(old_value, new_value);
	}

	~LFNode() {}
};


//class NodeManager {
//	LFNode *first, *second;
//	mutex firstLock, secondLock;
//
//public:
//	NodeManager() : first{ nullptr }, second{ nullptr } {}
//	~NodeManager() {
//		while (nullptr != first) {
//			LFNode* p = first;
//			first = first->next;
//			delete p;
//		}
//		while (nullptr != second) {
//			LFNode* p = second;
//			second = second->next;
//			delete p;
//		}
//	}
//
//	LFNode* GetNode(int x) {
//		lock_guard<mutex> lck{ firstLock };
//		if (nullptr == first) {
//			return new LFNode{x};
//		}
//		LFNode *p = first;
//		first = first->next;
//		p->key = x;
//		p->deleted = false;
//		p->next = nullptr;
//		return p;
//	}
//
//	void FreeNode(LFNode *n) {
//		lock_guard<mutex> lck{ secondLock };
//		n->next = second;
//		second = n;
//	}
//
//	void Recycle() {
//		LFNode *p = first;
//		first = second;
//		second = p;
//	}
//} nodePool;

class LFSet {
	LFNode head, tail;
public:
	LFSet() : head{ 0x80000000 }, tail{ 0x7fffffff } { head.SetNext(&tail); }

	bool add(int x) {
		LFNode *pred, *curr;
		while (true)
		{
			find(x, pred, curr);
			if (curr->key == x) {
				return false;
			}
			else {
				auto node = new LFNode{ x };
				node->SetNext(curr);
				if (pred->CAS(curr, node, false, false))
					return true;
			}
		}
	}

	bool remove(int x) {
		LFNode *pred, *curr;
		while (true)
		{
			find(x, pred, curr);

			if (curr->key != x) {
				return false;
			}
			else {
				auto retval = curr->TryMark();
				auto succ = curr->GetNext();
				if (!retval)
					continue;

				pred->CAS(curr, succ, false, false);
				return true;
			}
		}
	}

	void find(int key, LFNode*& pred, LFNode*& curr) {
	retry:
		LFNode *pr = &head;
		LFNode *cu = pr->GetNext();

		while (true) {
			bool removed{ false };
			auto su = cu->GetNextWithMark(removed);
			while (true == removed) {
				if (false == pr->CAS(cu, su, false, false))
					goto retry;
				cu = su;
				su = cu->GetNextWithMark(removed);
			}

			if (cu->key >= key) {
				pred = pr;
				curr = cu;
				return;
			}

			pr = cu;
			cu = cu->GetNext();
		}
	}

	bool contains(int x) {
		LFNode *pred, *curr;
		pred = &head;
		curr = pred->GetNext();
		bool marked{ false };

		while (curr->key < x) {
			pred = curr;
			curr = curr->GetNextWithMark(marked);
		}
		return curr->key == x && !marked;
	}

	void clear() {
		while (head.GetNext() != &tail) {
			auto temp = head.GetNext();
			head.next = temp->next;
			delete temp;
		}
	}

	void dump(size_t count) {
		auto ptr = head.GetNext();
		cout << count << " Result : ";
		for (auto i = 0; i < count && ptr != &tail; ++i) {
			cout << ptr->key << " ";
			ptr = ptr->GetNext();
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