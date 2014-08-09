#include "queue.h"

void Queue::Init(void)
{
	QueueFront = 0;
	QueueRear = 0;
	QueueSize = 0;
}
char * Queue::Dequeue(void)
{
	if (QueueSize == 0)
		return "";
	QueueRear++;
	if (QueueRear == QUEUE_SIZE)
		QueueRear = 0;
	QueueSize--;
	return QueueBuffer[(QueueRear == 0) ? QUEUE_SIZE-1 : QueueRear-1];
}
int Queue::Enqueue(char * string)
{
	int string_length = 0;
	int queue_index = 0;
	int i = 0;
	if (QueueSize == QUEUE_SIZE)
		return 0;
	while(*string++ != 0)
		string_length++;
	if (string_length > STRING_SIZE)
		return -1;
	QueueFront++;
	if (QueueFront == QUEUE_SIZE)
		QueueFront = 0;
	queue_index = (QueueFront == 0) ? QUEUE_SIZE-1 : QueueFront-1;
	for (i = 0; i < string_length; i++)
		QueueBuffer[queue_index][i] = string[i];
	return 1;
}
int Queue::Size(void)
{
	return QueueSize;
}
