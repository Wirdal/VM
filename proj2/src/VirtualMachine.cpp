#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <queue>


/*****************************
 *         Data Structs      *
 ****************************/

/*****************************
 *      Thread Control Block  *
 * **************************/
struct TCB{
    // Needed for thread create
    TVMThreadEntry DEntry;
    void * DParam;
    TVMMemorySize DMemsize;
    TVMThreadPriority DPrio;
    TVMThreadID DTID;
    static TVMThreadID DTIDCounter;
    uint8_t * DStack;
    
    // What will get set later/ don't care about at the time
    int DTicks;
    int DFd;
    TVMThreadState DState;
    SMachineContext DContext;
    
    
    // Constructor
    TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio);
    // ~TCB();
    void IncrementID();
};
TVMThreadID TCB::DTIDCounter;

TCB::TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio){
    DEntry = entry;
    DParam = param;
    DMemsize = memsize;
    DPrio = prio;
    IncrementID();
    DTID = DTIDCounter;
    DStack = new uint8_t [memsize];
    DTicks = 0;
    DFd = 0;
    DState = VM_THREAD_STATE_DEAD;
    
    //MachineContextCreate(&DContext, DEntry, DParam, &DStack, DMemsize);
};
// TCB::~TCB(){
//     delete &DStack;
// };
void TCB::IncrementID(){
    ++DTIDCounter;
};

/*****************************
 *      TCB LIST STRUCTURE    *
 * **************************/
struct TCBList{
    // Current thread control block
    TCB* DCurrentTCB;
    
    // Containers
    std::vector<TCB*> DTList;
    std::queue<TCB*> DHighPrio;
    std::queue<TCB*> DMedPrio;
    std::queue<TCB*> DLowPrio;
    
    TCB* FindTCB(TVMThreadID id);
    void AddToReady(TCB*);
};

TCB* TCBList::FindTCB(TVMThreadID id){
    //Looks for a TCB given a TID
    for (auto iteratedlist: DTList){
        if (iteratedlist->DTID == id){
            return iteratedlist;
        }
    }
    return NULL;
}
void TCBList::AddToReady(TCB* tcb){
    switch (tcb->DState){
        case VM_THREAD_PRIORITY_LOW: DLowPrio.push(tcb);
        case VM_THREAD_PRIORITY_NORMAL: DMedPrio.push(tcb);
        default: DHighPrio.push(tcb);
    }
}
/*****************************
 *  Any global vars are here *
 * **************************/

TCBList GLOBAL_TCB_LIST = TCBList();

TCB* CurrentTCB;
std::vector<TCB*> SleepList;

int globaltick = 0;
TVMThreadID mainID = 0;
TVMThreadID idleID = 1;


/*****************************
 * Our definitions are here  *
 *     Callback Functions    *
 * **************************/
void Scheduler();
void AlarmCallback(void *calldata){
    //check for threads sleeping
    if(CurrentTCB->DState == VM_THREAD_STATE_WAITING){
        //Decrement the ticks until it reaches 0
        if(CurrentTCB->DTicks >0){
            CurrentTCB->DTicks = CurrentTCB->DTicks - 1;
        }
        //Done sleeping
        else{
            CurrentTCB->DState = VM_THREAD_STATE_READY;
            //put in correct place
            //delete from sleeping list
        }
        
        
    }
    globaltick++;
    Scheduler();
    
    
    
    //Call the scheduler here
};
void MachineCallback(void *calldata, int result){
    
};
void IdleCallback(void *calldata){
    MachineEnableSignals();
    while(1){
        std::cout<<"hanging"<<"\n";
    }
    //Schedule
};



/*****************************
 * The required code is here *
 * **************************/

extern "C" {
    typedef void (*TVMMainEntry)(int, char*[]);
    TVMMainEntry VMLoadModule(const char *module);
    void VMUnloadModule(void);
    TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
}


TVMStatus VMStart(int tickms, int argc, char *argv[]){
    MachineInitialize();
    MachineEnableSignals();
    MachineRequestAlarm(1000 * tickms, AlarmCallback, NULL);
    
    
    TVMMainEntry entry = VMLoadModule(argv[0]);
    
    //Create Main Thread
    //VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
    VMThreadCreate(NULL, NULL, 0x100000, VM_THREAD_PRIORITY_NORMAL, &mainID);
    
    //Create Idle Thread
    VMThreadCreate(IdleCallback, NULL, 0x100000, ((TVMThreadPriority)0x00), &idleID);
    
    
    entry(argc, argv);
    return VM_STATUS_SUCCESS;
};

void Scheduler(){
    TCB* NewTCB = new TCB(NULL,NULL,NULL,NULL);
    if(CurrentTCB->DPrio == VM_THREAD_PRIORITY_NORMAL){
        std::cout<<"normal shedule"<<"\n";
        NewTCB = CurrentTCB; //New thread front of normal list
        //delete off normal list
    }

    else{
        std::cout<<"else shedule"<<"\n";
        NewTCB->DTID = idleID; //set the new TCB to be idle
    }
    MachineContextSwitch(&(CurrentTCB->DContext), &(NewTCB->DContext));  //Switch from current to new;
}

TVMStatus VMTickMS(int *tickmsref){
    
};
TVMStatus VMTickCount(TVMTickRef tickref){
    
};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.DTList.__emplace_back(new TCB(entry, param, memsize, prio)); //Add it on the heap
    *tid = TCB::DTIDCounter;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadDelete(TVMThreadID thread){
    
};

TVMStatus VMThreadActivate(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* FoundTCB = GLOBAL_TCB_LIST.FindTCB(thread);
    FoundTCB->DState = VM_THREAD_STATE_READY;
    GLOBAL_TCB_LIST.AddToReady(FoundTCB);
    MachineContextCreate(&FoundTCB->DContext, FoundTCB->DEntry, FoundTCB->DParam, FoundTCB->DStack, FoundTCB->DMemsize);
    MachineResumeSignals(&localsigs);
};
TVMStatus VMThreadTerminate(TVMThreadID thread){
    
};

TVMStatus VMThreadID(TVMThreadIDRef threadref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    *threadref = GLOBAL_TCB_LIST.DCurrentTCB->DTID;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    *stateref = foundtcb->DState;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadSleep(TVMTick tick){
    
    CurrentTCB->DState = VM_THREAD_STATE_WAITING;
    CurrentTCB->DTicks = tick;
    SleepList.push_back(CurrentTCB);
    Scheduler();
    return VM_STATUS_SUCCESS;
    //TCBList->DCurrentTCB->DState = VM_THREAD_STATE_WAITING;
    
    
};

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
    
};
TVMStatus VMFileClose(int filedescriptor){
    
};
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
    
};
TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
    MachineFileWrite(filedescriptor, data, *length, MachineCallback, NULL);
};
TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
    
};
