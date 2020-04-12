#pragma once

extern void create_context(void **stack, void (*start)(void *));

extern void *swap_context(void **cur_stack, void *dest_stack, void *arg);