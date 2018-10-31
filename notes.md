## What we have
 1. A TCB Struct which holds a bunch of info
 2. A TCBList struct, which holds all of TCBs, the ready TCBs into vectors depending on their priority
    Methods that perform operations based on the current TCB, TCB Finding, and a 'scheduling' function, that can return/set the next TCB in line, and remove TCBs from the ready lists.
 3.  An Idle thread created, which runs an empty loop (Maybe) 
 4.  A main loop, which accesses the provided file and runs its main fn
 4.  VMThreadCreate, VMThreadActivate.
 
    
## What we think we know
- We are supposed to create 2 threads, Idle and Main, main runs the specified program.  The main thread can create new threads.  When each thread is created (or maybe on each tick), the machine should check what thread should be running in a scheduler function.  This scheduler function should decide which thread to run based on the priority stored in the TCB.  If the scheduler finds a thread with a higher priority, or the current thread finishes, the schduler will switch contexts using MCS(oldthread, newthread).  The context holds thread information such as the function that the thread should be running and is stored in the TCB and saved with SMC.  The context is created with MCC, but we don't know exactly what MCC does (vanessa said we don't really need to know, it magically creates the context for the machine?).-
 
 ## What we don't know
 1. How to perform the threads 'function' on each tick
 2. How to properly use the callbacks
 3. When we should be calling our sched. fn 
 4. Skeleton function?
 5. How to control the MachineFileXXX
 6. Why is our scheduler() not working?
 
