#include <setjmp.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "tinythreads.h"

#define NULL            0
#define DISABLE()       cli()
#define ENABLE()        sei()
#define STACKSIZE       80
#define NTHREADS        4
#define SETSTACK(buf,a) *((unsigned int *)(buf)+8) = (unsigned int)(a) + STACKSIZE - 4; \
                        *((unsigned int *)(buf)+9) = (unsigned int)(a) + STACKSIZE - 4

struct thread_block {
    void (*function)(int);   // code to run
    int arg;                 // argument to the above
    thread next;             // for use in linked lists
    jmp_buf context;         // machine state
    char stack[STACKSIZE];   // execution stack space
};

struct thread_block threads[NTHREADS];

struct thread_block initp;

thread freeQ   = threads;
// Alla trådar som vill köra ligger i readyQ
thread readyQ  = NULL;
thread current = &initp;

int initialized = 0;

// Loopar in adresser till nästa objekt i kön (förutom det sista som har null)
static void initialize(void) {
	int i;
	for (i=0; i<NTHREADS-1; i++)
	threads[i].next = &threads[i+1];
	threads[NTHREADS-1].next = NULL;

	CLKPR = 0x80;
	CLKPR = 0x00;
	
	//Joystick
	PORTE |= (1<<PE3);
	EIMSK = (1 << PCIE0);
	PCMSK0 = (1 << PCINT3);
		
	// Klockan
	TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);
	TCNT1 = (TCNT1 & 0X0000);
	TCCR1A = (1 << COM1A1) | (1 << COM1A0);
	TIMSK1 = (1 << OCIE1A);
	OCR1A = 391;
	
    initialized = 1;
}

// Vi skickar in P och en kö. Vi kollar om kön är tom: om den är det så lägger vi till p längst fram i kön. Är kön inte tom lägger vi p
// längst bak. 
static void enqueue(thread p, thread *queue) {
    p->next = NULL;
    if (*queue == NULL) {
        *queue = p;
    } else {
        thread q = *queue;
        while (q->next)
            q = q->next;
        q->next = p;
    }
}

// Vi tar ut tråden längst fram och returnerar det. Sen går vi ett steg vidare i kön.
static thread dequeue(thread *queue) {
    thread p = *queue;
    if (*queue) {
        *queue = (*queue)->next;
    } else {
        // Empty queue, kernel panic!!!
        while (1) ;  // not much else to do...
    }
    return p;
}

// Exekvera den tråd som skickas med
static void dispatch(thread next) {
    if (setjmp(current->context) == 0) {
        current = next;
        longjmp(next->context,1);
    }
}

void spawn(void (* function)(int), int arg) {
    thread newp;

    DISABLE();
    if (!initialized) initialize();

    newp = dequeue(&freeQ);
    newp->function = function;
    newp->arg = arg;
    newp->next = NULL;
    if (setjmp(newp->context) == 1) {
        ENABLE();
        current->function(current->arg);
        DISABLE();
        enqueue(current, &freeQ);
        dispatch(dequeue(&readyQ));
    }
    SETSTACK(&newp->context, &newp->stack);

    enqueue(newp, &readyQ);
    ENABLE();
}


void yield(void) {
	// Queuea current i readyQ
	enqueue(current, &readyQ);
	
	// Genom att ta dequeue som argument får vi ut de första elementet i readyQ. 
	// Dispatch kommer context-switcha (exekvera första elementet i readyQ).
	dispatch(dequeue(&readyQ));
	
}

/*
Om flaggan tidigare var unlocked --> Sätt till locked
Om den inte var unlocked --> Den körande tråden ska placeras i "waiting queue" of the mutex och en ny tråd ska bli exekverad från readyQ
*/
void lock(mutex *m) {
	
	if (m->locked == 0)
	{
		m->locked = 1;
	}
	else{
		enqueue(current, &m->waitQ);
		dispatch(dequeue(&readyQ));
	}
}
/* 
Ska aktivera en tråd i Waiting Queue of the mutex om det finns något i waitingQ. Annars ska locked flag resetas. 
*/ 
void unlock(mutex *m) {
	if(m->waitQ != NULL){
		enqueue(current, &readyQ);
		dispatch(dequeue(&m->waitQ));

	}
	else{
		m->locked = 0;
	}

}
