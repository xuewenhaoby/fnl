#include "thread_pool.h"
#include <unistd.h>

queue<Task*> ThreadPool::taskList;
int ThreadPool::busyNum = 0;
bool ThreadPool::shutdown = false;
pthread_mutex_t ThreadPool::mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ThreadPool::cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t ThreadPool::cond_a = PTHREAD_COND_INITIALIZER;

pthread_mutex_t stdio_mutex = PTHREAD_MUTEX_INITIALIZER;

void *ThreadPool::ThreadFunc(void *arg)
{
	//pthread_t tid = pthread_self();
	
	while(1){
		pthread_mutex_lock(&mutex);
		while(taskList.empty() && !shutdown){
			pthread_cond_wait(&cond,&mutex);
		}

		if(shutdown){
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}

		Task *task = taskList.front();
		taskList.pop();
		busyNum++;

		pthread_mutex_unlock(&mutex);

		task->Run();
		TASK_FREE(task);

		pthread_mutex_lock(&mutex);
		busyNum--;
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cond_a);
	}

	return (void*)0;
}

void ThreadPool::create()
{
	threadArray = new pthread_t[threadNum];
	for(int i = 0; i < threadNum; i++){
		pthread_create(&threadArray[i],NULL,ThreadFunc,NULL);
	}
}

ThreadPool::ThreadPool(int threadNum)
{
	this->threadNum = threadNum;
	create();
}

void ThreadPool::addTask(Task *task)
{
	pthread_mutex_lock(&mutex);
	taskList.push(task);
	pthread_mutex_unlock(&mutex);
	pthread_cond_signal(&cond);
}

void ThreadPool::stopAll()
{
	// Avoid that the func be called repeatly.
	if(shutdown) return;

	shutdown = true;
	pthread_cond_broadcast(&cond);

	for(int i = 0; i < threadNum; i++){
		pthread_join(threadArray[i],NULL);
	}

	delete[] threadArray;
	threadArray = NULL;

	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	pthread_cond_destroy(&cond_a);
}

void ThreadPool::wait()
{
	sleep(1); // avoid this func get the mutex before task_threads
	pthread_mutex_lock(&mutex);
	while(busyNum > 0){
		pthread_cond_wait(&cond_a,&mutex);
	}
	pthread_mutex_unlock(&mutex);
}
