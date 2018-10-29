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

TMachineSignalStateRef GlobalSignal;
// The Thread control block. One is made for each thread
struct TCB {
    TVMThreadEntry entry; // Entry point, what function we will point to
    void * param;
    TVMTick ticks;
    TVMThreadPriority prio;
    TVMThreadID ThreadID;
    TVMThreadState state;
    uint8_t *stack; // Not sure if this is correct stack base
    void SetState(TVMThreadState state);
    TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ID, uint8_t stack);
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
    int i = 0;
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
// std::list <TVMThreadID*> sleepingThreads;

//TA says list necessary, shaky on why, maybe b/c mem non contiguous
//non contig ref:  https://techdifferences.com/difference-between-contiguous-and-non-contiguous-memory-allocation.html
//So this can be used for sleeping threads existing in non-contigous threads, i.e. thread 1,5 alseep @far away mem locs


/*
void TCB::TCB(){

}
*/
extern "C" {
        //typedef void (*TVMMainEntry)(int, char*[]);  //This is why were couldn't access the fn in main LOL
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
TVMStatus VMStart(int tickms, int argc, char *argv[]){

	// Returns Null if fails to load
	MachineInitialize();
	MachineEnableSignals();
	TVMMainEntry entry = VMLoadModule(argv[0]);
	if (entry == NULL) {
		std::cout << "Failed to load \n";
	}
	else{
		//std::cout << "Loaded module \n";
	}

        //create idle and main threads
        //idle
    
        //TVMThreadEntry tentry = *argv[0];
        TVMThreadEntry tentry = NULL;

        //Creates Main thread

        //Creates Idle Thread
    TVMThreadID idleID = VM_THREAD_ID_INVALID; // decrements the thread ID

    
    //Create main thread, but we don't want to disable signal
    TVMThreadID maintid;
    TVMMemorySize memorysize = 0x100000;
    TVMThreadPriority mainpriority = VM_THREAD_PRIORITY_NORMAL;
    TCB *maintcb = new TCB(AlarmCallback, NULL, mainpriority, maintid, memorysize);
    globalList.AddTCB(maintcb);
    
    //Create Idle Thread
    //TVMThreadID idleID = VM_THREAD_ID_INVALID; // decrements the thread ID
    VMThreadCreate(AlarmCallback, NULL, memorysize,  ((TVMThreadPriority)0x01), &idleID);
    std::cout<<"Threads | idle"<<"\n";

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
    MachineSuspendSignals(GlobalSignal); //suspend threads so we can run
    if ((entry == NULL) || (tid == NULL)){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    //TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack);
    TCB NewTCB = TCB(entry, param, prio, globalList.IncrementID(), memsize);
    globalList.AddTCB(&NewTCB);
    // Add it to the list
    MachineResumeSignals(GlobalSignal);
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
    MachineSuspendSignals(GlobalSignal);
	TCB* FoundTCB = globalList.FindTCB(thread);
    if (FoundTCB == NULL){
        MachineResumeSignals(GlobalSignal);
        return VM_STATUS_ERROR_INVALID_ID;
    }
    else if (FoundTCB->state != VM_THREAD_STATE_DEAD){
        MachineResumeSignals(GlobalSignal);
        return VM_STATUS_ERROR_INVALID_STATE;
    }
    else {
        FoundTCB->SetState(VM_THREAD_STATE_READY);
        switch (FoundTCB->prio)
        {
            case VM_THREAD_PRIORITY_LOW:
                globalList.LowReady.push_back(FoundTCB);
                break;
            case VM_THREAD_PRIORITY_NORMAL:
                globalList.MediumReady.push_back(FoundTCB);
                break;
            case VM_THREAD_PRIORITY_HIGH:
                globalList.HighReady.push_back(FoundTCB);
                break;
        }
        MachineResumeSignals(GlobalSignal);
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
	TMachineSignalStateRef state;
	if ((filename == NULL) || (filedescriptor == NULL)){
		std::cout << "Invalid FD | filename" << '\n';
		return VM_STATUS_ERROR_INVALID_PARAMETER;
	}
	std::cout << "Calling machine file open" << '\n';
	MachineSuspendSignals(state);
	MachineFileOpen(filename, flags, mode, FileDescriptorCallback, filedescriptor);
	MachineResumeSignals(state);

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
