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

#define DEBUG_TRACE(format, ...)    if(_debug){_debug->printf(format, ##__VA_ARGS__);}

static void defaultCb(TouchManager::TouchMsg* msg){
}
 
    
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
TouchManager::TouchManager(PinName sda, PinName scl, PinName irq, uint8_t addr) : MPR121_CapTouch(sda, scl, irq, addr){
            
    _debug = 0;
    _pub_topic = 0;
    _curr_sns = 0;
    _evt_cb = callback(defaultCb);
                
    // Carga callbacks estáticas de publicación/suscripción
    _publicationCb = callback(this, &TouchManager::publicationCb);   

    // Inicializa parámetros del hilo de ejecución propio
    _th.start(callback(this, &TouchManager::task));    
}



//------------------------------------------------------------------------------------
//- PROTECTED CLASS IMPL. ------------------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void TouchManager::task(){
    
    DEBUG_TRACE("TouchManager: Start\r\n");
    
    // Arranca espera
    _timeout = osWaitForever;
    for(;;){
        osEvent evt = _th.signal_wait(0, _timeout);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            if((sig & IrqFlag)!=0){
                DEBUG_TRACE("TouchManager: IrqDetect!!\r\n");   
                // lee el valor de los sensores
                uint16_t sns = MPR121_CapTouch::touched();
                // evalúa sensor a sensor
                for(uint8_t i = 0; i< MPR121_CapTouch::SensorCount; i++){
                    if((sns & ((uint16_t)1 << i)) != (_curr_sns & ((uint16_t)1 << i))){
                        TouchMsg msg = {i, (((sns & ((uint16_t)1 << i)) != 0)? TouchedEvent : ReleasedEvent)};
                        // notifica evento en callback
                        _evt_cb.call(&msg);
                        // publica mensaje
                        if(_pub_topic){
                            MQ::MQClient::publish(_pub_topic, &msg, sizeof(TouchMsg), &_publicationCb);
                        }
                    }
                }
            } 
        }
    }
}
    

//------------------------------------------------------------------------------------
void TouchManager::onIrqCb(){
    _th.signal_set(IrqFlag);   
}        


//------------------------------------------------------------------------------------
void TouchManager::publicationCb(const char* topic, int32_t result){
}
