/*
 * ProximityManager.cpp
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 */

#include "ProximityManager.h"



//------------------------------------------------------------------------------------
//- STATIC ---------------------------------------------------------------------------
//------------------------------------------------------------------------------------

#define DEBUG_TRACE(format, ...)    if(_debug){_debug->printf(format, ##__VA_ARGS__);}




//------------------------------------------------------------------------------------
//- PUBLIC CLASS IMPL. ---------------------------------------------------------------
//- ProximityManager Class -------------------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
ProximityManager::ProximityManager(PinName trig, PinName echo, bool run_thread) : HCSR04(trig, echo){ 
                        
    _ready = false;
    _debug = 0;
    _msg = (char*)Heap::memAlloc(32);
    
    // Carga callbacks estáticas de publicación/suscripción    
    _pub_topic = 0;
    _sub_topic = 0;
    _pub_topic_unique = 0;
    _publCb = callback(this, &ProximityManager::publicationCb);   
    _subscrCb = callback(this, &ProximityManager::subscriptionCb);   
    
    // Inicia callback de recepción de eventos
    _distCb = callback(this, &ProximityManager::distEventCb);
    
    // Inicializa parámetros del hilo de ejecución propio si corresponde
    if(run_thread){
        _th.start(callback(this, &ProximityManager::task));    
        return;
    }
    _ready = true;
}


//------------------------------------------------------------------------------------
void ProximityManager::job(uint32_t signals){
    if((signals & DistEventFlag) != 0){                
        if(_pub_topic_unique){
            sprintf(_pub_topic_unique, "%s/sta/dist", _pub_topic);
            sprintf(_msg, "%d,%d", HCSR04::_last_event, HCSR04::_last_dist_cm);
            MQ::MQClient::publish(_pub_topic_unique, _msg, strlen(_msg)+1, &_publCb);
        }       
    }    
    
    if((signals & InvalidDistEventFlag) != 0){        
        if(_pub_topic_unique){
            sprintf(_pub_topic_unique, "%s/sta/dist/invalid", _pub_topic);
            sprintf(_msg, "0,%d", HCSR04::_filter.dist_cm[HCSR04::_filter.curr]);
            MQ::MQClient::publish(_pub_topic_unique, _msg, strlen(_msg)+1, &_publCb);
        }       
    }    
    
    if((signals & MeasureErrorEventFlag) != 0){        
        if(_pub_topic_unique){
            sprintf(_pub_topic_unique, "%s/sta/dist/ERROR", _pub_topic);
            sprintf(_msg, "0,%d", HCSR04::_last_error);
            MQ::MQClient::publish(_pub_topic_unique, _msg, strlen(_msg)+1, &_publCb);
        }       
    }        
}


//------------------------------------------------------------------------------------
void ProximityManager::setSubscriptionBase(const char* sub_topic) {
    if(_sub_topic){
        DEBUG_TRACE("\r\nProximityManager: ERROR_SUB ya hecha!\r\n");
        return;
    }
    
    _sub_topic = (char*)sub_topic; 
 
    // Se suscribe a $sub_topic/cmd/#
    char* suscr = (char*)Heap::memAlloc(strlen(sub_topic) + strlen("/cmd/#")+1);
    if(suscr){
        sprintf(suscr, "%s/cmd/#", _sub_topic);
        MQ::MQClient::subscribe(suscr, &_subscrCb);
        DEBUG_TRACE("\r\nProximityManager: Suscrito a %s/cmd/#\r\n", sub_topic);
    }     
}   


//------------------------------------------------------------------------------------
void ProximityManager::setPublicationBase(const char* pub_topic) {
    _pub_topic = (char*)pub_topic; 
    if(!_pub_topic_unique){
        _pub_topic_unique = (char*)Heap::memAlloc(MQ::MQClient::getMaxTopicLen());
    }
}   



//------------------------------------------------------------------------------------
//- PROTECTED CLASS IMPL. ------------------------------------------------------------
//- ProximityManager Class -------------------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void ProximityManager::task(){
    _ready = true;
    for(;;){
        osEvent evt = _th.signal_wait(0, osWaitForever);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            job(sig);
        }
    }
}


//------------------------------------------------------------------------------------
void ProximityManager::distEventCb(HCSR04::DistanceEvent ev, int16_t dist){
    switch(ev){
        case HCSR04::NoEvents:{
            _th.signal_set(InvalidDistEventFlag);
            break;
        }
        case HCSR04::MeasureError:{
            _th.signal_set(MeasureErrorEventFlag);
            break;
        }
        default:{
            _th.signal_set(DistEventFlag);
            break;
        }
    }
}


//------------------------------------------------------------------------------------
void ProximityManager::subscriptionCb(const char* name, void* msg, uint16_t msg_len){
    // si es un comando para ajustar eventos D(cm)I(cm),O(cm),F,R
    if(strstr(name, "/cmd/config") != 0){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", name, msg);
        // obtengo los parámetros del mensaje Tstep y Tmax
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint16_t max_dist = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t approach_dist = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t goaway_dist = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t filt_count = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t filt_range = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t endis_invalid_evts = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t endis_err_evts = atoi(arg);
            Heap::memFree(data);
            HCSR04::config(max_dist, approach_dist, goaway_dist, filt_count, filt_range, endis_invalid_evts, endis_err_evts);
        }
        return;
    }
    
    // si es un comando para iniciar un movimiento repetitivo cada T(ms)
    if(strstr(name, "/cmd/start") != 0){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", name, msg);
        // obtengo los parámetros del mensaje Tstep y Tmax
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint32_t lapse_ms = atoi(arg);
            arg = strtok(0, ",");
            uint32_t timeout_ms = atoi(arg);
            Heap::memFree(data);
            // inicia el movimiento
            HCSR04::start(_distCb, lapse_ms, timeout_ms);
        }
        return;
    }

    // si es un comando para detener el movimiento
    if(strstr(name, "/cmd/stop") != 0){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", name, msg);
        HCSR04::stop();
        return;
    }                      
}


//------------------------------------------------------------------------------------
void ProximityManager::publicationCb(const char* topic, int32_t result){
}



