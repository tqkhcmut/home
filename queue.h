#ifndef queue_h
#define queue_h

#include <iostream>

#define QUEUE_SIZE 20

#define STRING_SIZE 20

typedef char QueueData[STRING_SIZE];

class Queue 
{
	private:
	QueueData QueueBuffer[QUEUE_SIZE];

	int QueueFront;
	int QueueRear;
	int QueueSize;
		
	public:
		Queue();
		char * Dequeue(void);
		int Enqueue(char * string);
		int Size(void);
		int Clear(void);
};
#endif // queue_h
