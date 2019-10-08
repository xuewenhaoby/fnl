#ifndef __POOL_H__
#define __POOL_H__

#include "task.h"
#include <queue>
#include <pthread.h>
#include <iostream>
using namespace std;

#define POOL_SIZE 1

class ThreadPool{
private:
	static queue<Task*> taskList;
	static int busyNum;
	static pthread_mutex_t mutex;
	static pthread_cond_t cond;
	static pthread_cond_t cond_a;
	static bool shutdown;

	int threadNum;
	pthread_t * threadArray;

protected:
	static void *ThreadFunc(void *arg);
	void create();

public:
	ThreadPool(){ThreadPool(POOL_SIZE);}
	ThreadPool(int _tNum);
	void addTask(Task *task);
	void stopAll();
	int getTaskNum(){return taskList.size();}
	void wait();
};

extern pthread_mutex_t stdio_mutex;
inline void print(string msg)
{
	pthread_mutex_lock(&stdio_mutex);
	cout << msg << endl;
	pthread_mutex_unlock(&stdio_mutex);
}

#endif