#pragma once

extern void create_context(char **stack, void (*const start)(void *arg));

extern void *swap_context(char **cur_stack, char *dest_stack, void *arg);
