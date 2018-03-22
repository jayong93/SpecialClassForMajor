#include <thread>
#include <iostream>
#include <atomic>

volatile bool done{ false };
volatile int *bound;
int error{ 0 };

void th0() {
	for (int i = 0; i < 25000000; ++i) *bound = -(1 + *bound);
	done = true;
}

void th1() {
	while (!done) {
		int temp = *bound;
		if ((temp != 0) && (temp != -1))
		{
			++error;
			// 0�� -1�� �߰� ������ ���´�.
			std::cout << std::hex << "[" << temp << "], ";
		}
	}
}

int main() {
	int arr[32];
	int temp = reinterpret_cast<int>(arr+16);
	// temp�� 64�� ����� �ǵ��� ���� ��, 2�� ��
	// 64�� cache line ������.
	temp = (temp / 64) * 64;
	temp -= 2;

	bound = reinterpret_cast<int*>(temp);
	*bound = 0;

	std::thread t1{ th0 };
	std::thread t2{ th1 };

	t1.join();
	t2.join();

	std::cout << std::dec << "\nTotal Memory Error = " << error << "\n";
	system("pause");
}