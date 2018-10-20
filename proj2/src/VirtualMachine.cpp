#include <VirtualMachine.h>
<<<<<<< HEAD
#include <Machine.h>

extern "C" {
  TVMMainEntry VMLoadModule(const char *module);
}

=======

//Define Thread Control Block (TCB)

//function prototypes to use functions from VirtualMachineUtils.c

extern "C"{
// Prototype here
}


>>>>>>> 21c147e901dfcdc413823e450cfc703c631d32f5
/*!
VMStart() starts the virtual machine by loading the module specified by argv [0]. The argc and argv
are passed directly into the VMMain() function that exists in the loaded module. The time
in milliseconds of the virtual machine tick is specified by the tickms parameter.
*/
TVMStatus VMStart(int tickms, int argc, char *argv[]){
    MachineInitialize();
    TVMMainEntry entry = VMLoadModule(argv[0]);
    return 0;
};

/*!
Upon successful loading and running of the VMMain() function, VMStart() will return
VM_STATUS_SUCCESS after VMMain() returns. If the module fails to load,
or the module does not contain a VMMain() function, VM_STATUS_FAILURE is returned.
*/


/*!
VMTickMS() puts tick time interval in milliseconds in the location specified by tickmsref.
This is the value tickmsfrom the previous call to VMStart().
*/
TVMStatus VMTickMS(int *tickmsref){

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

};

/*
VMThreadTerminate()terminates the thread specified by threadparameter in the virtual machine.
After termination the thread entersthe state VM_THREAD_STATE_DEAD.
The termination of a thread can trigger another thread to be scheduled.
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

};

/*
VMThreadSleep() puts the currently running thread to sleep for tickticks.
If tick is specified as VM_TIMEOUT_IMMEDIATEthe current process yields the remainder of its processing quantum to the next ready process of equal priority.
*/
TVMStatus VMThreadSleep(TVMTick tick){

};

/*
MFileOpen()attempts to open the file specified by filename, using the flags specified by flagsparameter, and mode specified by modeparameter.
The file descriptor of the newly opened file will be placed in the location specified by filedescriptor.
The flags and mode values follow the same format as that of open system call.
The filedescriptor returned can be used in subsequent calls to VMFileClose(), VMFileRead(), VMFileWrite(), and VMFileSeek().
When a thread calls VMFileOpen() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful opening of the file is completed.
*/
TVMStatus VMFileOpen(const char *filename, int flags, int mode, int *filedescriptor){
//MachineFileOpen from Machine.cpp probably used
};

/*
VMFileClose() closes a file previously opened with a call to VMFileOpen().
When a thread calls VMFileClose() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful closing of the file is completed.
*/
TVMStatus VMFileClose(int filedescriptor){

};

/*
VMFileRead() attempts to read the number of bytes specified in the integer referenced by lengthinto the location specified by datafrom the file specified by filedescriptor.
The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The actual number of bytes transferred by the read will be updated in the lengthlocation.
When a thread calls VMFileRead() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful reading of the file is completed.
*/
TVMStatus VMFileRead(int filedescriptor, void *data, int *length){

};

/*
VMFileWrite() attempts to write the number of bytes specified in the integer referenced by lengthfrom the location specified by datato the file specified by filedescriptor.
The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The actual number of bytes transferred by the write will be updated in the lengthlocation.
When a thread calls VMFileWrite() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful writing of the file is completed.
*/
TVMStatus VMFileWrite(int filedescriptor, void *data, int *length){

    return 0;
};

/*
VMFileSeek() attempts to seek the number of bytes specified by offsetfrom the location specified by whencein the file specified by filedescriptor.
The filedescriptorshould have been obtained by a previous call to VMFileOpen(). The new offset placed in the newoffsetlocation if the parameter is not NULL.
When a thread calls VMFileSeek() it blocks in the wait state VM_THREAD_STATE_WAITING until the either successful or unsuccessful seeking inthe file is completed.
*/
TVMStatus VMFileSeek(int filedescriptor, int offset, int whence, int *newoffset){

};

//#define VMPrint(format, ...)        VMFilePrint ( 1,  format, ##__VA_ARGS__)
//#define VMPrintError(format, ...)   VMFilePrint ( 2,  format, ##__VA_ARGS__)
