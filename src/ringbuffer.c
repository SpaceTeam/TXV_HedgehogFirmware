#include "ringbuffer.h"
#include <string.h>


inline void memcopy(volatile uint8_t *dst, volatile uint8_t *src, size_t n)
{
	size_t i;
	for(i=0; i<n; i++)
	{
		*(dst + i) = *(src + i);
	}
}

inline rb_return_t ringbuffer_init(ringbuffer_t *rb, volatile uint8_t *buf, size_t bufSize, void (*startReadFunction)(void))
{
	if((bufSize - 1) & bufSize) return RB_ERROR_CONFIG;

	rb->size = bufSize;
	rb->buffer = buf;
	rb->head = 0;
	rb->tail = 0;
	rb->startReadFunction = startReadFunction;

	return RB_SUCCESS;
}


inline size_t ringbuffer_getFree(ringbuffer_t *rb)
{
	return rb->size - ringbuffer_getFilled(rb);
}

inline size_t ringbuffer_getFilled(ringbuffer_t *rb)
{
	return rb->head - rb->tail;
}


inline rb_return_t ringbuffer_push(ringbuffer_t *rb, uint8_t value)
{
	if(ringbuffer_getFree(rb) == 0) return RB_ERROR_OVERFLOW;

	rb->buffer[TRUNC(rb->head)] = value;
	rb->head++;

	if(rb->startReadFunction != NULL) //start-read-function is specified
		(rb->startReadFunction)(); //execute start-read-function

	return RB_SUCCESS;
}

inline rb_return_t ringbuffer_push_multiple(ringbuffer_t *rb, volatile uint8_t *data, size_t n)
{
	if(ringbuffer_getFree(rb) < n) return RB_ERROR_OVERFLOW;

	if((TRUNC(rb->head) + n) <= rb->size) //no wrap
	{
		memcopy(&rb->buffer[TRUNC(rb->head)], data, n); //copy data
	}
	else //wrap
	{
		size_t length = rb->size - TRUNC(rb->head); //number of values before wrap
		memcopy(&rb->buffer[TRUNC(rb->head)], data, length); //copy data before wrap
		memcopy(&rb->buffer[0], &data[length], n - length); //copy data after wrap
	}
	
	rb->head += n;

	if(rb->startReadFunction != NULL) //start-read-function is specified
		(rb->startReadFunction)(); //execute start-read-function

	return RB_SUCCESS;
}


inline rb_return_t ringbuffer_peek(ringbuffer_t *rb, volatile uint8_t *data)
{
	if(ringbuffer_getFilled(rb) == 0) return RB_ERROR_UNDERFLOW;
	memcopy(data, &rb->buffer[TRUNC(rb->tail)], 1);
	return RB_SUCCESS;
}

inline rb_return_t ringbuffer_peek_multiple(ringbuffer_t *rb, volatile uint8_t *data, size_t n)
{
	return ringbuffer_peek_at_multiple(rb, rb->tail, data, n);
}

inline rb_return_t ringbuffer_peek_at(ringbuffer_t *rb, size_t position, volatile uint8_t *data)
{
	if(!IS_BETWEEN(TRUNC(position), TRUNC(rb->tail), TRUNC(rb->head))) return RB_ERROR_UNDERFLOW; //position is not between tail and head
	memcopy(data, &rb->buffer[TRUNC(position)], 1);
	return RB_SUCCESS;
}

inline rb_return_t ringbuffer_peek_at_multiple(ringbuffer_t *rb, size_t position, volatile uint8_t *data, size_t n)
{
	//if not all data is between tail and head
	if(!IS_BETWEEN(TRUNC(position), TRUNC(rb->tail), TRUNC(rb->head))) return RB_ERROR_UNDERFLOW;
	if(!IS_BETWEEN(TRUNC(position + n), TRUNC(rb->tail), TRUNC(rb->head))) return RB_ERROR_UNDERFLOW;

	if((TRUNC(position) + n) <= rb->size) //no wrap
	{
		memcopy(data, &rb->buffer[TRUNC(position)], n); //copy data
	}
	else //wrap
	{
		size_t length = rb->size - TRUNC(position); //number of values before wrap
		memcopy(data, &rb->buffer[TRUNC(position)], length); //copy data before wrap
		memcopy(&data[length], &rb->buffer[0], n - length); //copy data after wrap
	}
	return RB_SUCCESS;
}

inline rb_return_t ringbuffer_peek_relative(ringbuffer_t *rb, size_t offset, volatile uint8_t *data)
{
	return ringbuffer_peek_at(rb, (rb->tail + offset), data);
}

inline rb_return_t ringbuffer_peek_relative_multiple(ringbuffer_t *rb, size_t offset, volatile uint8_t *data, size_t n)
{
	return ringbuffer_peek_at_multiple(rb, rb->tail + offset, data, n);
}


inline rb_return_t ringbuffer_consume(ringbuffer_t *rb, size_t n)
{
	if(ringbuffer_getFilled(rb) < n) return RB_ERROR_UNDERFLOW;
	rb->tail += n;
	return RB_SUCCESS;
}


inline rb_return_t ringbuffer_pop(ringbuffer_t *rb, volatile uint8_t *data)
{
	uint8_t errcode = ringbuffer_peek(rb, data);
	if(errcode != RB_SUCCESS) return errcode;
	return ringbuffer_consume(rb, 1);
}

inline rb_return_t ringbuffer_pop_multiple(ringbuffer_t *rb, volatile uint8_t *data, size_t n)
{
	uint8_t errcode = ringbuffer_peek_multiple(rb, data, n);
	if(errcode != RB_SUCCESS) return errcode;
	return ringbuffer_consume(rb, n);
}


inline rb_return_t ringbuffer_peek_multiple_trans(ringbuffer_t *rb, ringbuffer_t *dst, size_t n)
{
	return ringbuffer_peek_at_multiple_trans(rb, rb->tail, dst, n);
}

inline rb_return_t ringbuffer_peek_at_multiple_trans(ringbuffer_t *rb, size_t position, ringbuffer_t *dst, size_t n)
{
	//if not all data is between tail and head
	if(!IS_BETWEEN(TRUNC(position), TRUNC(rb->tail), TRUNC(rb->head))) return RB_ERROR_UNDERFLOW;
	if(!IS_BETWEEN(TRUNC(position + n), TRUNC(rb->tail), TRUNC(rb->head))) return RB_ERROR_UNDERFLOW;
	if(ringbuffer_getFree(dst) < n) return RB_ERROR_OVERFLOW;

	if((TRUNC(position) + n) <= rb->size) //no wrap
	{
		uint8_t errcode = ringbuffer_push_multiple(dst, &rb->buffer[TRUNC(position)], n); //copy data
		if(errcode != RB_SUCCESS) return errcode;
	}
	else //wrap
	{
		size_t length = rb->size - TRUNC(position); //number of values before wrap
		uint8_t errcode = ringbuffer_push_multiple(dst, &rb->buffer[TRUNC(position)], length); //copy data before wrap
		if(errcode != RB_SUCCESS) return errcode;
		errcode = ringbuffer_push_multiple(dst, &rb->buffer[0], n - length); //copy data after wrap
		if(errcode != RB_SUCCESS) return errcode;
	}

	return RB_SUCCESS;
}

inline rb_return_t ringbuffer_peek_relative_multiple_trans(ringbuffer_t *rb, size_t offset, ringbuffer_t *dst, size_t n)
{
	return ringbuffer_peek_at_multiple_trans(rb, rb->tail + offset, dst, n);
}

inline rb_return_t ringbuffer_pop_multiple_trans(ringbuffer_t *rb, ringbuffer_t *dst, size_t n)
{
	uint8_t errcode = ringbuffer_peek_multiple_trans(rb, dst, n);
	if(errcode != RB_SUCCESS) return errcode;
	return ringbuffer_consume(rb, n);
}
