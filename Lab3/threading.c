#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <string.h>
#include <stdint.h>

#include "threading.h"

static ucontext_t main_ctx;
static int g_inited = 0;

//This function will find an available slot in the contexts array and will return its index. If no slot is available, it returns -1.
static int find_available_slot(void) {
	for (int i = 0; i < NUM_CTX; i++) {
		if (contexts[i].state != VALID) {  // Found an available slot
			return i;
		}
	}
	return -1;
}

//This will select the next thread to run in a round-robin fashion, starting from the index after the current one. If no valid thread is found, it returns -1.
static int select_next_thread(int current) {
	int start = (current + 1) % NUM_CTX;
	for (int i = 0; i < NUM_CTX; i++) {
		int idx = (start + i) % NUM_CTX;
		if (contexts[idx].state == VALID) {
			return idx;
		}
	}
	return -1;
}

//This will count the number of active threads (those in VALID state) and return that count.
static int count_active_threads(void) {
	int active = 0;
	for (int i = 0; i < NUM_CTX; i++) {
		if (contexts[i].state == VALID) {
			active++;
		}
	}
	return active;
}

//This function performs the actual context switch from one thread to another using swapcontext. It updates the current_context_idx accordingly.
static void perform_context_switch(int from_idx, int to_idx) {
	uint8_t old_idx = current_context_idx;
	current_context_idx = (uint8_t)to_idx;
	
	if (from_idx == NUM_CTX) {
		swapcontext(&main_ctx, &contexts[to_idx].context);
	} else {
		swapcontext(&contexts[from_idx].context, &contexts[to_idx].context);
	}
	
	current_context_idx = old_idx;
}

//This function initializes the threading system by setting all context states to INVALID.
void t_init() {
	for (int i = 0; i < NUM_CTX; i++) {
		contexts[i].state = INVALID;
		memset(&contexts[i].context, 0, sizeof(ucontext_t));
	}
	current_context_idx = NUM_CTX; // Indicate we're in main context
	g_inited = 1;
}

//This function creates a new thread context for the provided function and its arguments. It allocates a stack, sets up the context, and marks it as VALID.
int32_t t_create(fptr foo, int32_t arg1, int32_t arg2) {
	if (!g_inited) t_init();
	
	int slot = find_available_slot(); // Find an available slot
	if (slot < 0) return 1;

	void *stack = malloc(STK_SZ); // Allocate stack for the new context
	if (!stack) return 1;
	
	ucontext_t *uc = &contexts[slot].context;
	if (getcontext(uc) == -1) { // Error getting context
		free(stack);
		return 1;
	}

	uc->uc_stack.ss_sp = stack;
	uc->uc_stack.ss_size = STK_SZ;
	uc->uc_stack.ss_flags = 0;
	uc->uc_link = NULL; // Set to NULL as required by lab spec
	makecontext(uc, (ctx_ptr)foo, 2, arg1, arg2); 

	contexts[slot].state = VALID;
	return 0;
}

//This function yields execution from the current thread to the next available thread. It returns the number of active threads after the yield.
int32_t t_yield() {
	int active = count_active_threads();
	if (active == 0) return 0;
	
	int from = (current_context_idx < NUM_CTX) ? current_context_idx : NUM_CTX;
	int to = select_next_thread(from == NUM_CTX ? -1 : from); // If yielding from main, start search from -1
	
	if (to < 0) return -1;
	
	if (from == NUM_CTX) { // If yielding from main, save its context
		if (getcontext(&main_ctx) == -1) return -1;
	}
	
	perform_context_switch(from, to);
	
	return count_active_threads();
}

//This function marks the current thread as DONE, frees its stack, and switches to the next available thread or back to main if none are left.
void t_finish() {
	if (current_context_idx >= NUM_CTX) { // If called from main, just return
		return;
	}

	int my_idx = current_context_idx; // Index of the current context
	
	if (contexts[my_idx].context.uc_stack.ss_sp) {
		free(contexts[my_idx].context.uc_stack.ss_sp); // Free the stack
		contexts[my_idx].context.uc_stack.ss_sp = NULL;
		contexts[my_idx].context.uc_stack.ss_size = 0;
	}
	contexts[my_idx].state = DONE;

	int next = select_next_thread(my_idx); // Find next valid context
	if (next >= 0) { // Switch to next valid context
		current_context_idx = (uint8_t)next; 
		setcontext(&contexts[next].context);
	} else { // No valid contexts left, switch back to main
		current_context_idx = NUM_CTX;
		setcontext(&main_ctx);
	}

	abort();
}