**측정 CPU : Intel Core i5 4690 3.50Ghz, 4 Core 4 Thread**

| #Thread | No Lock                          | Mutex                              | Backery                           |
| ------- | -------------------------------- | ---------------------------------- | --------------------------------- |
| 1       | Time: 156ms, <br>Sum = 100000000 | Time : 1345ms, <br>Sum = 100000000 | Time : 278ms, <br>Sum = 100000000 |
| 2       | Time : 285ms, <br>Sum = 50815146 | Time : 1328ms, <br>Sum = 100000000 | Time : 4380ms, <br>Sum = 99500976 |
| 4       | Time : 347ms, <br>Sum = 30381176 | Time : 1330ms, <br>Sum = 100000000 | Time : 8554ms, <br>Sum = 99914640 |
| 8       | Time : 354ms, <br>Sum = 30979790 | Time : 1327ms, <br>Sum = 100000000 | 너무 오래 걸림                     |
| 16      | Time : 335ms, <br>Sum = 38460690 | Time : 1334ms, <br>Sum = 100000000 | 너무 오래 걸림                     |

Backery 알고리즘이 시간이 너무 오래 걸리는 이유는 스레드가 코어 수보다 많고, 실행이 정지된 스레드가 임계영역의 진입 우선순위를 갖는 상태에서 CPU를 차지하는 다른 스레드들이 busy-waiting을 하면서 시간을 잡아먹기 때문으로 추측됨.