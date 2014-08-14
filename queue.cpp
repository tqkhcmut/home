#include "queue.h"

Queue::Queue(void)
{
	Queue::Clear();
}

int Queue::Clear(void)
{
	int i,j;
	Queue::QueueFront = 0;
	Queue::QueueRear = 0;
	Queue::QueueSize = 0;
	for (i = 0; i < QUEUE_SIZE; i++)
	{
		for (j = 0; j < STRING_SIZE; j++)
		{
			Queue::QueueBuffer[i][j] = 0;
		}
	}
	return 1;
}

char * Queue::Dequeue(void)
{
	int i; 
	if (Queue::QueueSize == 0)
		return (char*)"";
	Queue::QueueRear++;
	if (Queue::QueueRear == QUEUE_SIZE)
		Queue::QueueRear = 0;
	Queue::QueueSize--;
	
	return Queue::QueueBuffer[(Queue::QueueRear == 0) ? QUEUE_SIZE-1 : Queue::QueueRear-1];
}
int Queue::Enqueue(char * string)
{
	int string_length = 0;
	int queue_index = 0;
	int i = 0;
	if (Queue::QueueSize == QUEUE_SIZE)
		return 0;
	while(*(string+i++) != 0)
		string_length++;
	if (string_length > STRING_SIZE-1)
		return -1;
	Queue::QueueFront++;
	if (Queue::QueueFront == QUEUE_SIZE)
		Queue::QueueFront = 0;
	queue_index = (Queue::QueueFront == 0) ? QUEUE_SIZE-1 : Queue::QueueFront-1;
	for (i = 0; i < string_length; i++)
		Queue::QueueBuffer[queue_index][i] = string[i];
	Queue::QueueBuffer[queue_index][string_length] = 0;
	Queue::QueueSize++;
	
	return 1;
}
int Queue::Size(void)
{
	return Queue::QueueSize;
}
