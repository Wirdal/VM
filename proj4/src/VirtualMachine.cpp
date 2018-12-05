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
    std::vector<TVMMutexID> DOwnedMutex;
    void* DWaitedMutex; //Mutex the thread is waiting for

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
    GLOBAL_TCB_LIST.DTList.emplace_back(new TCB(entry, param, memsize, prio)); //Add it on the heap
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
    VMPrint("Thread Activated \n");
    TCB* foundtcb = GLOBAL_TCB_LIST.FindTCB(thread);
    MachineContextCreate(&GLOBAL_TCB_LIST.FindTCB(thread)->DContext, Skeleton, GLOBAL_TCB_LIST.FindTCB(thread)->DParam, GLOBAL_TCB_LIST.FindTCB(thread)->DStack, GLOBAL_TCB_LIST.FindTCB(thread)->DMemsize);
    
    // Check if the thread needs to yield
    if (foundtcb->DState > GLOBAL_TCB_LIST.DCurrentTCB->DState) {
        VMPrint("/Thread needs to yield\n");
        // Special scheduuling?
        // foundtcb->DEntry(foundtcb->DParam);
        GLOBAL_TCB_LIST.AddToReady(DCurrentTCB);
        GLOBAL_TCB_LIST.DCurrentTCB = foundtcb;
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
    TCB* foundtcb;
    TCB* oldtcb = GLOBAL_TCB_LIST.DCurrentTCB;
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
        foundtcb = NULL;
    }
    if (foundtcb == NULL){
        // Continue on with the idle thread
        MachineContextSwitch(&(oldtcb->DContext), &(GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DContext));
    }
    else{
        MachineContextSwitch(&(oldtcb->DContext), &(GLOBAL_TCB_LIST.DCurrentTCB->DContext));
    }
};
void TCBList::DecrementSleep(){
    for (int threadnum = 0; threadnum < DSleepingList.size(); threadnum++){
        if (DSleepingList[threadnum]->DTicks != 0){
            DSleepingList[threadnum]->DTicks --;

        }
        else {
            VMPrint("context switch from sleep\n");
            GLOBAL_TCB_LIST.AddToReady(DSleepingList[threadnum]);
            //DSleepingList.erase(DSleepingList.begin()+threadnum); //http://www.cplusplus.com/reference/vector/vector/erase/
            GLOBAL_TCB_LIST.Schedule();
            
            //remove it from the sleeping list
            
            
        }
        
    }
}

    /*****************************
    *      MUTEX  STRUCTURE      *
    *****************************/

struct Mutex{
    // Needed for thread create
    static TVMMutexID DTIDCounter;
    TVMMutexID DTID;
    TVMThreadID DOwnerID;
    std::queue<TCB*> DWaitList;

    void Lock(){
        //In use
        if (DOwnerID == -1){
            DOwnerID = GLOBAL_TCB_LIST.DCurrentTCB->DTID;
            GLOBAL_TCB_LIST.DCurrentTCB->DOwnedMutex.push_back(DTID);
            return;
        }
        //Free
        else {
            // Block thread
            DWaitList.push(GLOBAL_TCB_LIST.DCurrentTCB);
            GLOBAL_TCB_LIST.DCurrentTCB->DState=VM_THREAD_STATE_WAITING;
            return;
        }
    }

    void Unlock(){
        if (DWaitList.front() != NULL){
            DOwnerID = DWaitList.front()->DTID;
            Dequeue();
            return;
        }
        else {
            DOwnerID = -1;
            return;
        }
    }
    void Dequeue(){
        DWaitList.pop();
        return;
    }
    void IncrementID(){
        DTIDCounter++;
        return;
    } 
    Mutex(){
        DOwnerID = -1; // Set for noone 
        IncrementID();
        DTID = DTIDCounter;
    }
};
TVMMutexID Mutex::DTIDCounter;

    /*****************************
    *    MUTEX LIST STRUCTURE    *
    *****************************/
struct MutexList{
    // Current Mutex
    Mutex* DCurrentMutex;
    std::vector<Mutex*> DMlist;
    std::queue<TCB*> DHighPrio;
    
    //Functions

    //Scheduler
    //void Schedule();
};

MutexList GLOBAL_MUTEX_LIST = MutexList();




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
    GLOBAL_TICK++; // Should be counting up
    GLOBAL_TCB_LIST.DecrementSleep();
    // GLOBAL_TCB_LIST.DCurrentTCB->DTicks = GLOBAL_TCB_LIST.DCurrentTCB->DTicks - 1; // Already counting in the above statement
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
    TVMStatus VMDateTime(SVMDateTimeRef curdatetime);
    uint32_t VMStringLength(const char *str);
    void VMStringCopy(char *dest, const char *src);
    void VMStringCopyN(char *dest, const char *src, int32_t n);
    void VMStringConcatenate(char *dest, const char *src);
    TVMStatus VMFileSystemValidPathName(const char *name);
    TVMStatus VMFileSystemIsRelativePath(const char *name);
    TVMStatus VMFileSystemIsAbsolutePath(const char *name);
    TVMStatus VMFileSystemGetAbsolutePath(char *abspath, const char *curpath, const char *destpath);
    TVMStatus VMFileSystemPathIsOnMount(const char *mntpt, const char *destpath);
    TVMStatus VMFileSystemDirectoryFromFullPath(char *dirname, const char *path);
    TVMStatus VMFileSystemFileFromFullPath(char *filename, const char *path);
    TVMStatus VMFileSystemConsolidatePath(char *fullpath, const char *dirname, const char *filename);
    TVMStatus VMFileSystemSimplifyPath(char *simpath, const char *abspath, const char *relpath);
    TVMStatus VMFileSystemRelativePath(char *relpath, const char *basepath, const char *destpath);
}



TVMStatus VMStart(int tickms, TVMMemorySize heapsize, TVMMemorySize sharedsize, const char *mount, int argc, char *argv[]){
    //these must be before request alarm and enable si
    
    MachineInitialize(sharedsize);
    MachineRequestAlarm(1000 * tickms, AlarmCallback, NULL);
    MachineEnableSignals();
    //Stuff for mounting the file system.

    
    VMThreadCreate(NULL, NULL, 0x100000, VM_THREAD_PRIORITY_HIGH, &MAIN_ID);
    GLOBAL_TCB_LIST.AddToReady(GLOBAL_TCB_LIST.FindTCB(MAIN_ID));
    
    //create idle thread
    VMThreadCreate(IdleCallback, NULL, 0x100000, ((TVMThreadPriority)0x00), &IDLE_ID); //ID should be 1
    GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DState =  VM_THREAD_STATE_READY;
    MachineContextCreate(&(GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DContext), IdleCallback, NULL, GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DStack, GLOBAL_TCB_LIST.FindTCB(IDLE_ID)->DMemsize);
    
    
    GLOBAL_TCB_LIST.DCurrentTCB = GLOBAL_TCB_LIST.FindTCB(MAIN_ID); //This was giving me problems because it seemed to not have access to the data, not sure, reorganized it to be in Thread membmer fn, seen below

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


TVMStatus VMMutexCreate(TVMMutexIDRef mutex){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    if (mutex == NULL){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    
    GLOBAL_MUTEX_LIST.DMlist.emplace_back(new Mutex()); // Changed the structure of it
    *mutex = Mutex::DTIDCounter;
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;

}
TVMStatus VMMutexDelete(TVMMutexID mutex){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    if (GLOBAL_MUTEX_LIST.DMlist[mutex-1]->DOwnerID != -1){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    if (mutex > Mutex::DTIDCounter){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    
    GLOBAL_MUTEX_LIST.DMlist[mutex-1] = NULL;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}

TVMStatus VMMutexQuery(TVMMutexID mutex, TVMThreadIDRef ownerref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    if (ownerref == NULL){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    if (GLOBAL_MUTEX_LIST.DMlist[mutex-1]->DOwnerID == -1){
        MachineResumeSignals(&localsigs);
        return VM_THREAD_ID_INVALID;
    }
    *ownerref = GLOBAL_MUTEX_LIST.DMlist[mutex-1]->DOwnerID;
    
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMMutexAcquire(TVMMutexID mutexref, TVMTick timeout){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    
    if (GLOBAL_MUTEX_LIST.DMlist[mutexref-1] == NULL){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    if ((timeout == VM_TIMEOUT_IMMEDIATE) && (GLOBAL_MUTEX_LIST.DMlist[mutexref-1]->DOwnerID != -1)){
        MachineResumeSignals(&localsigs);
        return VM_STATUS_FAILURE;
    }
    //Match mutex priority Q with current thread priority Q
    //sleep for specified time
    VMThreadSleep(timeout);
    MachineResumeSignals(&localsigs);
    return VM_STATUS_SUCCESS;
}
TVMStatus VMMutexRelease(TVMMutexID mutexref){
    TMachineSignalState localsigs;
    MachineSuspendSignals(&localsigs);
    //check each Mutex priority Qs, if empty then release them
    for (int i = 0; GLOBAL_TCB_LIST.DCurrentTCB->DOwnedMutex.size(); i++){
        if (GLOBAL_MUTEX_LIST.DMlist[GLOBAL_TCB_LIST.DCurrentTCB->DOwnedMutex[i]]->DOwnerID == mutexref){
            GLOBAL_MUTEX_LIST.DMlist[mutexref]->DOwnerID = -1;
            MachineResumeSignals(&localsigs);
            return VM_STATUS_SUCCESS;
        }
    }
    MachineResumeSignals(&localsigs);
    return VM_STATUS_ERROR_INVALID_ID;
}

#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
