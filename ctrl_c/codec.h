#pragma once
#include <stdint.h>

size_t cobs_encode(const uint8_t *ptr, size_t length, uint8_t *dst);
size_t cobs_decode(const uint8_t *ptr, size_t length, uint8_t *dst);
