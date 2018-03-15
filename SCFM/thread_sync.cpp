#include <iostream>
#include <thread>
#include <mutex>

int data;
bool flag = false;
std::mutex lock;

void thread_recv() {
    lock.lock();
    bool myFlag = flag;
    lock.unlock();
    while (false == myFlag) {
        lock.lock(); myFlag = flag; lock.unlock();
    }
    lock.lock();
    int myData = data;
    lock.unlock();
    std::cout << "I received [" << myData << "]\n";
}

void thread_send() {
    lock.lock();
    data = 999;
    flag = true;
    lock.unlock();
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