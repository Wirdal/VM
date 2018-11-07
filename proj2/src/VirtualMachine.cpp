#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <queue>

/*****************************
 * Our definitions are here  *
 *     Callback Functions    *
 * **************************/
int globaltick = 0;
void AlarmCallback(void *calldata){
    //Call the scheduler here
};
void MachineCallback(void *calldata, int result){

};
              

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
    TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
    // ~TCB();
    void IncrementID();
};
TVMThreadID TCB::DTIDCounter;

TCB::TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    DEntry = entry;
    DParam = param;
    DMemsize = memsize;
    DPrio = prio;
    IncrementID();
    DTID = DTIDCounter;
    DStack = new uint8_t [memsize];
    tid = &DTID;
    DTicks = 0;
    DFd = 0;
    DState = VM_THREAD_STATE_DEAD;
    // MachineContextCreate(&DContext, DEntry, DParam, &DStack, DMemsize);

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

    TCB* FindTCB(TVMThreadIDRef id);
    void AddToReady(TCB*);
};
TCB* TCBList::FindTCB(TVMThreadIDRef id){
    //Looks for a TCB given a TID
    for (const auto &iteratedlist: DTList){
        if (iteratedlist->DTID == *id){
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
    if (entry == NULL) {
        VMPrint("Failed to load \n");
        return VM_STATUS_FAILURE;
    }

    entry(argc, argv);
    return VM_STATUS_SUCCESS;
};

TVMStatus VMTickMS(int *tickmsref){

};
TVMStatus VMTickCount(TVMTickRef tickref){

};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB newTCB = TCB(entry, param, memsize, prio, tid);
    GLOBAL_TCB_LIST.DTList.push_back(&newTCB);
    *tid = newTCB.DTID;
    MachineResumeSignals(&localsigs);
};

TVMStatus VMThreadDelete(TVMThreadID thread){

};

TVMStatus VMThreadActivate(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* FoundTCB = GLOBAL_TCB_LIST.FindTCB(&thread);
    FoundTCB->DState=VM_THREAD_STATE_READY;
    GLOBAL_TCB_LIST.AddToReady(FoundTCB);
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
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(&thread);
    // foundtcb->DSTate = *stateref;
    stateref = &foundtcb->DState;
    MachineResumeSignals(&localsigs);
};
TVMStatus VMThreadSleep(TVMTick tick){

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

