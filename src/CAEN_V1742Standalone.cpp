#include "interface/CAEN_V1742Standalone.hpp"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <bitset>

int CAEN_V1742Standalone::Read (vector<WORD> &v)
{
    ostringstream s;
    CAEN_DGTZ_ErrorCode ret=CAEN_DGTZ_Success ;
    ERROR_CODES ErrCode= ERR_NONE ;
    CAEN_DGTZ_EventInfo_t       EventInfo ;
    
    if (bufferSize_==0)
    {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::NULL BUFFER SIZE!!!";
        Log(s.str(),1);
        
        ErrCode = ERR_READOUT ;
        return ErrCode ;
    }
    
    // /* Analyze data */
    // for (i = 0 ; i < (int)NumEvents ; i++) {
    /* Get one event from the readout buffer */
    ret = CAEN_DGTZ_GetEventInfo (digitizerHandle_, buffer_, bufferSize_, 0, &EventInfo, &eventPtr_) ;
    if (ret) {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";
        Log(s.str(),1);
        
        ErrCode = ERR_EVENT_BUILD ;
        return ErrCode ;
    }
    ret = CAEN_DGTZ_DecodeEvent (digitizerHandle_, eventPtr_, (void**)&event_) ;
    if (ret) {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";
        Log(s.str(),1);
        
        ErrCode = ERR_EVENT_BUILD ;
        return ErrCode ;
    }    
    // if (digitizerConfiguration_.useCorrections != -1) { // if manual corrections
    //     ApplyDataCorrection ( 0, digitizerConfiguration_.useCorrections, digitizerConfiguration_.DRS4Frequency, & (event_->DataGroup[0]), &Table_gr0) ;
    //     ApplyDataCorrection ( 1, digitizerConfiguration_.useCorrections, digitizerConfiguration_.DRS4Frequency, & (event_->DataGroup[1]), &Table_gr1) ;
    //       }
    ret = (CAEN_DGTZ_ErrorCode) writeEventToOutputBuffer (v,&EventInfo,event_) ;
    if (ret) {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::EVENT BUILD ERROR!!!";
        Log(s.str(),1);
        
        ErrCode = ERR_EVENT_BUILD ;
        return ErrCode ;
    }    
    // } //close cycle over events    
    return 0 ;
} ;

bool CAEN_V1742Standalone::TriggerReceived()
{
    CAEN_DGTZ_ErrorCode ret=CAEN_DGTZ_Success ;
    ERROR_CODES ErrCode= ERR_NONE ;
    
    uint32_t NumEvents;
    ostringstream s;
    
    bufferSize_ = 0 ;
    NumEvents = 0 ;
    
    while (1 > NumEvents)
    {
        bufferSize_=0;
        NumEvents=0;
        ret = CAEN_DGTZ_ReadData (digitizerHandle_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT, buffer_, &bufferSize_) ;
        
        if (ret) {
            
            s.str(""); s << "[CAEN_V1742]::[ERROR]::READOUT ERROR!!!";
            Log(s.str(),1);
            
            ErrCode = ERR_READOUT ;
            return false;
        }
        
        if (bufferSize_ != 0) {
            ret = CAEN_DGTZ_GetNumEvents (digitizerHandle_, buffer_, bufferSize_, &NumEvents) ;
            if (ret) {
                s.str(""); s << "[CAEN_V1742]::[ERROR]::READOUT ERROR!!!";
                Log(s.str(),1);
                
                ErrCode = ERR_READOUT ;
                return false;
            }
            if (NumEvents == 0)
            {
                s.str(""); s << "[CAEN_V1742]::[WARNING]::NO EVENTS BUT BUFFERSIZE !=0";
                Log(s.str(),1);
            }
        }
        usleep(50);
    }
    
    //For the moment empty the buffers one by one
    if (NumEvents != 1)
    {
        s.str(""); s << "[CAEN_V1742]::[ERROR]::MISMATCHED EVENTS!!!" << NumEvents;
        Log(s.str(),1);

        ErrCode = ERR_MISMATCH_EVENTS ;
        // return ErrCode ;
    }
  
    return true;
}


int CAEN_V1742Standalone::SetBusyOn()
{
  return 0;
}

int CAEN_V1742Standalone::SetBusyOff()
{
  return 0;
}

int CAEN_V1742Standalone::TriggerAck()
{
  return 0;
}

int CAEN_V1742Standalone::SetTriggerStatus(TRG_t triggerType, TRG_STATUS_t triggerStatus)
{
    int status=0;
    if (triggerStatus == TRIG_ON)
        status |= CAEN_DGTZ_SWStartAcquisition(digitizerHandle_);
    else if (triggerStatus == TRIG_OFF)
        status |= CAEN_DGTZ_SWStopAcquisition(digitizerHandle_);
    return status;
}
