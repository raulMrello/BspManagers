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
RelayManager::RelayManager(RelayEntity** relays, PinName zc, Zerocross::LogicLevel zc_level){
            
    // Borro valor de sincronización 
    _delay_us = 0;
    
    // Creo objetos
    _relay_list = new List<RelayHandler>();
    uint8_t rlcount = 0;
    while(relays[rlcount]){
        RelayHandler* rh = (RelayHandler*)Heap::memAlloc(sizeof(RelayHandler));
        rh->relay = new Relay(relays[rlcount]->out_high, relays[rlcount]->out_low, relays[rlcount]->level);
        rh->info.id = relays[rlcount]->id;
        rh->info.action = Relay::NoActions;        
        _relay_list->addItem(rh);
        rlcount++;
    }
    _relay_list->setLimit(rlcount);
    _zc = new Zerocross(zc);    
    
    // Carga callbacks estáticas de publicación/suscripción
    _subscriptionCb = callback(this, &RelayManager::subscriptionCb);
    _publicationCb = callback(this, &RelayManager::publicationCb);   

    // Inicializa parámetros del hilo de ejecución propio
    _th.start(callback(this, &RelayManager::task));    
}


//------------------------------------------------------------------------------------
void RelayManager::task(){
    
    // se suscribe a mensajes en el topic "relay/value/cmd"
    MQ::MQClient::subscribe("relay/value/cmd", &_subscriptionCb);
    
    // Solicita apgado de los relés por defecto
    RelayHandler* rh = _relay_list->getFirstItem();
    while(rh){
        rh->info.action = Relay::RelayTurnOff;
        rh = _relay_list->getNextItem();
    }
    
    // Asigna manejadores de eventos del zerocross
    _zc->enableEvents(Zerocross::EdgeActiveAreBoth, callback(this, &RelayManager::isrZerocrossCb));
    
    // Arranca espera
    _timeout = osWaitForever;
    for(;;){
        osEvent evt = _th.signal_wait(0, _timeout);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            
            if((sig & RelayActionPendingFlag)!=0){
                // activa eventos del zerocross para ejecutar las acciones pendientes de forma sincronizada
                _zc->enableEvents(Zerocross::EdgeActiveAreBoth, callback(this, &RelayManager::isrZerocrossCb));
            }
            
            if((sig & RelayChangedFlag)!=0){
                // publica mensaje por cada acción completada
                _mut.lock();
                rh = _relay_list->getFirstItem();
                while(rh){
                    if(rh->info.action == Relay::ActionCompleted){
                        rh->info.stat = rh->relay->getState();
                        rh->info.action = Relay::NoActions;                        
                        MQ::MQClient::publish("relay/value/stat", &rh->info, sizeof(Relay::RelayInfo*), &_publicationCb);
                    }
                    rh = _relay_list->getNextItem();
                }           
                _mut.unlock();
            } 
        }
    }
}


//------------------------------------------------------------------------------------
void RelayManager::isrZerocrossCb(Zerocross::LogicLevel level){
    bool raise_event = false;
    // realizo espera forzada de sincronización
    wait_us(_delay_us);
    
    // busco acciones pendientes y las ejecuto en contexto ISR    
    RelayHandler* rh = _relay_list->getFirstItem();
    while(rh){
        if(rh->info.action == Relay::RelayTurnOff){
            rh->relay->turnOff();
            rh->info.action = Relay::ActionCompleted;
            raise_event = true;
        }
        else if(rh->info.action == Relay::RelayTurnOn){
            rh->relay->turnOn();
            rh->info.action = Relay::ActionCompleted;
            raise_event = true;
        }
        rh = _relay_list->getNextItem();
    }
    if(raise_event){
        _th.signal_set(RelayChangedFlag);
    }
    else{
        _zc->disableEvents(Zerocross::EdgeActiveAreBoth);
    }    
}        


//------------------------------------------------------------------------------------
void RelayManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
    
    // topic "relay/value/cmd"
    // los mensajes en el topic "relay/value/cmd" son un puntero a Relay::RelayInfo con el tamaño de sizeof(Relay::RelayInfo)
    if(strcmp(topic, "relay/value/cmd")==0){
        bool raise_event = false;
        Relay::RelayInfo* info = (Relay::RelayInfo*)msg;
        if(msg_len == sizeof(Relay::RelayInfo)){
            // una vez chequeado que todo es correcto, busca el relé y notifica la acción
            _mut.lock();
            RelayHandler* rh = _relay_list->getFirstItem();
            while(rh){
                if(rh->info.id == info->id){
                    rh->info.action = info->action;   
                    raise_event = true;
                }
                rh = _relay_list->getNextItem();
            }     
            _mut.unlock();
        }
        if(raise_event){
            _th.signal_set(RelayActionPendingFlag);
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
