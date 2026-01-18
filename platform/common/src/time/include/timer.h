#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint64_t expiry_ms;
} mstimer_t;

typedef struct {
    uint64_t expiry_sec;
} sectimer_t;

/* Millisecond timers */
void mstimer_start(mstimer_t *t, uint32_t timeout_ms);
bool mstimer_expired(const mstimer_t *t);
uint64_t mstimer_remaining(const mstimer_t *t);

/* Second timers */
void sectimer_start(sectimer_t *t, uint32_t timeout_sec);
bool sectimer_expired(const sectimer_t *t);
uint32_t sectimer_remaining(const sectimer_t *t);

