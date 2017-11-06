/*
 * RelayManager.cpp
 *
 *  Created on: 20/04/2015
 *      Author: raulMrello
 */

#include "RelayManager.h"


//------------------------------------------------------------------------------------
//--- EXTERN TYPES ------------------------------------------------------------------
//------------------------------------------------------------------------------------


 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
RelayManager::RelayManager(PinName zc, Zerocross::LogicLevel zc_level){
            
    // Borro valor de sincronización 
    _delay_us = 0;
    _rhnext = NULL;
        
    // Creo objetos
    _relay_list = new List<RelayHandler>();
    _zc = new Zerocross(zc);    
    
    // Carga callbacks estáticas de publicación/suscripción
    _subscriptionCb = callback(this, &RelayManager::subscriptionCb);
    _publicationCb = callback(this, &RelayManager::publicationCb);   
    _timeoutCb = callback(this, &RelayManager::isrMaxCurrTimeout);

    // Inicializa parámetros del hilo de ejecución propio
    _th.start(callback(this, &RelayManager::task));    
}


//------------------------------------------------------------------------------------
void RelayManager::task(){
    
    // se suscribe a mensajes en el topic "relay/value/cmd"
    MQ::MQClient::subscribe("relay/value/cmd", &_subscriptionCb);
        
    // Por defecto, desactiva eventos del zerocross
    _zc->disableEvents(Zerocross::EdgeActiveAreBoth);    
    
    // Arranca espera
    _timeout = osWaitForever;
    for(;;){
        osEvent evt = _th.signal_wait(0, _timeout);
        
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            
            if((sig & RelayActionPendingFlag)!=0){
                // activa eventos del zerocross para ejecutar las acciones pendientes de forma sincronizada
                // busca en toda la lista
                _rhnext = NULL;
                _zc->enableEvents(Zerocross::EdgeActiveAreBoth, callback(this, &RelayManager::isrZerocrossCb));
            }
            
            if((sig & RelayChangedFlag)!=0){
                // publica mensaje por cada acción completada
                _mut.lock();
                RelayHandler* rh = _relay_list->getFirstItem();
                while(rh){
                    if(rh->action == ActionCompleted){
                        RelayMsg msg(rh->relay->getId(), rh->relay->getState());
                        MQ::MQClient::publish("relay/value/stat", &msg, sizeof(RelayMsg), &_publicationCb);
                    }
                    rh = _relay_list->getNextItem();
                }                           
                _mut.unlock();
            } 
        }
    }
}


//------------------------------------------------------------------------------------
Relay* RelayManager::addRelay(RelayDefinition* relaydef, uint32_t high2lowtime){
    Relay* rl = new Relay(relaydef->id, relaydef->out_high, relaydef->out_low, relaydef->level, (1000 * high2lowtime));
    if(rl){
        RelayHandler* rhnd = (RelayHandler*)Heap::memAlloc(sizeof(RelayHandler));
        if(rhnd){
            rhnd->relay = rl;
            rhnd->action = NoActions;
            _mut.lock();
            if(_relay_list->addItem(rhnd) == List<RelayHandler>::SUCCESS){
                _mut.unlock();
                return rl;
            }
            _mut.unlock();
            Heap::memFree(rhnd);
            return NULL;
        }        
        delete(rl);
        return NULL;
    }
    return NULL;
}


//------------------------------------------------------------------------------------
void RelayManager::isrZerocrossCb(Zerocross::LogicLevel level){

    if(_rhnext && _rhnext->action != ActionCompleted){
        // realizo espera forzada de sincronización
        wait_us(_delay_us);
        if(_rhnext->action == RelayTurnOnHigh){
            _rhnext->relay->turnOnHigh(&_timeoutCb);
        }
        else if(_rhnext->action == RelayTurnOnLow){
            _rhnext->relay->turnOnLow();
        }
        else{
            _rhnext->relay->turnOff();
        }
        _rhnext->action = ActionCompleted;
    }
    
    // busco acciones pendientes y las ejecuto en contexto ISR, una a una  
    RelayHandler* rh;
    if(!_rhnext){
        rh = _relay_list->getFirstItem();
    }
    else{
        _rhnext = NULL;
        rh = _relay_list->getNextItem();
    }
    
    while(rh){
        if(rh->action != ActionCompleted && rh->action != NoActions){
            _rhnext = rh;
            return;
        }
        rh = _relay_list->getNextItem();
    }
    // desactivo próximos eventos
    _zc->disableEvents(Zerocross::EdgeActiveAreBoth);
    // notifico ejecución
    _th.signal_set(RelayChangedFlag);

}        


//------------------------------------------------------------------------------------
void RelayManager::isrMaxCurrTimeout(uint32_t id){
    RelayHandler* rh;
    rh = _relay_list->getFirstItem();
    while(rh){
        if(rh->relay->getId() == id){
            rh->action = RelayTurnOnLow;
            _th.signal_set(RelayActionPendingFlag);
            return;
        }
        rh = _relay_list->getNextItem();
    } 
}

//------------------------------------------------------------------------------------
void RelayManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
    
    // topic "relay/value/cmd"
    // los mensajes en el topic "relay/value/cmd" son un puntero a Relay::RelayInfo con el tamaño de sizeof(Relay::RelayInfo)
    if(strcmp(topic, "relay/value/cmd")==0){
        RelayMsg* rmsg = (RelayMsg*)msg;
        if(msg_len == sizeof(RelayMsg)){
            // una vez chequeado que todo es correcto, busca el relé y notifica la acción
            _mut.lock();
            RelayHandler* rh = _relay_list->getFirstItem();
            while(rh){
                if(rh->relay->getId() == rmsg->id){
                    rh->action = (rmsg->stat == Relay::RelayIsOff)? RelayTurnOff : RelayTurnOnHigh;   
                    _mut.unlock();
                    _th.signal_set(RelayActionPendingFlag);
                    return;
                }
                rh = _relay_list->getNextItem();
            }     
            _mut.unlock();
        }
    }
        
    // topic "relay/sync/cmd"
    // los mensajes en el topic "relay/sync/cmd" son un puntero a uint32_t con el tamaño de sizeof(uint32_t)
    if(strcmp(topic, "relay/sync/cmd")==0){
        uint32_t* delay = (uint32_t*)msg;
        if(msg_len == sizeof(uint32_t)){
            // una vez chequeado que todo es correcto, guarda el valor recibido y notifica solicitud
            _delay_us_req = *delay;
            _th.signal_set(SyncUpdateFlag);
        }
    }
}


//------------------------------------------------------------------------------------
void RelayManager::publicationCb(const char* topic, int32_t result){
}
