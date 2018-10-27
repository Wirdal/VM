#include <VirtualMachine.h>
#include <Machine.h>
#include <iostream>
#include <vector>
#include <array>

/*
class VMThread{
    //MachineContextCreate() //what args to pass?

}
*/

struct TCB {
    TVMThreadEntry entry; // Entry point, what function we will point to
    void * param;
    TVMThreadPriority prio;
    TVMThreadID ThreadID;
    TVMThreadState state;
    uint8_t stack; // Not sure if this is correct stack base
    TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack);
    // TCB()
};

TCB::TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack){
    entry = entry;
    param = param;
    prio = prio;
    ThreadID = 0; // needs to be assigned later
    state = state;
    stack = stack;
}

// This neeeds to be created ONCE
struct TCBList{
    std::vector<TCB*> Tlist;
    static TVMThreadID IDCounter;
    TCB* FindTCB(TVMThreadID IDnum);
    static TVMThreadID IncrementID();
    std::vector<TCB*> GetList();
    static TVMThreadID GetID();
    void AddTCB(TCB*);
};

TCB* TCBList::FindTCB(TVMThreadID IDnum){
    for(auto s: Tlist)
        if (s->ThreadID == IDnum){
            return s;
        }
}

TVMThreadID TCBList::IncrementID() {
    return IDCounter = IDCounter + 1;
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
// GlobalTCBList = new TCBList;
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
        unsigned int* maintid;
        TVMMemorySize memorysize = 0x100000;
        //TVMThreadEntry tentry = *argv[0];
        TVMThreadEntry tentry = NULL;

        //mainetry = entry;

        //TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack);
        // TCB()
        TVMThreadID maintid = 0;
        TVMThreadPriority mainpriority = VM_THREAD_PRIORITY_NORMAL;

        //Creates Main thread
        TCB maintcb = TCB(entry, NULL, mainpriority, maintid, VM_THREAD_STATE_RUNNING, memorysize);

        //Creates Idle Thread
        TVMThreadID idleID = VM_THREAD_ID_INVALID; // decrements the thread ID
        VMThreadCreate(AlarmCallback, NULL, memorysize, VM_THREAD_PRIORITY_NORMAL, idleID);
        //VMThreadActivate idle ID

        //CREATE IDLE THREAD
        //TVMThreadIDRef idleThreadID;

        std::cout<<"Threads | idle: id"<<"\n";
        //returns immediated, alrarmcb called by machine
        //tickms is the param for the AlarmCallback
        //alarmcb(calldata) - called every tick, callback param can be NULL here
        //sleeping thread gets decremented / other cases increment all (including sleeping thread)?
        // MachineRequestAlarm(tickms * 1000, AlarmCallback, NULL);


        //machineenablesignals
	// Just need to pass VMmain its arguements?
	// Or need to call what is returned


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
VMThreadCreate() creates a thread in the virtual machine.Once created the thread is in the dead stateVM_THREAD_STATE_DEAD.
The entryparameter specifies the function of the thread, and paramspecifies the parameter that is passed to the function.
The size of the threads stack is specified by memsize, and the priority is specified by prio.
The thread identifier is put into the location specified by the tidparameter.
*/
TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    if ((entry == NULL) || (tid == NULL)){
        return VM_STATUS_ERROR_INVALID_PARAMETER;
    }
    //TCB(TVMThreadEntry entry, void * param, TVMThreadPriority prio, TVMThreadID ThreadID, TVMThreadState state, uint8_t stack);
    TVMThreadID thread_id = TCBList::IDCounter;
    TCB NewTCB = TCB(entry, param, prio, thread_id, VM_THREAD_STATE_READY, memsize);
    //GlobalTCBList.
    // Add it to the list
    return VM_STATUS_SUCCESS;
    //VMThreadState(tid, running);
	//Allocate space for thread
};

/*
VMThreadDelete()deletes the dead thread specified by threadparameter from the virtual machine.
*/
TVMStatus VMThreadDelete(TVMThreadID thread){

};

/*
VMThreadActivate()activates the dead thread specified by threadparameter in the virtual machine.
After activation the thread enters the ready state VM_THREAD_STATE_READY, and must begin at the entryfunction specified.
*/
TVMStatus VMThreadActivate(TVMThreadID thread){
	//init context
};

/*
VMThreadTerminate()terminates the thread specified by threadparameter in the virtual machine.
After termination the thread entersthe state VM_THREAD_STATE_DEAD.
The termination of a thread can triMachineFileWritegger another thread to be scheduled.
*/
TVMStatus VMThreadTerminate(TVMThreadID thread){

};

/*
VMThreadID() puts the thread identifier of the currently running thread in the location specified by threadref.
*/
TVMStatus VMThreadID(TVMThreadIDRef threadref){


};

/*
VMThreadState() retrieves the state of the thread specified by threadand places the state in the location specified by state.
*/
TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){
    //get TCB from ID
    // stateref = tcb -> state;
    // return succ


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
