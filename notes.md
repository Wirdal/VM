## What we have
 1. A TCB Struct which holds a bunch of info
 2. A TCBList struct, which holds all of TCBs, the ready TCBs into vectors depending on their priority
    Methods that perform operations based on the current TCB, TCB Finding, and a 'scheduling' function, that can return/set the next TCB in line, and remove TCBs from the ready lists.
    
## What we think we know
 -Fill out-
 
 ## What we don't know
 1. How to perform the threads 'function' on each tick
 2. How to properly use the callbacks
 3. When we should be calling our sched. fn
 4. Skeleton function?
 5. How to control the MachineFileXXX
 