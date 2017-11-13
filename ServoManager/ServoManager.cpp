/*
 * ServoManager.cpp
 *
 *  Created on: Nov 2017
 *      Author: raulMrello
 */

#include "ServoManager.h"


//------------------------------------------------------------------------------------
//--- PRIVATE TYPES ------------------------------------------------------------------
//------------------------------------------------------------------------------------

#define DEBUG_TRACE(format, ...)    if(_debug){_debug->printf(format, ##__VA_ARGS__);}

 
    
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
ServoManager::ServoManager(PinName sda, PinName scl, uint8_t num_servos) : PCA9685_ServoDrv(sda, scl, num_servos){
            
    _debug = 0;
    _sub_topic = 0;    
    _rmove.duty = NULL;
    _num_servos = num_servos;
    for(uint8_t i=0; i<PCA9685_ServoDrv::ServoCount; i++){
        _move_step[i] = NULL;
    }
                
    // Carga callbacks estáticas de publicación/suscripción    
    _subscrCb = callback(this, &ServoManager::subscriptionCb);   
    
    // Inicializa parámetros del hilo de ejecución propio
    _th.start(callback(this, &ServoManager::task));    
}


//------------------------------------------------------------------------------------
void ServoManager::startMovement(uint16_t* duty, uint8_t steps, uint32_t step_tick_us, uint8_t servo_zero, uint8_t step_dif){
    if(servo_zero >= _num_servos){
        return;
    }
    
    if(_rmove.duty){
        _tick_move.detach();
        Heap::memFree(_rmove.duty); 
        _rmove.duty = NULL;
    }
    _rmove.duty = (uint16_t*)Heap::memAlloc(steps * sizeof(uint16_t));
    if(_rmove.duty){
        _rmove.steps = steps;
        _rmove.step_tick_us = step_tick_us;
        for(uint8_t i=0;i<steps;i++){
            _rmove.duty[i] = duty[i];
        }
        for(uint8_t i=servo_zero;i<_num_servos;i++){
            _move_step[i] = step_dif * (i - servo_zero);
        }
        for(int8_t i=servo_zero-1;i>=0;i--){
            _move_step[i] = step_dif * (servo_zero-i);
        }        
        _tick_move.attach_us(callback(this, &ServoManager::onTickCb), _rmove.step_tick_us);
    }    
}


//------------------------------------------------------------------------------------
void ServoManager::stopMovement(){
    _tick_move.detach();
    
    if(_rmove.duty){
        Heap::memFree(_rmove.duty); 
        _rmove.duty = NULL;
    }
}

//------------------------------------------------------------------------------------
void ServoManager::setSubscriptionBase(const char* sub_topic) {
    if(_sub_topic){
        DEBUG_TRACE("\r\nServoManager: ERROR_SUB ya hecha!\r\n");
        return;
    }
    
    _sub_topic = (char*)sub_topic; 
 
    // Se suscribe a $sub_topic/cmd/#
    char* suscr = (char*)Heap::memAlloc(strlen(sub_topic) + strlen("/cmd/#")+1);
    if(suscr){
        sprintf(suscr, "%s/cmd/#", _sub_topic);
        MQ::MQClient::subscribe(suscr, &_subscrCb);
        DEBUG_TRACE("\r\nServoManager: Suscrito a %s/cmd/#\r\n", sub_topic);
    }
     
}   

//------------------------------------------------------------------------------------
//- PROTECTED CLASS IMPL. ------------------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
void ServoManager::task(){
    
    do{
        Thread::yield();
    }while(PCA9685_ServoDrv::getState() != PCA9685_ServoDrv::Ready);
       
    // Arranca espera
    _timeout = osWaitForever;
    for(;;){
        osEvent evt = _th.signal_wait(0, _timeout);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            
            if((sig & TickMoveFlag)!=0){
                // prepara el siguiente movimiento de cada servo
                for(uint8_t i = 0; i<_num_servos; i++){
                    setServoDuty(i, _rmove.duty[_move_step[i]]);
                    _move_step[i] = (_move_step[i] < (_rmove.steps - 1))? (_move_step[i] + 1) : 0;
                }
                // actualiza los servos
                PCA9685_ServoDrv::updateAll();
            } 
        }
    }
}
    

//------------------------------------------------------------------------------------
void ServoManager::onTickCb(){
    _th.signal_set(TickMoveFlag);   
}        


//------------------------------------------------------------------------------------
void ServoManager::subscriptionCb(const char* name, void* msg, uint16_t msg_len){
    // si es un comando para detener un movimiento repetitivo tipo respiración...
    if(strstr(name, "/cmd/move/stop") != 0){
        DEBUG_TRACE("\r\nServoManager: Movimiento terminado!\r\n");
        stopMovement();
        return;
    }
    
    // si es un comando para iniciar un movimiento repetitivo tipo respiración...
    if(strstr(name, "/cmd/move/start") != 0){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", name, msg);
        // obtengo los parámetros del mensaje Tstep y Tmax
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint32_t tstep = atoi(arg);
            arg = strtok(NULL, ",");
            uint32_t num_steps = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t srvorig = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t stepdif = atoi(arg);
            Heap::memFree(data);
            
            // obtiene el número de pasos y se añadirá un paso extra en cada extremo del movimiento
            uint16_t* duties = (uint16_t*)Heap::memAlloc(num_steps * sizeof(uint16_t));            
            
            if(duties){
                float deg = 360.0f/num_steps;
                float accdeg = 0;
                for(int i=0; i<num_steps;i++){
                    uint8_t angle = (uint8_t)(((PCA9685_ServoDrv::MaxAngleValue - PCA9685_ServoDrv::MinAngleValue)*((1.0f + sinf(270.0f + ((accdeg * 3.14159265f) / 360)))/2)) + PCA9685_ServoDrv::MinAngleValue);
                    accdeg += deg;
                    duties[i] = PCA9685_ServoDrv::getDutyFromAngle(srvorig, angle);                    
                }
                startMovement(duties, num_steps, tstep, srvorig, stepdif);
                Heap::memFree(duties);
            }            
        }
        return;
    }

    // si es un comando para mover un único servo
    if(strstr(name, "/cmd/servo") != 0){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", name, msg);
        // obtengo los parámetros del mensaje ServoID,Deg
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint8_t servo = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t deg = atoi(arg);
            Heap::memFree(data);
            
            // mueve el servo
            PCA9685_ServoDrv::setServoAngle(servo, deg, true);                       
        }
        return;
    }    
}
