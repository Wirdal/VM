#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <queue>

extern "C" {
    typedef void (*TVMMainEntry)(int, char*[]);
    TVMMainEntry VMLoadModule(const char *module);
    void VMUnloadModule(void);
    TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
}

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
int tickms;
//int globaltickms = 0;
TVMThreadID mainID = 1;
TVMThreadID idleID = 2;


/*****************************
 * Our definitions are here  *
 *     Callback Functions    *
 * **************************/
void Scheduler();
void AlarmCallback(void *calldata){
    //check for threads sleeping
    
    if(!SleepList.empty()){
        //Decrement the ticks until it reaches 0
        if(CurrentTCB->DTicks > 0){
            CurrentTCB->DTicks = CurrentTCB->DTicks - 1;
        }
        //Done sleeping
        else{
            CurrentTCB->DState = VM_THREAD_STATE_READY;
            
            //put in correct place
            //delete from sleeping list
            SleepList.pop_back(); //http://www.cplusplus.com/reference/vector/vector/erase/
        }
        
        
    }
    else{
        VMPrint("****Sleeplist empty\n");

    }

    globaltick++;
    tickms++;
    Scheduler();
    
    

    //Call the scheduler here
};
void EmptyCallback(void *calldata){
    //VMPrint("**empty\n");
};
void MachineCallback(void *calldata, int result){
    //VMPrint("**machine\n");
    //Scheduler();
};
void IdleCallback(void *calldata){
    MachineEnableSignals(); //enabling signals allows the callbacks to be called
    while(1){
        //VMPrint("Idle\n");
    }
    //Schedule
};



/*****************************
 * The required code is here *
 * **************************/



TVMStatus VMStart(int tickms, int argc, char *argv[]){
    MachineInitialize();
    VMPrint("/Machine Init\n");
    //MachineRequest alarm returns immediately, Alarmcallback is called by the machine every tick
    MachineRequestAlarm(10000 * tickms, AlarmCallback, NULL); //should be 1000
    MachineEnableSignals();

    
    
    TVMMainEntry entry = VMLoadModule(argv[0]);
    
    //Create Main Thread
    //VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid)
    VMThreadCreate(AlarmCallback, NULL, 0x100000, VM_THREAD_PRIORITY_NORMAL,&mainID); //mainID -> 0
    CurrentTCB = GLOBAL_TCB_LIST.FindTCB(mainID); //Set current thread to be main
    std::cout<<"Main"<< " | TCB ID: " << CurrentTCB->DTID<< "\n";
    //Create Idle Thread
    VMThreadCreate(IdleCallback, NULL, 0x100000, ((TVMThreadPriority)0x00), &idleID);
    //CurrentTCB = GLOBAL_TCB_LIST.FindTCB(idleID); //Set current thread to be main
    //std::cout<<"IDLE"<< " | TCB ID: " << CurrentTCB->DTID<< "\n";
    
    std::cout<<"Calling entry()"<<"\n";
    entry(argc, argv);
    return VM_STATUS_SUCCESS;
};

void Scheduler(){
    std::cout<<"Called Scheduler()"<<"\n";
    VMPrint("/Scheduler()\n");
    TCB* NewTCB = new TCB(NULL,NULL,NULL,NULL);
    
    //found a waiting thread
    if(CurrentTCB->DState == VM_THREAD_STATE_WAITING){
        NewTCB = CurrentTCB; //New thread front of normal list
        
    //Found a thread to execute
    if(CurrentTCB->DPrio == VM_THREAD_PRIORITY_NORMAL){
        NewTCB = CurrentTCB; //New thread front of normal list
        NewTCB->DState = VM_THREAD_STATE_RUNNING;
        //delete off normal list
    }

    else{
        NewTCB->DTID = idleID; //set the new TCB to be idle
    }

    MachineContextSwitch(&(CurrentTCB->DContext), &(NewTCB->DContext));  //Switch from current to new;
}
void skeleton(void *param){
    CurrentTCB->DEntry(param);
    
}

TVMStatus VMTickMS(int *tickmsref){
    //retreives milliseconds between ticks of the VM
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    *tickmsref = tickms;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
};
TVMStatus VMTickCount(TVMTickRef tickref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    *tickref = globaltick;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.DTList.__emplace_back(new TCB(entry, param, memsize, prio)); //Add it on the heap
    MachineContextCreate(&(GLOBAL_TCB_LIST.FindTCB(*tid)->DContext), skeleton, GLOBAL_TCB_LIST.FindTCB(*tid), GLOBAL_TCB_LIST.FindTCB(*tid)->DStack, GLOBAL_TCB_LIST.FindTCB(*tid)->DMemsize);
    std::cout<<"VMThreadCreate created id: "<< GLOBAL_TCB_LIST.FindTCB(*tid)->DTID<<"\n";
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
    std::cout<<"VMThreadSleep tick: "<< tick <<"\n";
    VMPrint("/sleep\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    CurrentTCB->DState = VM_THREAD_STATE_WAITING;
    CurrentTCB->DTicks = tick;
    SleepList.push_back(CurrentTCB);
    Scheduler();
    MachineResumeSignals(&localsigs);
    VMPrint("/sleep finished\n");
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

#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
