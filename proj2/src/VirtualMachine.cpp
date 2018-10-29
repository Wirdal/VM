#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>

/*
 class VMThread{
 //MachineContextCreate() //what args to pass?
 
 }
 */
//globals
SMachineContext contextGlob;



//TMachineSignalStateRef GlobalSignal; not supposed to be global
// The Thread control block. One is made for each thread
struct TCB {
    TVMThreadEntry entry; // Entry point, what function we will point to
    void * param;
    TVMTick ticks;
    TVMThreadPriority prio;
    TVMThreadID ThreadID;
    TVMThreadState state;
    TVMMemorySize memorysize = 0x100000;
    uint8_t *stackaddr = new uint8_t[memorysize]; //to use in machine context create
    uint8_t *stack; // Not sure if this is correct stack base
    void SetState(TVMThreadState state);
    TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ID, uint8_t stack);
    SMachineContext *TCBcontext = new SMachineContext; //The TCBs context, used in MachineContextCreate

    //~TCB();
};

//Sets the state of the thread
void TCB::SetState(TVMThreadState state){
    state=state;
}

//The constructor for the TCB.
TCB::TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ID, uint8_t stack){
    entry = entry;
    ticks = 0; //Not sure about this one
    param = param;
    prio = prio;
    ThreadID = ID; // Have to increment locally. This is already done
    state = VM_THREAD_STATE_DEAD;
    stack = *new uint8_t[stack];
}
//Deconstructor for a thread control block. Is meant to free memory up
// Not sure if it works rigth now though
// TCB::~TCB(){
//     delete stack;
// }
struct TCBList{
    TCB* CurrentTCB;
    std::vector<TCB*> Tlist;
    TVMThreadID IDCounter;
    TCB* FindTCB(TVMThreadID IDnum);
    TVMThreadID IncrementID();
    std::vector<TCB*> GetList();
    TVMThreadID GetID();
    void AddTCB(TCB*);
    void RemoveTCB(TVMThreadID IDnum);
    TVMThreadID NextReadyThread();
    void SetCurrentThread(TCB* CurrentTCB);
    void SetCurrentThread(TVMThreadID ID);
    void RemoveFromReady(TVMThreadID IDnum);
    TCB* GetCurrentTCB();
    std::vector<TCB*> SleepingThreads;
    std::vector<TCB*> HighReady;
    std::vector<TCB*> MediumReady;
    std::vector<TCB*> LowReady;
    void AddSleeper();
};

//Tells the current thread to go to sleep.
void TCBList::AddSleeper(){
    SleepingThreads.push_back(CurrentTCB);
    RemoveFromReady(CurrentTCB->ThreadID);
    
}

void TCBList::SetCurrentThread(TCB* CurrentTCB){
    CurrentTCB = CurrentTCB;
}
void TCBList::SetCurrentThread(TVMThreadID ID){
    CurrentTCB = FindTCB(ID);
}

TCB* TCBList::GetCurrentTCB(){
    return CurrentTCB;
}

TCB* TCBList::FindTCB(TVMThreadID IDnum){
    for(auto s: Tlist)
        if (s->ThreadID == IDnum){
            return s;
        }
    return NULL;
}

TVMThreadID TCBList::IncrementID() {
    return IDCounter = IDCounter + 1;
}

void TCBList::RemoveFromReady(TVMThreadID IDnum){
    if (CurrentTCB->ThreadID == IDnum){
        CurrentTCB = NULL;
    }
    int i = 0;
    for(auto s: LowReady)
        if (s->ThreadID == IDnum){
            LowReady.erase(LowReady.begin()+ i); // Need to remove it from the schedular as well?
            return;
        }
    ++i;
    i = 0;
    for(auto s: MediumReady)
        if (s->ThreadID == IDnum){
            MediumReady.erase(MediumReady.begin()+ i); // Need to remove it from the schedular as well?
            return;
        }
    ++i;
    i = 0;
    for(auto s: HighReady)
        if (s->ThreadID == IDnum){
            HighReady.erase(HighReady.begin()+ i); // Need to remove it from the schedular as well?
            return;
        }
    ++i;
}

TVMThreadID TCBList::NextReadyThread(){
    //Finds the next ready thread in the schedular
    // Returns the ID to it
    //Also removes the thread from the block
    if (HighReady.empty()) {
        if (MediumReady.empty()){
            if (LowReady.empty()){
                return 0;
            }
            else{
                return LowReady[0]->ThreadID;
            }
        }
        else {
            return MediumReady[0]->ThreadID;
        }
    }
    else{
        return HighReady[0]->ThreadID;
    }
}

TVMThreadID TCBList::GetID(){
    return IDCounter;
}

std::vector<TCB*> TCBList::GetList(){
    return Tlist;
}

void TCBList::AddTCB(TCB *TCB){
    Tlist.push_back(TCB);
}

void TCBList::RemoveTCB(TVMThreadID IDnum){
    // Removes the current running thread if it is the one
    if (CurrentTCB->ThreadID == IDnum){
        CurrentTCB = NULL;
    }
    int i = 0;
    for(auto s: Tlist)
        if (s->ThreadID == IDnum){
            Tlist.erase(Tlist.begin()+ i); // Need to remove it from the schedular as well?
            return;
        }
    ++i;
}

TCBList globalList = TCBList();
TVMThreadID globid;
// std::list <TVMThreadID*> sleepingThreads;

//TA says list necessary, shaky on why, maybe b/c mem non contiguous
//non contig ref:  https://techdifferences.com/difference-between-contiguous-and-non-contiguous-memory-allocation.html
//So this can be used for sleeping threads existing in non-contigous threads, i.e. thread 1,5 alseep @far away mem locs


/*
 void TCB::TCB(){
 
 }
 */
extern "C" {
    typedef void (*TVMMainEntry)(int, char*[]);  //This is why were couldn't access the fn in main LOL
    TVMMainEntry VMLoadModule(const char *module);
    void VMUnloadModule(void);
    TVMStatus VMFilePrint(int filedescriptor, const char *format, ...);
}

void AlarmCallback(void *calldata){
    //calldata - passed into the callback function upon completion of the open file request
    //calldata - received from MachineFileOpen()
    //result - new file descriptor
    ;
}



/*!
 VMStart() starts the virtual machine by loading the module specified by argv [0]. The argc and argv
 are passed directly into the VMMain() function that exists in the loaded module. The time
 in milliseconds of the virtual machine tick is specified by the tickms parameter.
 */
void IdleCallback(void *calldata){
    MachineEnableSignals();
    std::cout<<"**In Idle"<<"\n";
    VMPrint("***In Idle\n");
    /*
    std::cout<<"In Idle"<<"\n";
    MachineEnableSignals();
    while(1){
        std::cout<<"In Idle"<<"\n";
    }
    */
    /*
    while(1){
        std::cout<<"In Idle"<<"\n";
        VMPrint("***In Idle\n");
    }
     */
    ;
}
void VMThread(void *param){
    TMachineSignalStateRef signalref;
    MachineSuspendSignals(signalref);
    //MachineEnableSignals();


    VMPrint("***Sig Enabled\n");
    VMPrint("****In Thread Callback\n");
    //VMPrint("VMThread Alive\n");
    //VMThreadSleep(10);
    //VMPrint("VMThread Awake\n");
    MachineResumeSignals(signalref);

}
TVMStatus VMStart(int tickms, int argc, char *argv[]){
    std::cout<<"/VMStart"<<"\n";
    VMPrint("Machine not initialized, will not print\n"); //wont print
    
    MachineInitialize();
    VMPrint("\nMachine Initialized\n");
    MachineEnableSignals();
    VMPrint("Sig Enabled\n");
    
    // Returns Null if fails to load
    TVMMainEntry entry = VMLoadModule(argv[0]);
    if (entry == NULL) {
        std::cout << "Failed to load \n";
    }
    else{
        std::cout << "Loaded module \n";
    }
    

    //TVMThreadID idleID = VM_THREAD_ID_INVALID; // decrements the thread ID
    TVMThreadID idleID;
    TVMThreadID maintid;
    
    //Create main thread, but we don't want to disable signal
    
    /*
    std::cout<<"/Create VMTHREAD Thread"<<"\n";
    VMPrint("*VM creating Example thread\n");
    TVMThreadID VMThreadID1;
    
    VMThreadCreate(VMThread, NULL, 0x100000, VM_THREAD_PRIORITY_NORMAL, &VMThreadID1);
    VMThreadActivate(VMThreadID1);
    TVMThreadState VMState = VM_THREAD_STATE_READY;
    
    TCB* FoundTCB = globalList.FindTCB(VMThreadID1);
    if (FoundTCB == NULL){
        VMPrint("NO TCB ex\n");
        MachineResumeSignals(GlobalSignal);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else{VMPrint("TCB FOUND ex\n");}
    
    VMPrint("VMThread State: ");
    switch(VMState){
        case VM_THREAD_STATE_DEAD:       VMPrint("DEAD\n");
            break;
        case VM_THREAD_STATE_RUNNING:    VMPrint("RUNNING\n");
            break;
        case VM_THREAD_STATE_READY:      VMPrint("READY\n");
            break;
        case VM_THREAD_STATE_WAITING:    VMPrint("WAITING\n");
            break;
        default:                        break;
    }
    std::cout<<"/Activate VMTHREAD Thread"<<"\n";
    VMPrint("VMThread activate\n");
    VMThreadActivate(VMThreadID1);
    */
    
    
    //Idle Thread
    //id = 0 - always
    //TVMThreadID idleID = VM_THREAD_ID_INVALID; // decrements the thread ID
    // TVMThreadPriority 0x00 -> lower than low (0x01)
    std::cout<<"/Create idle Thread"<<"\n";
    VMPrint("VM creating idle thread\n");
    
    TVMMemorySize memorysize = 0x100000;
    VMThreadCreate(IdleCallback, NULL, memorysize,  ((TVMThreadPriority)0x00), &idleID);
    //MachineContextCreate(
    std::cout<<"Activate idle Thread [id: "<< idleID << &idleID<<"\n";
    VMThreadActivate(1); //idleID = 1
    
    
    //main thread
    //id - 1
    std::cout<<"\n";
    std::cout<<"/Create main Thread"<<"\n";
    VMPrint("VM creating main thread\n");
    TVMThreadPriority mainpriority = VM_THREAD_PRIORITY_NORMAL;
    TCB *maintcb = new TCB(IdleCallback, NULL, mainpriority, maintid, memorysize);
    globalList.AddTCB(maintcb);

    

    MachineEnableSignals();
    entry(argc, argv);
    MachineTerminate();
    VMUnloadModule();
    return VM_STATUS_SUCCESS;
    
};


/*!
 VMTickMS() puts tick time interval in milliseconds in the location specified by tickmsref.
 This is the value tickmsfrom the previous call to VMStart().
 */
TVMStatus VMTickMS(int *tickmsref){
    if(tickmsref){
        return VM_STATUS_SUCCESS;
    }
    else{
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
};

/*
 VMTickCount() puts the number of ticks that have occurred since the start of the virtual machine in the location specified by tickref.
 */
TVMStatus VMTickCount(TVMTickRef tickref){
    
};

/*
 VMThreadCreate() creates a thread in the virtual machine.Once created the thread is in the dead state VM_THREAD_STATE_DEAD.
 The entryparameter specifies the function of the thread, and paramspecifies the parameter that is passed to the function.
 The size of the threads stack is specified by memsize, and the priority is specified by prio.
 The thread identifier is put into the location specified by the tidparameter.
 */
TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TMachineSignalStateRef signalref;
    std::cout<<"/VMThreadCreate"<<"\n";
    VMPrint("Create thread\n");
    MachineSuspendSignals(signalref); //suspend threads so we can run
    //VMPrint("Create thread\n");
    if ((entry == NULL) || (tid == NULL)){
        VMPrint("NO Create\n");
        return VM_STATUS_ERROR_INVALID_PARAMETER;
        
    }
    //TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack);
    
    //TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ID, uint8_t stack);
    ++globid;           //ex 1 -> 2
    tid = &globid;      //new tid (reference) is location of the new id
    std::cout<<"globalid: "<<globid<<"\n";
    
    TCB *NewTCB = new TCB(entry, param, prio, *tid, memsize);
    globalList.AddTCB(NewTCB);
    // Add it to the list
    MachineResumeSignals(signalref);
    return VM_STATUS_SUCCESS;
    //VMThreadState(tid, running);
    //Allocate space for thread
};

/*
 VMThreadDelete()deletes the dead thread specified by threadparameter from the virtual machine.
 */
TVMStatus VMThreadDelete(TVMThreadID thread){
    TCB* FoundTCB = globalList.FindTCB(thread);
    if (FoundTCB == NULL){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (FoundTCB->state != VM_THREAD_STATE_DEAD){
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    else {
        // Remove the thread from the scheduler too
        globalList.RemoveTCB(thread);
        return VM_STATUS_SUCCESS;
    }
};

/*
 VMThreadActivate()activates the dead thread specified by threadparameter in the virtual machine.
 After activation the thread enters the ready state VM_THREAD_STATE_READY, and must begin at the entryfunction specified.
 */
TVMStatus VMThreadActivate(TVMThreadID thread){
    std::cout<<"/VMThreadActivate"<<"\n";
    VMPrint("\nACTIVATING\n");
    TMachineSignalStateRef signalref;
    MachineSuspendSignals(signalref);
    
    TCB* FoundTCB = globalList.FindTCB(thread);
    if (FoundTCB == NULL){
        VMPrint("NO TCB\n");
        MachineResumeSignals(signalref);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (FoundTCB->state != VM_THREAD_STATE_DEAD){
        VMPrint("TCB Not dead\n");
        MachineResumeSignals(signalref);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    else {
        VMPrint("Else (TCB dead)\n");
        MachineContextCreate(globalList.FindTCB(thread)->TCBcontext, IdleCallback, NULL,  globalList.FindTCB(thread)->stackaddr,0x100000);
        FoundTCB->state = VM_THREAD_STATE_READY;
        globalList.FindTCB(thread)->state = VM_THREAD_STATE_READY;
        switch (globalList.FindTCB(thread)->prio)
        {
            case VM_THREAD_PRIORITY_LOW:
                VMPrint("low\n");
                globalList.LowReady.push_back(FoundTCB);
                break;
            case VM_THREAD_PRIORITY_NORMAL:
                VMPrint("norm\n");
                globalList.MediumReady.push_back(FoundTCB);
                break;
            case VM_THREAD_PRIORITY_HIGH:
                VMPrint("high\n");
                globalList.HighReady.push_back(FoundTCB);
                break;
            case ((TVMThreadPriority)0x00):
                VMPrint("idle\n");
                FoundTCB->state = VM_THREAD_STATE_RUNNING;
                break;
            default:
                VMPrint("no prio\n");
                break;
                
        }
       // globalList.FindTCB(thread)->state = VM_THREAD_STATE_READY;
        //FoundTCB->state = VM_THREAD_STATE_READY;
        switch (globalList.FindTCB(thread)->state)
        {
            case VM_THREAD_STATE_READY:
                VMPrint("Thread Ready\n");
                globalList.LowReady.push_back(FoundTCB);
                break;
            case VM_THREAD_STATE_RUNNING:
                VMPrint("Thread idle\n");
                globalList.LowReady.push_back(FoundTCB);
                break;
            default:
                VMPrint("setstate failed\n");
                break;
                
        }
        //switch old to new
        MachineContextSwitch(globalList.FindTCB(thread - 1)->TCBcontext,globalList.FindTCB(thread)->TCBcontext);
        MachineResumeSignals(signalref);
        return VM_STATUS_SUCCESS;
    }
};

/*
 VMThreadTerminate()terminates the thread specified by threadparameter in the virtual machine.
 After termination the thread entersthe state VM_THREAD_STATE_DEAD.
 The termination of a thread can triMachineFileWritegger another thread to be scheduled.
 */
TVMStatus VMThreadTerminate(TVMThreadID thread){
    TCB* FoundTCB = globalList.FindTCB(thread);
    if (FoundTCB == NULL){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (FoundTCB->state != VM_THREAD_STATE_DEAD){
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    else {
        // Also need to stop it running
        FoundTCB->SetState(VM_THREAD_STATE_DEAD);
        // Move it to dead
        return VM_STATUS_SUCCESS;
    }
};

/*
 VMThreadID() puts the thread identifier of the currently running thread in the location specified by threadref.
 */
TVMStatus VMThreadID(TVMThreadIDRef threadref){
    //No idea how to do this one
    //Needs to be current operating thread?
    threadref = &globalList.GetCurrentTCB()->ThreadID;
};

/*
 VMThreadState() retrieves the state of the thread specified by threadand places the state in the location specified by state.
 */
TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
    TCB* FoundTCB = globalList.FindTCB(thread);
    if (FoundTCB == NULL){
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (stateref == NULL){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    else{
        stateref = &FoundTCB -> state;
        return VM_STATUS_SUCCESS;
    }
};

/*
 VMThreadSleep() puts the currently running thread to sleep for tickticks.
 If tick is specified as VM_TIMEOUT_IMMEDIATEthe current process yields the remainder of its processing quantum to the next ready process of equal priority.
 */
TVMStatus VMThreadSleep(TVMTick tick){
    
    
    
};

/*
 VMFileOpen()attempts to open the file specified by filename, using the flags specified by flagsparameter, and mode specified by modeparameter.
 The file descriptor of the newly opened file will be placed in the location specified by filedescriptor.
 The flags and mode values follow the same format as that of open system call.
 The filedescriptor returned can be used in subsequent calls to VMFileClose(), VMFileRead(), VMFileWrite(), and VMFileSeek().
 When a thread calls VMFileOpen() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful opening of the file is completed.
 */
void FileDescriptorCallback(void* calldata, int result){
    // Used as a callback to get the FD from the machinefileopen
    std::cout << "result here " << result << "\n";
    calldata = &result;
}

TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
    TMachineSignalStateRef signalref;
    if ((filename == NULL) || (filedescriptor == NULL)){
        std::cout << "Invalid FD | filename" << '\n';
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    std::cout << "Calling machine file open" << '\n';
    MachineSuspendSignals(signalref);
    MachineFileOpen(filename, flags, mode, FileDescriptorCallback, filedescriptor);
    MachineResumeSignals(signalref);
    
};
void EmptyCallback2(void *calldata, int result){
    //calldata - passed into the callback function upon completion of the open file request
    //calldata - received from MachineFileOpen()
    //result - new file descriptor
    ;
}

/*
 VMFileClose() closes a file previously opened with a call to VMFileOpen().
 When a thread calls VMFileClose() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful closing of the file is completed.
 */
TVMStatus VMFileClose(int filedescriptor){
    MachineFileClose(filedescriptor, EmptyCallback2, NULL);
};

/*
 VMFileRead() attempts to read the number of bytes specified in the integer referenced by lengthinto the location specified by datafrom the file specified by filedescriptor.
 The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The actual number of bytes transferred by the read will be updated in the lengthlocation.
 When a thread calls VMFileRead() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful reading of the file is completed.
 */
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){
};

/*
 VMFileWrite() attempts to write the number of bytes specified in the integer referenced by length from the location specified by data to the file specified by filedescriptor.
 The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The actual number of bytes transferred by the write will be updated in the lengthlocation.
 When a thread calls VMFileWrite() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful writing of the file is completed.
 */


void EmptyCallback(void *calldata, int result){
    //calldata - passed into the callback function upon completion of the open file request
    //calldata - received from MachineFileOpen()
    //result - new file descriptor
    ;
}

TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){
    //Just a call to other things?
    //TMachineFileCallback callback;
    //get filedescriptor from VMFILEOPEN()
    //void *data;
    //Suspend signals before here? Don't want race
    MachineFileWrite(filedescriptor, data, *length, EmptyCallback, NULL);
    
};

/*
 VMFileSeek() attempts to seek the number of bytes specified by offsetfrom the location specified by whencein the file specified by filedescriptor.
 The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The new offset placed in the newoffsetlocation if the parameter is not NULL.
 When a thread calls VMFileSeek() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful seeking inthe file is completed.
 */
TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){
    
};

#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
