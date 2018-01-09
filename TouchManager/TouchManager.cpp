/*
 * TouchManager.cpp
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 */

#include "TouchManager.h"


//------------------------------------------------------------------------------------
//--- PRIVATE TYPES ------------------------------------------------------------------
//------------------------------------------------------------------------------------

#define DEBUG_TRACE(format, ...)    if(_debug){ _debug->printf(format, ##__VA_ARGS__);}

static void defaultCb(TouchManager::TouchMsg* msg){
}
 
    
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
TouchManager::TouchManager(PinName sda, PinName scl, PinName irq, uint16_t elec_mask, uint8_t addr, bool run_thread) : MPR121_CapTouch(sda, scl, irq, elec_mask, addr){
            
    _debug = 0;
    _ready = false;
    _pub_topic = 0;
    _curr_sns = 0;
    _evt_cb = callback(defaultCb);
                
    // Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &TouchManager::publicationCb);   

    // instala manejador isr en driver
    MPR121_CapTouch::attachIrqCb(callback(this, &TouchManager::onIrqCb));
    
    // Inicializa parámetros del hilo de ejecución propio si corresponde
    if(run_thread){
         _th.start(callback(this, &TouchManager::task));     
        return;
    }
    _ready = true;
}


//------------------------------------------------------------------------------------
void TouchManager::job(uint32_t signals){    
    if((signals & IrqFlag)!=0){
        // lee el valor de los sensores
        _sns = MPR121_CapTouch::touched();
        // descarta eventos de electrodos no habilitados
        if(_sns == _curr_sns){
            return;
        }
        
        // activa filtro antiglitch
        _tick_glitch.attach_us(callback(this, &TouchManager::isrTickCb), AntiGlitchTimeout);
    }
    
    if((signals & AntiGlitchFlag)!=0){  
        uint16_t sns = MPR121_CapTouch::touched();        
        // descarta glitches
        if(sns != _sns){
            return;
        }
        // evalúa sensor a sensor
        for(uint8_t i = 0; i< MPR121_CapTouch::SensorCount; i++){
            if((_sns & ((uint16_t)1 << i)) != (_curr_sns & ((uint16_t)1 << i))){
                TouchMsg msg = {i, (((_sns & ((uint16_t)1 << i)) != 0)? TouchedEvent : ReleasedEvent)};
                // notifica evento en callback
                _evt_cb.call(&msg);
                // publica mensaje
                if(_pub_topic){
                    sprintf(_msg, "%d,%d", msg.elec, msg.evt);
                    MQ::MQClient::publish(_pub_topic, _msg, strlen(_msg)+1 , &_publicationCb);
                }
            }
        }
        _curr_sns = _sns;
    }  
}


//------------------------------------------------------------------------------------
void TouchManager::setPublicationBase(const char* pub_topic) {
    _pub_topic = (char*)pub_topic; 
}   


//------------------------------------------------------------------------------------
//- PROTECTED CLASS IMPL. ------------------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void TouchManager::task(){
    while(MPR121_CapTouch::getState() != MPR121_CapTouch::Ready){
        Thread::yield();
    }
    _curr_sns = MPR121_CapTouch::touched();
    _ready = true;
    
    // Arranca espera
    _timeout = osWaitForever;
    for(;;){
        osEvent evt = _th.signal_wait(0, _timeout);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            job(sig);           
        }
    }
}
    

//------------------------------------------------------------------------------------
void TouchManager::onIrqCb(){
    _th.signal_set(IrqFlag);   
}   
    

//------------------------------------------------------------------------------------
void TouchManager::isrTickCb(){
    _tick_glitch.detach();
    _th.signal_set(AntiGlitchFlag);   
}


//------------------------------------------------------------------------------------
void TouchManager::publicationCb(const char* topic, int32_t result){
}
