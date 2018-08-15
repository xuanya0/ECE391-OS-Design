# handlers.S
.globl timer_hand
.globl kbd_hand
.globl rtc_hand

timer_hand:
	cli

	push	%es
	push	%ds
	pushl	%eax
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	
	movl	%esp, sched_esp
	
	call	timer
	
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	popl	%eax
	pop		%ds
	pop		%es
	
	sti
	
	iret

kbd_hand:
	cli

	push	%es
	push	%ds
	pushl	%eax
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	
	call	kbd
	
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	popl	%eax
	pop		%ds
	pop		%es
	
	sti
	
	iret

rtc_hand:
	cli

	push	%es
	push	%ds
	pushl	%eax
	pushl	%ebp
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	
	call	rtc_handler
	
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	popl	%ebp
	popl	%eax
	pop		%ds
	pop		%es
	
	sti
	
	iret
	