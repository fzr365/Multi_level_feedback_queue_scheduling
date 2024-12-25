# Multi_level_feedback_queue_scheduling
You can learn to implement a multi-level feedback scheduling algorithm using C language.


假设有5个运行队列，它们的优先级分别为1，2，3，4，5，它们的时间片长度分别为10ms,20ms,40ms,80ms,160ms，即第i个队列的优先级比第i-1个队列要低一级，但是时间片比第i-1个队列的要长一倍。多级反馈队列调度算法包括四个部分：主程序main，进程产生器generator，进程调度器函数scheduler，进程运行器函数executor。结果输出：在进程创建、插入队列、执行时的相关信息，并计算输出总的平均等待时间。其中，generator用线程来实现，每隔一个随机时间(例如在[1,100]ms之间)产生一个新的进程PCB，并插入到第1个队列的进程链表尾部。Scheduler依次探测每个队列，寻找进程链表不为空的队列，然后调用Executor, executor把该队列进程链表首部的进程取出来执行。要设置1个互斥信号量来实现对第1个队列的互斥访问，因为generator和executor有可能同时对第1个队列进行操作。 同时要设置1个同步信号量，用于generator和scheduler的同步：generator每产生1个新进程，就signal一次这个同步信号量；只有所有队列不为空时，scheduler才会运行，否则scheduler要等待这个同步信号量。当所有进程运行完毕后，scheduler退出，主程序结束。
