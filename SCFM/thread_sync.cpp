#include <iostream>
#include <thread>
#include <mutex>

int data;
bool flag = false;

void thread_recv() {
    while (false == flag);
    std::cout << "I received [" << data << "]\n";
}

void thread_send() {
    data = 999;
    flag = true;
    std::cout << "I have sent [" << data << "]\n";
    std::cout << "Flag is [" << flag << "]\n";
}

int main() {
    std::thread rt{ thread_recv };
    std::thread st{ thread_send };
    rt.join(); st.join();
    char r;
    std::cin >> r;
}