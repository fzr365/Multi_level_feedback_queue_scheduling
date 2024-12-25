//题目如下，c语言实现
// 多级反馈队列(Multi-leveled feedback queue)调度算法
// 按以下要求实现多级反馈队列调度算法：假设有5个就绪队列，它们的优先级分别为1，2，3，4，5，它们的时间片长度分别为10ms,20ms,40ms,80ms,160ms，即第i个队列的优先级比第i-1个队列要低一级，但是时间片比第i-1个队列的要长一倍。调度算法包括四个部分：主程序main，进程产生器generator，进程调度器函数scheduler，进程运行器函数executor。 
// （1）	主程序：设置好多级队列以及它们的优先级、时间片等信息；创建两个信号量，一个用于generator和executor互斥的访问第1个运行队列(因为产生的新进程都是先放到第1个队列等待执行)，另一个用于generator和scheduler的同步(即，仅当多级队列中还有进程等待运行时，scheduler才能开始执行调度)。创建进程产生器线程，然后调用进程调度器。 
// （2）	进程产生器generator：用线程来实现进程产生器。每隔一个随机的时间段，例如[1,100]ms之间的一个随机数，就产生一个新的进程，创建PCB并填上所有的信息。注意，每个进程所需要运行的时间neededTime在一定范围内(假设为[2,200]ms)内由随机数产生，初始优先级为1。PCB创建完毕后，将其插入到第1个队列进程链表的尾部（要用到互斥信号量，因为executor有可能正好从第1个队列中取出排在队列首的进程来运行）。插入完毕后，generator调用Sleep函数卡睡眠等待随机的一个时间间隔(例如在[1，100]范围产生的1个随机数)，然后再进入下一轮新进程的产生。当创建的进程数量达到预先设定的个数，例如20个，generator就执行完毕退出。
// （3）	进程调度器函数scheduler：在该函数中，依次从第1个队列一直探测到第5个队列，如果第1个队列不为空，则调用执行器executor来执行排在该队列首部的进程。仅当第i号队列为空时，才去调度第i+1个队列的进程。如果时间片用完了但是执行的进程还没有完成（即usedTime<neededTime），则调度器把该进程移动到下一级队列的尾部。当所有的进程都执行完毕，调度器退出，返回主程序。
// （4）	进程执行器executor：根据scheduler传递的队列序号，将该队列进程链表首部的PCB取出，分配该队列对应的时间片给它运行(我们用Sleep函数，睡眠时间长度为该时间片，以模拟该进程得到CPU后的运行期间)。睡眠结束后，所有队列中的进程的等待时间都要加上该时间片。注意，在访问第1个队列时，要使用互斥信号量，以免跟进程产生器generator发生访问冲突。 
// 要求在进程创建时、执行时、进程在队列间移动时、进程运行结束时都输出相应的信息，例如进程的到达时间、进程运行了多长的时间片，进程从被移动到了第几个队列，进程运行结束等信息。

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

//最大进程数
#define MAX_PROCESS 20
//队列实现（需要关联PCB)
struct Queue{
    int priority;
    int timeSlice;
    struct PCB *list;
} queues[5];

//进程控制块
struct PCB{
    char pid[64]; 
    char state;  
    int priority;       
    int arrivalTime;    
    int neededTime;     
    int usedTime;       
    int totalWaitTime;   
    struct PCB *next;   
};


int totalProcess=0;
//锁
pthread_mutex_t mutex;
//总共等待时间
int totalWaitTime=0;
//同步信号量
sem_t shedulerCond; 

//进程产生器
void *generator(void *arg){
      int num=0;
      int curTime=0;
      while(num<MAX_PROCESS){
          //随机生成进程
          struct PCB *newProcess=(struct PCB*)malloc(sizeof(struct PCB));
          sprintf(newProcess->pid,"P%d",num+1);
          newProcess->state='W';
          newProcess->priority=1;
          newProcess->arrivalTime=curTime;
          //生成[2,200]的随机执行时间
          newProcess->neededTime=rand()%199+2;
          newProcess->usedTime=0;
          newProcess->totalWaitTime=0;
          newProcess->next=NULL;


          //将进程插入到第1个队列
          //加锁互斥
          pthread_mutex_lock(&mutex);
          //逻辑：找到第1个队列的最后一个元素，指向新进程
          struct PCB *cur=queues[0].list;
          if(cur==NULL){
              queues[0].list=newProcess;
          }else{
              while(cur->next!=NULL){
                  cur=cur->next;
              }
              cur->next=newProcess;
          }
          
          num++;
          totalProcess++;
          printf("生成器: Process %s 创建成功，达到时间：%d ms，该进程需要的时间： %d ms\n",newProcess->pid,newProcess->arrivalTime,newProcess->neededTime);
          //解锁
          pthread_mutex_unlock(&mutex);    

          //唤醒调度器
          sem_post(&shedulerCond);

          //随机生成[1,100]的时间间隔
          int interval=rand()%100+1;
          curTime+=interval;
          printf("生成器: 生成下一个前等待 %d ms\n",interval);
          //usleep的参数是微秒，所以乘以1000
          usleep(interval*1000);
      }
      return NULL;
}


//进程执行器
void executor(int i,struct PCB *process){
    printf("执行器: 正在执行队列 %d 的进程 %s，优先级：%d，时间片: %d ms\n",i+1,process->pid,queues[i].priority,queues[i].timeSlice);
    //执行进程
    usleep(queues[i].timeSlice*1000);
    //更新进程信息
    process->usedTime+=queues[i].timeSlice;
    //所有队列的进程都需要加上该时间片
    for(int j=0;j<5;j++){
        struct PCB *cnt=queues[j].list;
        while (cnt) 
        {
            cnt->totalWaitTime+=queues[i].timeSlice;
            cnt=cnt->next;
        }
    }
}


//进程调度器
void *scheduler(void *arg){
    //等待来自生成器的信号
    sem_wait(&shedulerCond);
    //调度进程
    while(totalProcess>0){
        //遍历队列
        for(int i=0;i<5;i++){
            //如果队列不为空，则执行进程
            if(queues[i].list!=NULL){
                struct  PCB *cur=queues[i].list;
                queues[i].list=cur->next;
                //执行进程
                executor(i,cur);
                //如果进程还没有执行完毕，则将进程移动到下一个队列
                if(cur->usedTime<cur->neededTime){
                    //将进程移动到下一个队列
                    if(i<4){
                        //和上面一样，找到第i+1个队列的最后一个元素，指向新进程
                        cur->next=NULL;
                        struct PCB *nextfinal=queues[i+1].list;
                        if(nextfinal==NULL){
                            queues[i+1].list=cur;
                        }else{
                            while(nextfinal->next!=NULL){
                                nextfinal=nextfinal->next;
                            }
                            nextfinal->next=cur;
                        }
                        printf("调度器: 进程 %s 移动到队列 %d\n",cur->pid,i+2); 
                    }else{
                    //进程执行完毕
                    totalProcess--;
                    printf("进程 %s 执行完毕",cur->pid);
                    //释放进程
                    free(cur);
                }
                }else{
                    //进程执行完毕
                    totalProcess--;
                    printf("调度器：进程 %s 执行完毕，等待时间:%d ms\n",cur->pid,cur->totalWaitTime);
                    totalWaitTime+=cur->totalWaitTime;
                    //释放进程
                    free(cur);
                }
                break;
            }
        }
    }       
    return NULL;
}



//先实现主函数
int main(){
    //初始化队列(5个优先级以及时间片长度)
    for(int i=0;i<5;i++){
        queues[i].priority=i+1;
        queues[i].timeSlice=10*pow(2,i);
        queues[i].list=NULL;
    }
    
    //初始化锁
    pthread_mutex_init(&mutex,NULL);
    //初始化同步信号量
    sem_init(&shedulerCond,0,0);
    //创建进程产生器线程
    pthread_t generatorThread;
    pthread_t schedulerThread;
    pthread_create(&generatorThread,NULL,generator,NULL);
    //创建进程调度器线程

    pthread_create(&schedulerThread,NULL,scheduler,NULL);
    //等待线程结束
    pthread_join(generatorThread,NULL);
    pthread_join(schedulerThread,NULL);
    //销毁同步信号量
    sem_destroy(&shedulerCond);
    //销毁锁
    pthread_mutex_destroy(&mutex);
    //最后打印平均等待时间
    double averageWaitTime = 1.0 * totalWaitTime / MAX_PROCESS;
    printf("总平均等待时间: %lfms\n", averageWaitTime);
    return 0;
}