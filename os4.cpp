//��Ŀ���£�c����ʵ��
// �༶��������(Multi-leveled feedback queue)�����㷨
// ������Ҫ��ʵ�ֶ༶�������е����㷨��������5���������У����ǵ����ȼ��ֱ�Ϊ1��2��3��4��5�����ǵ�ʱ��Ƭ���ȷֱ�Ϊ10ms,20ms,40ms,80ms,160ms������i�����е����ȼ��ȵ�i-1������Ҫ��һ��������ʱ��Ƭ�ȵ�i-1�����е�Ҫ��һ���������㷨�����ĸ����֣�������main�����̲�����generator�����̵���������scheduler����������������executor�� 
// ��1��	���������úö༶�����Լ����ǵ����ȼ���ʱ��Ƭ����Ϣ�����������ź�����һ������generator��executor����ķ��ʵ�1�����ж���(��Ϊ�������½��̶����ȷŵ���1�����еȴ�ִ��)����һ������generator��scheduler��ͬ��(���������༶�����л��н��̵ȴ�����ʱ��scheduler���ܿ�ʼִ�е���)���������̲������̣߳�Ȼ����ý��̵������� 
// ��2��	���̲�����generator�����߳���ʵ�ֽ��̲�������ÿ��һ�������ʱ��Σ�����[1,100]ms֮���һ����������Ͳ���һ���µĽ��̣�����PCB���������е���Ϣ��ע�⣬ÿ����������Ҫ���е�ʱ��neededTime��һ����Χ��(����Ϊ[2,200]ms)�����������������ʼ���ȼ�Ϊ1��PCB������Ϻ󣬽�����뵽��1�����н��������β����Ҫ�õ������ź�������Ϊexecutor�п������ôӵ�1��������ȡ�����ڶ����׵Ľ��������У���������Ϻ�generator����Sleep������˯�ߵȴ������һ��ʱ����(������[1��100]��Χ������1�������)��Ȼ���ٽ�����һ���½��̵Ĳ������������Ľ��������ﵽԤ���趨�ĸ���������20����generator��ִ������˳���
// ��3��	���̵���������scheduler���ڸú����У����δӵ�1������һֱ̽�⵽��5�����У������1�����в�Ϊ�գ������ִ����executor��ִ�����ڸö����ײ��Ľ��̡�������i�Ŷ���Ϊ��ʱ����ȥ���ȵ�i+1�����еĽ��̡����ʱ��Ƭ�����˵���ִ�еĽ��̻�û����ɣ���usedTime<neededTime������������Ѹý����ƶ�����һ�����е�β���������еĽ��̶�ִ����ϣ��������˳�������������
// ��4��	����ִ����executor������scheduler���ݵĶ�����ţ����ö��н��������ײ���PCBȡ��������ö��ж�Ӧ��ʱ��Ƭ��������(������Sleep������˯��ʱ�䳤��Ϊ��ʱ��Ƭ����ģ��ý��̵õ�CPU��������ڼ�)��˯�߽��������ж����еĽ��̵ĵȴ�ʱ�䶼Ҫ���ϸ�ʱ��Ƭ��ע�⣬�ڷ��ʵ�1������ʱ��Ҫʹ�û����ź�������������̲�����generator�������ʳ�ͻ�� 
// Ҫ���ڽ��̴���ʱ��ִ��ʱ�������ڶ��м��ƶ�ʱ���������н���ʱ�������Ӧ����Ϣ��������̵ĵ���ʱ�䡢���������˶೤��ʱ��Ƭ�����̴ӱ��ƶ����˵ڼ������У��������н�������Ϣ��

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <math.h>

//��������
#define MAX_PROCESS 20
//����ʵ�֣���Ҫ����PCB)
struct Queue{
    int priority;
    int timeSlice;
    struct PCB *list;
} queues[5];

//���̿��ƿ�
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
//��
pthread_mutex_t mutex;
//�ܹ��ȴ�ʱ��
int totalWaitTime=0;
//ͬ���ź���
sem_t shedulerCond; 

//���̲�����
void *generator(void *arg){
      int num=0;
      int curTime=0;
      while(num<MAX_PROCESS){
          //������ɽ���
          struct PCB *newProcess=(struct PCB*)malloc(sizeof(struct PCB));
          sprintf(newProcess->pid,"P%d",num+1);
          newProcess->state='W';
          newProcess->priority=1;
          newProcess->arrivalTime=curTime;
          //����[2,200]�����ִ��ʱ��
          newProcess->neededTime=rand()%199+2;
          newProcess->usedTime=0;
          newProcess->totalWaitTime=0;
          newProcess->next=NULL;


          //�����̲��뵽��1������
          //��������
          pthread_mutex_lock(&mutex);
          //�߼����ҵ���1�����е����һ��Ԫ�أ�ָ���½���
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
          printf("������: Process %s �����ɹ����ﵽʱ�䣺%d ms���ý�����Ҫ��ʱ�䣺 %d ms\n",newProcess->pid,newProcess->arrivalTime,newProcess->neededTime);
          //����
          pthread_mutex_unlock(&mutex);    

          //���ѵ�����
          sem_post(&shedulerCond);

          //�������[1,100]��ʱ����
          int interval=rand()%100+1;
          curTime+=interval;
          printf("������: ������һ��ǰ�ȴ� %d ms\n",interval);
          //usleep�Ĳ�����΢�룬���Գ���1000
          usleep(interval*1000);
      }
      return NULL;
}


//����ִ����
void executor(int i,struct PCB *process){
    printf("ִ����: ����ִ�ж��� %d �Ľ��� %s�����ȼ���%d��ʱ��Ƭ: %d ms\n",i+1,process->pid,queues[i].priority,queues[i].timeSlice);
    //ִ�н���
    usleep(queues[i].timeSlice*1000);
    //���½�����Ϣ
    process->usedTime+=queues[i].timeSlice;
    //���ж��еĽ��̶���Ҫ���ϸ�ʱ��Ƭ
    for(int j=0;j<5;j++){
        struct PCB *cnt=queues[j].list;
        while (cnt) 
        {
            cnt->totalWaitTime+=queues[i].timeSlice;
            cnt=cnt->next;
        }
    }
}


//���̵�����
void *scheduler(void *arg){
    //�ȴ��������������ź�
    sem_wait(&shedulerCond);
    //���Ƚ���
    while(totalProcess>0){
        //��������
        for(int i=0;i<5;i++){
            //������в�Ϊ�գ���ִ�н���
            if(queues[i].list!=NULL){
                struct  PCB *cur=queues[i].list;
                queues[i].list=cur->next;
                //ִ�н���
                executor(i,cur);
                //������̻�û��ִ����ϣ��򽫽����ƶ�����һ������
                if(cur->usedTime<cur->neededTime){
                    //�������ƶ�����һ������
                    if(i<4){
                        //������һ�����ҵ���i+1�����е����һ��Ԫ�أ�ָ���½���
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
                        printf("������: ���� %s �ƶ������� %d\n",cur->pid,i+2); 
                    }else{
                    //����ִ�����
                    totalProcess--;
                    printf("���� %s ִ�����",cur->pid);
                    //�ͷŽ���
                    free(cur);
                }
                }else{
                    //����ִ�����
                    totalProcess--;
                    printf("������������ %s ִ����ϣ��ȴ�ʱ��:%d ms\n",cur->pid,cur->totalWaitTime);
                    totalWaitTime+=cur->totalWaitTime;
                    //�ͷŽ���
                    free(cur);
                }
                break;
            }
        }
    }       
    return NULL;
}



//��ʵ��������
int main(){
    //��ʼ������(5�����ȼ��Լ�ʱ��Ƭ����)
    for(int i=0;i<5;i++){
        queues[i].priority=i+1;
        queues[i].timeSlice=10*pow(2,i);
        queues[i].list=NULL;
    }
    
    //��ʼ����
    pthread_mutex_init(&mutex,NULL);
    //��ʼ��ͬ���ź���
    sem_init(&shedulerCond,0,0);
    //�������̲������߳�
    pthread_t generatorThread;
    pthread_t schedulerThread;
    pthread_create(&generatorThread,NULL,generator,NULL);
    //�������̵������߳�

    pthread_create(&schedulerThread,NULL,scheduler,NULL);
    //�ȴ��߳̽���
    pthread_join(generatorThread,NULL);
    pthread_join(schedulerThread,NULL);
    //����ͬ���ź���
    sem_destroy(&shedulerCond);
    //������
    pthread_mutex_destroy(&mutex);
    //����ӡƽ���ȴ�ʱ��
    double averageWaitTime = 1.0 * totalWaitTime / MAX_PROCESS;
    printf("��ƽ���ȴ�ʱ��: %lfms\n", averageWaitTime);
    return 0;
}