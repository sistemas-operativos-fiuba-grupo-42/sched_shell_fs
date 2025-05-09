/*
 * Your code here for context_switch
 *
 * It should restore the register values in the Trapframe with the 'iret' instruction.
 * This exits the kernel and starts executing some environment's code.
 *
 * This function does not return.
 */

.globl context_switch;
// context_switch(struct Trapframe *tf)
// Restaurar el struct Trampframe ya que no lo hace iret. importante del struct -> env_id, env_pgdir, env_tf
// -> *tf    |   -> tf
// [esp] -> return value
// [esp + 4] -> tf
// [esp] -> *tf -> tf
context_switch:
	add $4, %esp
	movl (%esp), %esp

	popal
	pop %es
	pop %ds

	add $8, %esp
	
	iret

spin:
	jmp spin
