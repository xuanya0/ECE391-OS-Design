.globl sys_call_handler
.globl exit_to_shell
.globl jump_to_program

sys_call_handler:
	push	%es			# Save all the registers before
	push	%ds			# calling system_call functions
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	cmpl $1,%eax
	jl invalid_num
	cmpl $10,%eax
	jg invalid_num
	# Pass parameters to system_call
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	# Call the corresponding system_call 
	# functions indicated by EAX
	call	*targets(, %eax, 4)
	# Tear down the stack
	addl	$12, %esp
	jmp end_of_syscall
invalid_num:
	movl $-1,%eax
end_of_syscall:
	popl	%ebx		# Restore all the registers after
	popl	%ecx		# returning
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	pop		%ds
	pop		%es
	iret			
	.align	4		
targets:		
	.long	0		
	.long	halt	
	.long	execute	
	.long	read	
	.long	write	
	.long	open	
	.long	close	
	.long	getargs	
	.long	vidmap	
	.long	set_handler
	.long	sigreturn




jump_to_program:
	pushfl
	popl	%eax

	pushl	%ebp
	movl	%esp, %ebp

	movl	16(%ebp), %ecx
	cmpl	$0, %ecx
	jl		continue_jump_to_p
	movl	%esp, shell_esp_array(,%ecx,4)
	
continue_jump_to_p:
	pushl   $0x2B			
	pushl   8(%ebp)			
	pushl	%eax					
	pushl   $0x23		
	pushl	12(%ebp)			
	movl	8(%ebp), %ebp
	movw    $0x2B, %ax	
	movw    %ax, %ds
	sti
	iret					
	
exit_to_shell:
	pushl	%ebp
	movl	%esp, %ebp
	
	xorl	%eax, %eax
	movl	8(%ebp), %eax
	
	movl	12(%ebp), %ecx
	
	movl	shell_esp_array(,%ecx,4), %esp
	movl	%esp, %ebp
	sti
	leave
	ret
	
shell_esp_array:
	.rept	6
    .long   0	
	.endr
