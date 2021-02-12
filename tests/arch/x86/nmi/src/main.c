/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <tc_util.h>

#include <kernel_internal.h>
#if defined(__GNUC__)
#include "test_asm_inline_gcc.h"
#else
#include "test_asm_inline_other.h"
#endif

static volatile int int_handler_executed;

extern uint8_t z_x86_nmi_stack[];
extern uint8_t z_x86_nmi_stack1[];
extern uint8_t z_x86_nmi_stack2[];
extern uint8_t z_x86_nmi_stack3[];

uint8_t *nmi_stacks[] = {
	z_x86_nmi_stack,
#if CONFIG_MP_NUM_CPUS > 1
	z_x86_nmi_stack1,
#if CONFIG_MP_NUM_CPUS > 2
	z_x86_nmi_stack2,
#if CONFIG_MP_NUM_CPUS > 3
	z_x86_nmi_stack3
#endif
#endif
#endif
};

#define MAX_MSGS 4
K_MSGQ_DEFINE(mqueue, sizeof(int), MAX_MSGS, 4);

#define STACK_SIZE 1024
#define PRIORITY 7
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
struct k_thread thread_id;

bool z_x86_do_kernel_nmi(const z_arch_esf_t *esf)
{
	uint64_t stack;

	_get_esp(stack);

	TC_PRINT("ESP: 0x%llx CPU %d nmi_stack %p\n", stack,
		 arch_curr_cpu()->id, nmi_stacks[arch_curr_cpu()->id]);

	zassert_true(stack > (uint64_t)nmi_stacks[arch_curr_cpu()->id] &&
		     stack < (uint64_t)nmi_stacks[arch_curr_cpu()->id] +
		     CONFIG_X86_EXCEPTION_STACK_SIZE, "Incorrect stack");

	int_handler_executed++;

	k_msgq_put(&mqueue, (void *)&int_handler_executed, K_NO_WAIT);

	return true;
}

void test_nmi_handler(void)
{
	TC_PRINT("Testing to see interrupt handler executes properly\n");

	_trigger_isr_handler(IV_NON_MASKABLE_INTERRUPT);

	zassert_not_equal(int_handler_executed, 0,
			  "Interrupt handler did not execute");
	zassert_equal(int_handler_executed, 1,
		      "Interrupt handler executed more than once! (%d)\n",
		      int_handler_executed);
}

void thread_function(void *p1, void *p2, void *p3)
{
	TC_PRINT("Thread started");

	while (true) {
		int payload;

		k_msgq_get(&mqueue, &payload, K_FOREVER);
		printk("Got notification, data %d", payload);
	}
}

void test_nmi_handler_user(void)
{
	k_thread_create(&thread_id, thread_stack, STACK_SIZE, thread_function,
			NULL, NULL, NULL, PRIORITY, K_USER, K_FOREVER);
	k_thread_access_grant(&thread_id, &mqueue);
	k_thread_start(&thread_id);

	for (int i = 0; i < 20; i++) {
		//k_sleep(K_MSEC(100));
		_trigger_isr_handler(IV_NON_MASKABLE_INTERRUPT);
	}
}

void test_main(void)
{
	ztest_test_suite(nmi, ztest_unit_test(test_nmi_handler),
			 ztest_unit_test(test_nmi_handler_user));
	ztest_run_test_suite(nmi);
}
