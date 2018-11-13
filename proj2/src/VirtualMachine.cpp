#include "VirtualMachine.h"
#include "Machine.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <queue>

/*****************************
 *  Any global vars are here *
 * **************************/

//TCBList GLOBAL_TCB_LIST = TCBList();
int GLOBAL_TICK;
TVMThreadID IDLE_ID = 2; //DTID incremented when VMCreate called
TVMThreadID MAIN_ID = 1;

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
    
}
// TCB::~TCB(){
//     delete &DStack;
// };
void TCB::IncrementID(){
    ++DTIDCounter;
}

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
    void Threads();
    void ThreadActivate(TVMThreadID thread);
    void ThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
    //Scheduler
    void Schedule();
};
TCBList GLOBAL_TCB_LIST = TCBList();

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
void TCBList::Delete(TVMThreadID id){
    // Might need to handle if it is the current tcb.
    int i = 0;
    for (auto iteratedlist: DTList){
        if (iteratedlist->DTID == id){
            DTList.erase(DTList.begin()+i);
        }
        i++;
    }
}

void TCBList::ThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    GLOBAL_TCB_LIST.DTList.__emplace_back(new TCB(entry, param, memsize, prio)); //Add it on the heap
}

void TCBList::Sleep(TVMTick tick){
    //VMPrint("Sleep member start\n");
    if (DCurrentTCB == NULL){
        // Main has been told to go to sleep
        GLOBAL_TICK = tick;
        while (GLOBAL_TICK > 0);
        //VMPrint("Main Sleeping\n");
        return;
    }
    else{
        //VMPrint("Thread has been told to go to sleep\n");
    }
    DCurrentTCB->DTicks=tick;
    while (DCurrentTCB->DTicks > 0);
    DSleepingList.push_back(DCurrentTCB);
    DCurrentTCB = NULL;
    //std::cout<<"ticks: "<< tick << "\n";
    //VMPrint("Sleep member fn end\n");
}

void TCBList::ThreadActivate(TVMThreadID thread){

    
    //TO OUTPUT INFO
//    if (foundtcb == NULL){
//        VMPrint("/Found NULL TCB\n");
//        //return VM_STATUS_ERROR_INVALID_ID;
//    }
//    else{
//        VMPrint("/Found a TCB\n");
//    }
//    if (GLOBAL_TCB_LIST.DCurrentTCB == NULL){
//        VMPrint("DCurrentTCB is NULL");
//    }
//    else{
//        VMPrint("Found a DCurrentTCB");
//    }
//    if (foundtcb ->DState != VM_THREAD_STATE_DEAD) {
//        VMPrint("/Found non dead thread\n");
//        //return VM_STATUS_ERROR_INVALID_STATE;
//    }

    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    MachineContextCreate(&GLOBAL_TCB_LIST.FindTCB(thread)->DContext, GLOBAL_TCB_LIST.FindTCB(thread)->DEntry, GLOBAL_TCB_LIST.FindTCB(thread)->DParam, GLOBAL_TCB_LIST.FindTCB(thread)->DStack, GLOBAL_TCB_LIST.FindTCB(thread)->DMemsize);
    
    // Check if the thread needs to yield
    if (foundtcb->DState > GLOBAL_TCB_LIST.DCurrentTCB->DState) {
        VMPrint("/Thread needs to yield\n");
        // Special scheduuling?
        // foundtcb->DEntry(foundtcb->DParam);
        GLOBAL_TCB_LIST.DCurrentTCB = foundtcb;
    }
    else{
        //VMPrint("/Thread does not need to yield\n");
    }
    
    if ((GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_RUNNING) && (GLOBAL_TCB_LIST.DCurrentTCB->DPrio < foundtcb->DPrio)){
        //VMPrint("/Activate to scheduler, running and current prio < found tcb \n");
        GLOBAL_TCB_LIST.Schedule();
    }
    else if(GLOBAL_TCB_LIST.DCurrentTCB->DState != VM_THREAD_STATE_RUNNING){
        //VMPrint("/Activate to scheduler, current not running \n");
        GLOBAL_TCB_LIST.Schedule();
    }
    else{
       // VMPrint("/No schedule necessary \n");
    }
    
    foundtcb->DState = VM_THREAD_STATE_READY;
    GLOBAL_TCB_LIST.AddToReady(foundtcb);
    //VMPrint("/ThreadActivate member fn end\n");
}

void TCBList::Threads(){
    //create main thread
    //std::cout << "IDLE" << " | TCB ID: " << GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DTID << "\n"; //should be 1
    GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(MAIN_ID);
    //std::cout << "Current TCB: "<< GLOBAL_TCB_LIST.FindTCB(MAIN_ID)->DTID << "|" <<GLOBAL_TCB_LIST.DCurrentTCB->DTID << "\n";
}

void TCBList::Schedule(){
    //Look into the priorities, and switch context based on the results.
    //Also set the current TCB properly
    //Do I need to pop though?

    // - This chunk gave my machine some pretty serious errors, ended up having to restart my machine to get it to work again, it was checking to see if the queue was empty by comparing it to NULL, fixed by checking if empty instead of comparing with NULL.
    //VMPrint("The Scheduler Begins \n");
    TCB* foundtcb;
    if (!GLOBAL_TCB_LIST.DHighPrio.empty()){    //http://www.cplusplus.com/reference/queue/queue/empty/
        VMPrint("High priority found \n");
        foundtcb = FindTCB(DHighPrio.front()->DTID);
        DHighPrio.pop();
    }
    else if (!GLOBAL_TCB_LIST.DMedPrio.empty()){
        VMPrint("Med priority found \n");
        foundtcb = FindTCB(DMedPrio.front()->DTID);
        DMedPrio.pop();
    }
    else if (!GLOBAL_TCB_LIST.DLowPrio.empty()){
        VMPrint("Low priority found \n");
        foundtcb = FindTCB(DLowPrio.front()->DTID);
        DLowPrio.pop();
    }
    else{
        //VMPrint("Nothing found, push current thread\n");
        GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.DCurrentTCB);
        //get the idle thread
        // foundtcb = IDLE_TCB;
    }
}
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
}

void Skeleton(void* param){ //The skeleton is just the entry fn you give to the thread.  Can be any function switching to with MCC
    MachineEnableSignals();
    GLOBAL_TCB_LIST.DCurrentTCB->DEntry(param);
    VMThreadTerminate(GLOBAL_TCB_LIST.DCurrentTCB->DTID);
}

/*****************************
 * Our definitions are here  *
 *     Callback Functions    *
 * **************************/
void AlarmCallback(void *calldata){
    //MachineEnableSignals();
    //VMPrint("Alarm Callback \n");
    GLOBAL_TICK--;
    GLOBAL_TCB_LIST.DecrementSleep();
    GLOBAL_TCB_LIST.DCurrentTCB->DTicks = GLOBAL_TCB_LIST.DCurrentTCB->DTicks - 1;
    //std::cout << "Dticks: "<<GLOBAL_TCB_LIST.DCurrentTCB->DTicks<<"\n";
    //GLOBAL_TCB_LIST.Schedule();
}
int mCallbackTick; //This is just used to debug so we can see when the Machine Callback gets called without having it flood the screen
void MachineCallback(void *calldata, int result){
    //MachineEnableSignals();
    mCallbackTick++;
    while(mCallbackTick>5){
        //VMPrint(".\n");
        mCallbackTick = mCallbackTick - 5;
    }
    //GLOBAL_TCB_LIST.Schedule();
}
void IdleCallback(void* calldata){
    //MachineEnableSignals(); //enabling signals allows the callbacks to be called
    VMPrint("Idle\n");
    while(1){
        VMPrint("Idle\n");
    }
    //GLOBAL_TCB_LIST.Schedule();
    //Schedule
}
              
/*****************************
 * The required code is here *
 * **************************/

extern "C" {
    typedef void (*TVMMainEntry)(int, char*[]); 
    TVMMainEntry VMLoadModule(const char *module);
    void VMUnloadModule(void);
    TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
}

TVMStatus VMThreadActivate(TVMThreadID thread);
TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);


TVMStatus VMStart(int tickms, int argc, char *argv[]){
    //these must be before request alarm and enable si
    
    MachineInitialize();
    
    MachineRequestAlarm(1000 * tickms, AlarmCallback, NULL);
    MachineEnableSignals();
    
    VMThreadCreate(NULL, NULL, 0x100000, VM_THREAD_PRIORITY_HIGH, &MAIN_ID); //ID should be 2
    
    //create idle thread
    VMThreadCreate(IdleCallback, NULL, 0x100000, ((TVMThreadPriority)0x00), &IDLE_ID); //ID should be 1
    
    //GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(MAIN_ID); //This was giving me problems because it seemed to not have access to the data, not sure, reorganized it to be in Thread membmer fn, seen below
    GLOBAL_TCB_LIST.Threads();

    TVMMainEntry entry = VMLoadModule(argv[0]);
    entry(argc, argv);
    //VMUnloadModule();
    return VM_STATUS_SUCCESS;
}

TVMStatus VMTickMS(int *tickmsref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    *tickmsref = GLOBAL_TICK;
    MachineResumeSignals(&localsigs);
}
TVMStatus VMTickCount(TVMTickRef tickref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    *tickref = GLOBAL_TICK;
    MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.ThreadCreate(entry ,NULL, memsize, prio, tid); //SETTING TO NULL FOR NOW, CHANGE PARAM
    
    //MachineContextCreate(&(GLOBAL_TCB_LIST.FindTCB(*tid)->DContext), Skeleton, NULL, GLOBAL_TCB_LIST.FindTCB(*tid)->DStack, 0x100000); //idle just runs NULL
    *tid = TCB::DTIDCounter;
    MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadDelete(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.Delete(thread);
    MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadActivate(TVMThreadID thread){
    //std::cout << "std::cout VMThreadActivate ID: "<<thread<<"\n";
    //VMPrint("VMThreadActivate\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    GLOBAL_TCB_LIST.ThreadActivate(thread);
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMThreadTerminate(TVMThreadID thread){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    foundtcb->DState=VM_THREAD_STATE_DEAD;
    // Remove from ready
    // Call schedular
    MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadID(TVMThreadIDRef threadref){
    //std::cout << "VMThreadID\n";
    //VMPrint("VMThreadID");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    *threadref = GLOBAL_TCB_LIST.DCurrentTCB->DTID;
    MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
    //std::cout << "VMThreadState\n";
    //VMPrint("VMThreadState\n");
    //TMachineSignalState localsigs;
    //MachineSuspendSignals(&localsigs);
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    
    *stateref = foundtcb->DState;
    //MachineResumeSignals(&localsigs);
}

TVMStatus VMThreadSleep(TVMTick tick){
    //TMachineSignalState localsigs;
    //MachineSuspendSignals(&localsigs);
    
    GLOBAL_TCB_LIST.Sleep(tick);
    //GLOBAL_TCB_LIST.Schedule();
    //MachineResumeSignals(&localsigs);

}

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
    //VMPrint("VMFileOpen Called\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    MachineFileOpen(filename, flags, mode, MachineCallback, &GLOBAL_TCB_LIST.DCurrentTCB);
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMFileClose(int filedescriptor){
    //VMPrint("VMFileClosed Called\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    MachineFileClose(filedescriptor, MachineCallback, &GLOBAL_TCB_LIST.DCurrentTCB); //last argument is the thread its writing to.
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;

}
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
    //VMPrint("VMFileRead Called\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    MachineFileRead(filedescriptor, data, *length, MachineCallback, &GLOBAL_TCB_LIST.DCurrentTCB); //last argument is the thread its writing to.
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
    //VMPrint("VMFileWrite Called\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    MachineFileWrite(filedescriptor, data, *length, MachineCallback, &GLOBAL_TCB_LIST.DCurrentTCB); //last argument is the thread its writing to.
    //GLOBAL_TCB_LIST.Schedule();
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
    //VMPrint("VMFileSeek Called\n");
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    MachineFileSeek(filedescriptor, offset, whence, MachineCallback, &GLOBAL_TCB_LIST.DCurrentTCB);
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;

}


/*****************************
 *         Mutex      *
 ****************************/

/* proj3
TVMStatus VMMutexCreate(TVMutexIDRef mutexref){
    
}
TVMStatus VMMutexDelete(TVMutexID mutexref){
    
}

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMutexIDRef mutexref){
    
}
TVMStatus VMMutexAcquire(TVMutexID mutexref, TVMTick timeout){
    
}
TVMStatus VMMutexRelease(TVMutexID mutexref){
    
}

*/


#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
