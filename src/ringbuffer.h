#ifndef RINGBUFFER_H_
#define RINGBUFFER_H_


#include <stdint.h>
#include <stddef.h>


#define TRUNC(i) ((i) & (rb->size - 1))
#define IS_BETWEEN(x,a,b) ((x)>(a) && (x)<(b))


typedef enum rb_return_e
{
	RB_SUCCESS = 0,
	RB_ERROR_CONFIG = 1,
	RB_ERROR_OVERFLOW = 2,
	RB_ERROR_UNDERFLOW = 3
} rb_return_t;

typedef struct
{
    size_t size;
    volatile uint8_t *buffer;
    volatile size_t head;
    volatile size_t tail;
    void (*startReadFunction)(void);
} ringbuffer_t;


//initialize ringbuffer, bufSize must be a power of 2, bigger than 0 and smaller than MAX_BUFFER_SIZE
rb_return_t ringbuffer_init(volatile ringbuffer_t *rb, volatile uint8_t *buf, size_t bufSize, void (*startReadFunction)(void));

//get the number of free slots
size_t ringbuffer_getFree(volatile ringbuffer_t *rb);
//get the number of filled slots
size_t ringbuffer_getFilled(volatile ringbuffer_t *rb);

//push functions, make sure there is at least one free slot!
//push value into ringbuffer
rb_return_t ringbuffer_push(volatile ringbuffer_t *rb, uint8_t value);
//push multiple values into ringbuffer
rb_return_t ringbuffer_push_multiple(volatile ringbuffer_t *rb, volatile uint8_t *data, size_t n);

//peek functions, does not modify the ringbuffer, make sure the buffer is not empty!
//peek at next value in ringbuffer
rb_return_t ringbuffer_peek(volatile ringbuffer_t *rb, volatile uint8_t *data);
//peek at next multiple values in ringbuffer
rb_return_t ringbuffer_peek_multiple(volatile ringbuffer_t *rb, volatile uint8_t *data, size_t n);
//peek at value at specific position in ringbuffer
rb_return_t ringbuffer_peek_at(volatile ringbuffer_t *rb, size_t position, volatile uint8_t *data);
//peek at multiple values at specific position in ringbuffer
rb_return_t ringbuffer_peek_at_multiple(volatile ringbuffer_t *rb, size_t position, volatile uint8_t *data, size_t n);
//peek at value at specific offset in ringbuffer
rb_return_t ringbuffer_peek_relative(volatile ringbuffer_t *rb, size_t offset, volatile uint8_t *data);
//peek at multiple values at specific offset in ringbuffer
rb_return_t ringbuffer_peek_relative_multiple(volatile ringbuffer_t *rb, size_t offset, volatile uint8_t *data, size_t n);

//delete n values
rb_return_t ringbuffer_consume(volatile ringbuffer_t *rb, size_t n);

//pop functions, make sure the buffer is not empty!
//pop value out of buffer
rb_return_t ringbuffer_pop(volatile ringbuffer_t *rb, volatile uint8_t *data);
//pop multiple values out of buffer
rb_return_t ringbuffer_pop_multiple(volatile ringbuffer_t *rb, volatile uint8_t *data, size_t n);

//transfer functions, ringbuffer as the data destination, make sure the buffer is not empty and the destination buffer is not full!
//peek at next multiple values in ringbuffer *rb and push data to destination ringbuffer *dst
rb_return_t ringbuffer_peek_multiple_trans(volatile ringbuffer_t *rb, volatile ringbuffer_t *dst, size_t n);
//peek at multiple values at specific position in ringbuffer *rb and push data to destination ringbuffer *dst
rb_return_t ringbuffer_peek_at_multiple_trans(volatile ringbuffer_t *rb, size_t position, volatile ringbuffer_t *dst, size_t n);
//peek at multiple values at value at specific offset in ringbuffer *rb and push data to destination ringbuffer *dst
rb_return_t ringbuffer_peek_relative_multiple_trans(volatile ringbuffer_t *rb, size_t offset, volatile ringbuffer_t *dst, size_t n);
//pop multiple values out of buffer push data to destination ringbuffer *dst
rb_return_t ringbuffer_pop_multiple_trans(volatile ringbuffer_t *rb, volatile ringbuffer_t *dst, size_t n);

#endif /* RINGBUFFER_H_ */
