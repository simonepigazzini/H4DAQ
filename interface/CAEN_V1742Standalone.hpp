#ifndef CAEN_V1742STANDALONE_H
#define CAEN_V1742STANDALONE_H

#include "interface/StandardIncludes.hpp"
#include "interface/Board.hpp"
#include "interface/BoardConfig.hpp"
#include "interface/CAEN_V1742.hpp"


class CAEN_V1742Standalone: public CAEN_V1742, public TriggerBoard, public IOControlBoard 
{
public:
    CAEN_V1742Standalone(): CAEN_V1742(), TriggerBoard(), IOControlBoard()  { type_="CAEN_V1742Standalone" ; } 
    
    virtual int Read (vector<WORD> &v) ;
    
    //Main functions to handle the event trigger
    virtual int SetBusyOn();
    virtual int SetBusyOff();
    virtual bool TriggerReceived();
    virtual int TriggerAck();
    virtual inline bool  SignalReceived(CMD_t signal) { return true; };
    virtual int SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus);

private:
    uint32_t bufferSize_;
};

#endif
