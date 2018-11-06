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
 * **************************/

// Thread Control Block

struct TCB{
    // Needed for thread create
    TVMThreadEntry DEntry;
    void * DParam;
    TVMMemorySize DMemsize;
    TVMThreadPriority DPrio;
    TVMThreadID DTID;
    static TVMThreadID DTIDCounter;
    uint8_t DStack;

    // What will get set later/ don't care about at the time
    int DTicks;
    int DFd;
    TVMThreadState DSTate;


    // Constructor
    TCB(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid);
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
    DStack = *new uint8_t [memsize];
    tid = &DTID;
    DTicks = 0;
    DFd = 0;
    DSTate = VM_THREAD_STATE_DEAD;

};

void TCB::IncrementID(){
    ++DTIDCounter;
};

struct TCBList{
    std::vector<TCB*> DTList;
    std::queue<TCB*> DHighPrio;
    std::queue<TCB*> DMedPrio;
    std::queue<TCB*> DLowPrio;
};

/*****************************
 *Any global structs are here*
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

};

TVMStatus VMTickMS(int *tickmsref){

};
TVMStatus VMTickCount(TVMTickRef tickref){

};

TVMStatus VMThreadCreate(TVMThreadEntry entry, void *param, TVMMemorySize memsize, TVMThreadPriority prio, TVMThreadIDRef tid){
    TCB newTCB = TCB(entry, param, memsize, prio, tid);
    // Add it to the list as well
};
TVMStatus VMThreadDelete(TVMThreadID thread){

};
TVMStatus VMThreadActivate(TVMThreadID thread){

};
TVMStatus VMThreadTerminate(TVMThreadID thread){

};
TVMStatus VMThreadID(TVMThreadIDRef threadref){

};
TVMStatus VMThreadState(TVMThreadID thread, TVMThreadStateRef stateref){

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

