# Grovyle - URM simulator

## Description

This is a software that simulates "unlimited register machine".
The software itself doesn't have unlimited reigsters (that's impossible).

## Simulation limitations

* The highest register index is defined in C source (`#define REGISTER_AMOUNT (1u << 8)`).
* All register values are stored as an unsigned 64 bit value (`uint64_t`).
