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
void Skeleton(void* param);  //needed in shedule fn

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
    MachineContextCreate(&(GLOBAL_TCB_LIST.FindTCB(*tid)->DContext), Skeleton, GLOBAL_TCB_LIST.FindTCB(*tid), GLOBAL_TCB_LIST.FindTCB(*tid)->DStack, GLOBAL_TCB_LIST.FindTCB(*tid)->DMemsize);
}

void TCBList::Sleep(TVMTick tick){
    //VMPrint("Sleep member start\n");
    if (DCurrentTCB == NULL){
        // Main has been told to go to sleep
        GLOBAL_TICK = tick;
        while (GLOBAL_TICK > 0);
        VMPrint("Main Sleeping\n");
        return;
    }
    else{
        VMPrint("Thread has been told to go to sleep\n");
    }
    DCurrentTCB->DTicks=tick;
    DCurrentTCB->DState = VM_THREAD_STATE_WAITING;
    
    GLOBAL_TCB_LIST.Schedule();
    
    DSleepingList.push_back(DCurrentTCB);
    //DCurrentTCB = NULL;
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
    VMPrint("Thread Activated \n");
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    //MachineContextCreate(&GLOBAL_TCB_LIST.FindTCB(thread)->DContext, Skeleton, GLOBAL_TCB_LIST.FindTCB(thread)->DParam, GLOBAL_TCB_LIST.FindTCB(thread)->DStack, GLOBAL_TCB_LIST.FindTCB(thread)->DMemsize);
    
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
        VMPrint("/No schedule necessary \n");
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
    VMPrint("The Scheduler Begins \n");
    TCB* foundtcb;
    TCB* oldtcb;
    if (!GLOBAL_TCB_LIST.DHighPrio.empty()){    //http://www.cplusplus.com/reference/queue/queue/empty/
        //VMPrint("High priority found \n");
        //CHANGE CURRENT THREAD TO READY
        if(GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_RUNNING){
            //VMPrint("Currently running, change to ready\n");
            GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_READY;
        }
        //PUSH CURRENT THREAD INTO THE HIGH PRIORITY QUEUE
        if(GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_READY){
            //VMPrint("Currently ready, push to high priority queue \n");
            GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.DCurrentTCB);
        }
        else if(GLOBAL_TCB_LIST.DCurrentTCB->DTicks > 0 && GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_WAITING){
            //VMPrint("Ticks there, waiting\n");
            while (DCurrentTCB->DTicks > 0);
            //GLOBAL_TCB_LIST.DSleepingList.push_back(DCurrentTCB);
        }
        //VMPrint("Ready to Switch\n");
        oldtcb = GLOBAL_TCB_LIST.DCurrentTCB;
        if (!GLOBAL_TCB_LIST.DHighPrio.empty()){
            foundtcb = GLOBAL_TCB_LIST.FindTCB(DHighPrio.front()->DTID);
        }

        
        GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(DHighPrio.front()->DTID);
        //VMPrint("Ready to Switch\n");
        //GLOBAL_TCB_LIST.DHighPrio.pop();
        //VMPrint("Context Switch\n");
        //MachineContextSwitch(&(oldtcb->DContext), &(GLOBAL_TCB_LIST.DCurrentTCB->DContext));
    }
    
    
    
    else if (!GLOBAL_TCB_LIST.DMedPrio.empty()){
        VMPrint("Med priority found \n");
        foundtcb = FindTCB(DMedPrio.front()->DTID);
        //DMedPrio.pop();
    }
    else if (!GLOBAL_TCB_LIST.DLowPrio.empty()){
        VMPrint("Low priority found \n");
        foundtcb = FindTCB(DLowPrio.front()->DTID);
        //DLowPrio.pop();
    }
    
    //idle
    else{
        VMPrint("Nothing found, push current thread\n");
        GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.DCurrentTCB);
        
        VMPrint("idle\n");
        //PUSH CURRENT THREAD INTO THE HIGH PRIORITY QUEUE
        if(GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_READY){
            VMPrint("Current is ready\n");
            GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.DCurrentTCB);
        }
        else if(GLOBAL_TCB_LIST.DCurrentTCB->DTicks > 0 && GLOBAL_TCB_LIST.DCurrentTCB->DState == VM_THREAD_STATE_WAITING){
            VMPrint("Ticks & current is waiting\n");
            GLOBAL_TCB_LIST.DSleepingList.push_back(DCurrentTCB);
        }
        else{
            VMPrint("else\n");
        }
        oldtcb = GLOBAL_TCB_LIST.DCurrentTCB;
        foundtcb = GLOBAL_TCB_LIST.DHighPrio.front();
        //set current to idle thread
        GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(IDLE_ID);
        //set idle to running
        GLOBAL_TCB_LIST.DCurrentTCB->DState = VM_THREAD_STATE_RUNNING;
        //push idle thread to list of threads
        GLOBAL_TCB_LIST.DTList.__emplace_back( GLOBAL_TCB_LIST.DCurrentTCB);
        
        //switch to idle thread
        VMPrint("Context Switch to idle\n");
        //MachineContextSwitch(&(oldtcb->DContext), &(GLOBAL_TCB_LIST.DCurrentTCB->DContext));

    }
}
void TCBList::DecrementSleep(){
    for (int threadnum = 0; threadnum < DSleepingList.size(); threadnum++){
        if (DSleepingList[threadnum]->DTicks != 0){
            DSleepingList[threadnum]->DTicks --;

        }
        else {
            VMPrint("context switch from sleep\n");
            GLOBAL_TCB_LIST.AddToReady(DSleepingList[threadnum]);
            //DSleepingList.erase(DSleepingList.begin()+threadnum); //http://www.cplusplus.com/reference/vector/vector/erase/
            //GLOBAL_TCB_LIST.Schedule();
            
            //remove it from the sleeping list
            
            
        }
        
    }
}
/*
struct Mutex{
    // Needed for thread create
    TVMThreadIDRef DMOwner;
    TVMMutexIDRef DMIDRef;
    
    // Constructor
    Mutex(TVMThreadIDRef thread, TVMMutexIDRef mutex);
    

};

Mutex::Mutex(TVMThreadIDRef thread, TVMMutexIDRef mutex){
    DMOwner = thread;
    DMIDRef = mutex;

}
struct MutexList{
    // Current Mutex
    Mutex* DCurrentMutex;
    
    // Containers
    std::vector<Mutex*> DMList;
    // Ready lists
    std::queue<TCB*> DMHighPrio;
    std::queue<TCB*> DMMedPrio;
    std::queue<TCB*> DMLowPrio;
    
    
    //Functions
    //Functions
    void Lock(TVMMutexID mutexref, TVMTick timeout);
    void Unlock();

    //Scheduler
    //void Schedule();
};

MutexList GLOBAL_MUTEX_LIST = MutexList();

void MutexList::Lock(TVMMutexID mutexref, TVMTick timeout){
      VMPrint("Lock\n");
    
    //sleep for specified time
    VMThreadSleep(timeout);
}
void MutexList::Unlock(){
    VMPrint("Unlock\n");
}
*/
void Skeleton(void* param){ //The skeleton is just the entry fn you give to the thread.  Can be any function switching to with MCC
    MachineEnableSignals();
    TCB* paramTCB;
    paramTCB = (TCB*)param;
    

    VMThreadTerminate(paramTCB->DTID);
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
        VMPrint(".\n");
        mCallbackTick = mCallbackTick - 5;
    }
    TCB* callbackTCB = (TCB*)calldata;
    callbackTCB->DState = VM_THREAD_STATE_READY;
    //GLOBAL_TCB_LIST.AddToReady(callbackTCB);
    callbackTCB->DFd = result;
    
    
    if(GLOBAL_TCB_LIST.DCurrentTCB->DPrio < callbackTCB->DPrio){
        VMPrint(".\n");
        GLOBAL_TCB_LIST.Schedule();
    }
    //GLOBAL_TCB_LIST.Schedule();
}
void IdleCallback(void* calldata){
    //MachineEnableSignals(); //enabling signals allows the callbacks to be called
    //VMPrint("Idle\n");
    while(1){
        //VMPrint("Idle\n");
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
    
    MachineInitialize(); //proj2
    //MachineInitialize(sharedsize); //proj3
    MachineRequestAlarm(1000 * tickms, AlarmCallback, NULL);
    MachineEnableSignals();
    
    VMThreadCreate(NULL, NULL, 0x100000, VM_THREAD_PRIORITY_HIGH, &MAIN_ID); //ID should be 2
    GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.FindTCB(MAIN_ID));
    
    //create idle thread
    VMThreadCreate(IdleCallback, NULL, 0x100000, ((TVMThreadPriority)0x00), &IDLE_ID); //ID should be 1
    GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DState =  VM_THREAD_STATE_READY;
    MachineContextCreate(&(GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DContext), IdleCallback, NULL, GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DStack, GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DMemsize);
    
    
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
    GLOBAL_TCB_LIST.ThreadCreate(entry ,param, memsize, prio, tid); //SETTING TO NULL FOR NOW, CHANGE PARAM
    
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

/*
TVMStatus VMMutexCreate(TVMMutexIDRef mutex){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    GLOBAL_MUTEX_LIST.DMList.__emplace_back(new Mutex(0, mutex)); //not sure about thread owner arg, need to test
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;

}
TVMStatus VMMutexDelete(TVMMutexID mutex){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    GLOBAL_MUTEX_LIST.DMList[mutex] = NULL;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    *ownerref = *GLOBAL_MUTEX_LIST.DMList[mutex]->DMOwner;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMMutexAcquire(TVMMutexID mutexref, TVMTick timeout){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    //Match mutex priority Q with current thread priority Q
    
    //sleep for specified time
    GLOBAL_MUTEX_LIST.Lock(mutexref, timeout);
    
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMMutexRelease(TVMMutexID mutexref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    //check each Mutex priority Qs, if empty then release them
    GLOBAL_MUTEX_LIST.Unlock();
    
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}


*/


#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
