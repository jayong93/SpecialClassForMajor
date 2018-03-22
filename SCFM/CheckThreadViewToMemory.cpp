#include <thread>
#include <iostream>

static const int SIZE = 50000000;
volatile int x, y;
int t_x[SIZE], t_y[SIZE];

void th0() {
	for (int i = 0; i < SIZE; ++i) {
		x = i;
		t_y[i] = y;
	}
}

void th1() {
	for (int i = 0; i < SIZE; ++i) {
		y = i;
		t_x[i] = x;
	}
}

int main() {
	std::thread t1{ th0 };
	std::thread t2{ th1 };

	t1.join();
	t2.join();

	int count = 0;
	for (int i = 0; i < SIZE; ++i) {
		if (t_x[i] == t_x[i + 1]) {
			if (t_y[t_x[i]] == t_y[t_x[i] + 1]) {
				if (t_y[t_x[i]] != i) continue;
				++count;
				std::cout << "[" << i << "], ";
			}
		}
	}

	std::cout << "\nTotal Memory Error = " << count << "\n";
	char a;
	std::cin >> a;
}