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

#define DEBUG_TRACE(format, ...)    if(_debug){ _debug->printf(format, ##__VA_ARGS__);}

 
    
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
                    
    // Carga callbacks est�ticas de publicaci�n/suscripci�n    
    _subscrCb = callback(this, &ServoManager::subscriptionCb);   
    
    // Inicializa par�metros del hilo de ejecuci�n propio
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
 
    // Se suscribe a $sub_topic/#
    char* suscr = (char*)Heap::memAlloc(strlen(sub_topic) + strlen("/#")+1);
    if(suscr){
        sprintf(suscr, "%s/#", _sub_topic);
        MQ::MQClient::subscribe(suscr, &_subscrCb);
        DEBUG_TRACE("\r\nServoManager: Suscrito a %s/#\r\n", sub_topic);
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
void ServoManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
    // si es un comando para detener un movimiento repetitivo tipo respiraci�n...
    if(MQ::MQClient::isTopicToken(topic, "/move/stop")){
        DEBUG_TRACE("\r\nServoManager: Movimiento terminado!\r\n");
        stopMovement();
        return;
    }
    
    // si es un comando para iniciar un movimiento repetitivo tipo respiraci�n...
    if(MQ::MQClient::isTopicToken(topic, "/move/start")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje Tstep y Tmax
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
            arg = strtok(NULL, ",");
            uint8_t ang_min = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t ang_max = atoi(arg);
            Heap::memFree(data);
            
            // obtiene el n�mero de pasos
            uint16_t* duties = (uint16_t*)Heap::memAlloc(num_steps * sizeof(uint16_t));            
            if(duties){
                float rad_inc = (((360.0f/num_steps) * 3.14159265f) / 180);
                for(int i=0; i<num_steps;i++){
                    float value = sinf((i * rad_inc));
                    uint8_t angle = (uint8_t)((((ang_max - ang_min)/2) * value) + (ang_max - ang_min)/2);
                    duties[i] = PCA9685_ServoDrv::getDutyFromAngle(srvorig, angle);                    
                }
                startMovement(duties, num_steps, tstep, srvorig, stepdif);
                Heap::memFree(duties);
            }            
        }
        return;
    }

    // si es un comando para mover un �nico servo
    if(MQ::MQClient::isTopicToken(topic, "/servo")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje ServoID,Deg
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

    // si es un comando para mover un �nico servo
    if(MQ::MQClient::isTopicToken(topic, "/duty")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje ServoID,Duty
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint8_t servo = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t duty = atoi(arg);
            Heap::memFree(data);
            
            // mueve el servo
            PCA9685_ServoDrv::setServoDuty(servo, duty, true);                       
        }
        return;
    }  

    // si es un comando para mover un �nico servo
    if(MQ::MQClient::isTopicToken(topic, "/info")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje ServoID,Duty
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint8_t servo = atoi(arg);
            Heap::memFree(data);
            
            // mueve el servo
            uint8_t angle = PCA9685_ServoDrv::getServoAngle(servo);                       
            uint16_t duty = PCA9685_ServoDrv::getServoDuty(servo);
            int16_t min_ang, max_ang;
            uint16_t min_duty, max_duty;
            PCA9685_ServoDrv::getServoRanges(servo, &min_ang, &max_ang, &min_duty, &max_duty);             
            DEBUG_TRACE("\r\nServoManager: Servo %d: ang=%d (%d,%d), duty=%d (%d,%d)\r\n", servo, angle, min_ang, max_ang, duty, min_duty, max_duty); 
        }
        return;
    }            

    // si es un comando para leer el duty del servo del chip i2c
    if(MQ::MQClient::isTopicToken(topic, "/read")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje ServoID,Duty
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint8_t servo = atoi(arg);
            Heap::memFree(data);
            
            // mueve el servo
            uint16_t duty;    
            if(PCA9685_ServoDrv::readServoDuty(servo, &duty) == PCA9685_ServoDrv::Success){
                uint8_t angle = PCA9685_ServoDrv::getAngleFromDuty(servo, duty);                                   
                DEBUG_TRACE("\r\nServoManager: Servo %d: ang=%d, duty=%d\r\n", servo, angle, duty);                 
            }
            else{
                DEBUG_TRACE("\r\nServoManager: ERR_READ Servo %d\r\n", servo); 
            }
        }
        return;
    }              

    // si es un comando para calibrar el servo
    if(MQ::MQClient::isTopicToken(topic, "/cal")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los par�metros del mensaje ServoID,Duty
        char* data = (char*)Heap::memAlloc(msg_len);
        if(data){
            strcpy(data, (char*)msg);
            char* arg = strtok(data, ",");
            uint8_t servo = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t ang_min = atoi(arg);
            arg = strtok(NULL, ",");
            uint8_t ang_max = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t d_min = atoi(arg);
            arg = strtok(NULL, ",");
            uint16_t d_max = atoi(arg);
            Heap::memFree(data);
            
            // calibra el servo
            PCA9685_ServoDrv::setServoRanges(servo, ang_min, ang_max, d_min, d_max);
        }
        return;
    }                  

    // si es un comando para guardar la calibraci�n de los servos
    if(MQ::MQClient::isTopicToken(topic, "/save")){
        DEBUG_TRACE("\r\nServoManager: Topic:%s msg:%s\r\n", topic, msg);
        // obtengo los datos de calibraci�n y los actualizo
        uint32_t* caldata = (uint32_t*)Heap::memAlloc(NVFlash::getPageSize());
        if(caldata){
            NVFlash::readPage(0, caldata);
            PCA9685_ServoDrv::getNVData(caldata);
            NVFlash::erasePage(0);
            if(NVFlash::writePage(0, caldata) == NVFlash::Success){
                DEBUG_TRACE("\r\nGuardados datos de calibraci�n\r\n");               
            }
            else{
                DEBUG_TRACE("\r\nERROR guardando datos de calibraci�n\r\n");
            }
            Heap::memFree(caldata);
        }
        return;
    }                      
}
