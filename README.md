This is a MapReduce library in C utilizing POSIX threads and POSIX synchronization primitives, in particular mutex locks and condition variables. This library supports the execution of user-defined Map and Reduce functions on a multicore system.

### Threadpool Synchronization Primatives
The threadpool library uses locks and condition variables. The lock controls critical sections that access or modify the jobs queue. In threadpool_run, the lock needs to aquired first before the thread removes the first job from the job queue and executes it. In thread_pool add, the lock also needs to be aquired first before the thread adds a job to the job queue. Lastly, threadpool_check uses a lock to safely read both the job queue size and idle thread count to prevent worker threads from modifying these values concurrently.

### Mapreduce Synchronization Primatives
The partitions array in mapreduce uses a lock for each partition. This lock controls critical sections that modify the partition. This is to ensure that only one thread is modifying the partition at one time.  
The lock is not needed for accessing the partition, because during reduce only one thread will be accessing each partition and no synchronization si needed.

### Partition Implementation
The partitions are implemented as an array of partition structs. Each partition struct contains a lock and a linked list of keyvalue structs. A linked list was used for its simple inserts that keeps the list sorted. It also grows as needed - there is no need for dynamically allocating memory to accomodate the growing size. However, each insertion would be O(N) time complexity.

### Threadpool Job Queue Implementation
The threadpool job queue is also implemented as a linked list. Since jobs are submitted in ascending size order, insertions always happen at the tail, making the insertion time complexity O(1). Removing only happens at the head, so the deletion time complexity is also O(1). This makes insertion and deletion super efficient with a linked list data structure, in addition to other benefits like the simplicity of growing and shrinking the queue.

#### Resources
https://www.programiz.com/c-programming/library-function/string.h/strcmp  
https://stackoverflow.com/questions/21091000/how-to-get-thread-id-of-a-pthread-in-linux-c-program  
https://stackoverflow.com/questions/5134891/how-do-i-use-valgrind-to-find-memory-leaks  
https://www.geeksforgeeks.org/cpp/strdup-strdndup-functions-c/  
https://stackoverflow.com/questions/34219186/what-is-the-difference-between-and-in-a-makefile  
https://stackoverflow.com/questions/5362577/c-gettimeofday-for-computing-time  
https://man7.org/linux/man-pages/man3/clock_gettime.3.html  
https://www.geeksforgeeks.org/c/qsort-function-in-c/  
