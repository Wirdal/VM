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
    // Ready lists
    std::queue<TCB*> DHighPrio;
    std::queue<TCB*> DMedPrio;
    std::queue<TCB*> DLowPrio;
    std::vector<TCB*> DSleepingList;

    //Functions
    TCB* FindTCB(TVMThreadID id);
    void AddToReady(TCB*);
    void Delete(TVMThreadID id);
    void Sleep(TVMTick tick);
    void DecrementSleep();
    //Scheduler
    void Schedule();
};

TCB* TCBList::FindTCB(TVMThreadID id){
    //Looks for a TCB given a TID
    for (auto iteratedlist: DTList){
        if (iteratedlist->DTID == id){
            return iteratedlist;
        }
    }
    return NULL;
};
void TCBList::AddToReady(TCB* tcb){
    switch (tcb->DState){
        case VM_THREAD_PRIORITY_LOW: DLowPrio.push(tcb);
        case VM_THREAD_PRIORITY_NORMAL: DMedPrio.push(tcb);
        default: DHighPrio.push(tcb);
    }
};
void TCBList::Delete(TVMThreadID id){
    // Might need to handle if it is the current tcb.
    int i = 0;
    for (auto iteratedlist: DTList){
        if (iteratedlist->DTID == id){
            DTList.erase(DTList.begin()+i);
        }
        i++;
    }
};
void TCBList::Sleep(TVMTick tick){
    if (DCurrentTCB == NULL){
        // Main has been told to go to sleep

        return;
    }
    DCurrentTCB->DTicks=tick;
    DSleepingList.push_back(DCurrentTCB);
    DCurrentTCB = NULL;
};

void TCBList::Schedule(){
    //Look into the priorities, and switch context based on the results.
    //Also set the current TCB properly
    //Do I need to pop though?
    TCB* foundtcb;
    if (DHighPrio.front() != NULL){
        foundtcb = FindTCB(DHighPrio.front()->DTID);
        DHighPrio.pop();
    }
    else if (DMedPrio.front() != NULL){
        foundtcb = FindTCB(DMedPrio.front()->DTID);
        DMedPrio.pop();
    }
    else if (DLowPrio.front() != NULL){
        foundtcb = FindTCB(DLowPrio.front()->DTID);
        DLowPrio.pop();
    }
    else {
        //get the idle thread
        // foundtcb = IDLE_TCB;
        
    }
    MachineContextSwitch(&(DCurrentTCB->DContext),&(foundtcb->DContext));

};
void TCBList::DecrementSleep(){
    int i = 0;
    for (auto iteratedlist: DSleepingList){
        if (iteratedlist->DTicks == 1){
            iteratedlist->DTicks = 0;
            DSleepingList.erase(DSleepingList.begin()+i);
            //remove it from the sleeping list
            
        }
        else {
            iteratedlist->DTicks --;
        }
        i++;
    }
};
/*****************************
 *  Any global vars are here *
 * **************************/

TCBList GLOBAL_TCB_LIST = TCBList();
int GLOBAL_TICK = 0;
// TVMThreadID IDLE_ID;

/*****************************
 * Our definitions are here  *
 *     Callback Functions    *
 * **************************/
void AlarmCallback(void *calldata){
    GLOBAL_TCB_LIST.DecrementSleep();
    VMPrint("Alarm called \n");
    GLOBAL_TCB_LIST.Schedule();
};
void MachineCallback(void *calldata, int result){

};
void IdleCallback(){
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
    // GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(IDLE_ID);
    TVMMainEntry entry = VMLoadModule(argv[0]);
    entry(argc, argv);
    VMUnloadModule();
    return VM_STATUS_SUCCESS;
};

TVMStatus VMTickMS(int *tickmsref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    // tickmsref = sometick
    MachineResumeSignals(&localsigs);
};
TVMStatus VMTickCount(TVMTickRef tickref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    *tickref = GLOBAL_TICK;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.DTList.emplace_back(new TCB(entry, param, memsize, prio)); //Add it on the heap
    *tid = TCB::DTIDCounter;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadDelete(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.Delete(thread);
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadActivate(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    
    if (foundtcb == NULL){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    if (foundtcb ->DState != VM_THREAD_STATE_DEAD) {
        return VM_STATUS_ERROR_INVALID_STATE;
    }

    MachineContextCreate(&foundtcb->DContext, foundtcb->DEntry, foundtcb->DParam, foundtcb->DStack, foundtcb->DMemsize);
    
    // Check if the thread needs to yield
    if (foundtcb->DState > GLOBAL_TCB_LIST.DCurrentTCB->DState) {
        // Special scheduuling?
        // foundtcb->DEntry(foundtcb->DParam);
        GLOBAL_TCB_LIST.DCurrentTCB = foundtcb;
    }

    foundtcb->DState = VM_THREAD_STATE_READY; 
    GLOBAL_TCB_LIST.AddToReady(foundtcb);
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
};
TVMStatus VMThreadTerminate(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    foundtcb->DState=VM_THREAD_STATE_DEAD;
    // Remove from ready
    // Call schedular
    MachineResumeSignals(&localsigs);
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
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.Sleep(tick);
    MachineResumeSignals(&localsigs);

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

