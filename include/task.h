#ifndef __TASK_H__
#define __TASK_H__

void *func1(void *);

void HandleNodeEvent(void *arg);
void HandleTimerEvent(void *arg);

void HandleSatEvent(void *arg);
void HandleHostEvent(void *arg);

enum TaskType{
	TASK_Node,
	TASK_Timer,
};

class Task{
public:
	Task() = default;
	virtual ~Task(){}
	virtual int Run() = 0;
	void setData(void *data){taskData = data;}

protected:
	void *taskData;
};

class NodeTask:public Task{
public:
	NodeTask(void *data){setData(data);}
	int Run(){
		HandleNodeEvent(taskData);
		return 0;
	}
};

class TimerTask:public Task{
public:
	TimerTask(void *data){setData(data);}
	int Run(){
		HandleTimerEvent(taskData);
		return 0;
	}
};

#define TASK_FREE(m) delete m

#endif