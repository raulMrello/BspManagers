/*
 * ConsoleManager.cpp
 *
 *  Created on: Oct 2017
 *      Author: raulMrello
 */


#include "ConsoleManager.h"

//- PRIVATE -----------------------------------------------------------------------

//- IMPL. -------------------------------------------------------------------------

ConsoleManager::ConsoleManager(SerialTerminal* sterm, uint16_t recv_buf_size, const char* token) {    
    _sterm = sterm;
    _bufsize = recv_buf_size;
    _token = (char*)token;
    
    if(_bufsize){
        _databuf = (char*)Heap::memAlloc(_bufsize);
        if(_databuf){
            // prepara buffer
            memset(_databuf, 0, _bufsize);
            
            // Carga callbacks estáticas de publicación/suscripción
            _subscriptionCb = callback(this, &ConsoleManager::subscriptionCb);
            _publicationCb = callback(this, &ConsoleManager::publicationCb);   

            // Inicializa parámetros del hilo de ejecución propio
            _th.start(callback(this, &ConsoleManager::task));
        }
    }
}

//---------------------------------------------------------------------------------
void ConsoleManager::onRxComplete(){
    _th.signal_set(ReceivedData);
}

//---------------------------------------------------------------------------------
void ConsoleManager::onRxTimeout(){
    _th.signal_set(TimeoutOnRecv);
}

//---------------------------------------------------------------------------------
void ConsoleManager::onRxOvf(){
    _th.signal_set(OverflowOnRecv);
}

//---------------------------------------------------------------------------------
void ConsoleManager::task(){    
    
    // se suscribe a los topics de redireccionamiento rpc
    MQ::MQClient::subscribe("$SYS/rpc/dir/#", &_subscriptionCb);
    
    // Inicializo el terminal de recepción
    _sterm->config(callback(this, &ConsoleManager::onRxComplete), callback(this, &ConsoleManager::onRxTimeout), callback(this, &ConsoleManager::onRxOvf), 500, 0);
    _sterm->startReceiver();
        
    for(;;){        
        osEvent evt = _th.signal_wait(0, _timeout);
        if(evt.status == osEventSignal){   
            uint32_t sig = evt.value.signals;
            if((sig & (TimeoutOnRecv | OverflowOnRecv))!=0){
                // descarto la trama recibida
                _sterm->recv(0, 0);
            }
            if((sig & ReceivedData)!=0){
                // se lee la trama recibida
                _sterm->recv(_databuf, _bufsize);
                // se extraen los tokens para obtener el comando
                char pubcmd[32];
                char* cmd = strtok(_databuf, (const char*)_token);
                strcpy(pubcmd, "$SYS/rpc/req/");
                strcat(pubcmd, cmd);
                char* args = (char*)(cmd + (strlen(cmd) + 1));
                MQ::MQClient::publish(pubcmd, args, strlen(args)+1, &_publicationCb);
            }
        }
    }
}


//------------------------------------------------------------------------------------
void ConsoleManager::subscriptionCb(const char* topic, void* msg, uint16_t msg_len){
    // chequea que el tamaño del mensaje coincide
    if(strlen((char*)msg) == msg_len){
        // forma la trama de envío
        char* msg = (char*)Heap::memAlloc(_bufsize);
        if(msg){
            // obtengo el topic a redirigir
            char* redir = (char*)(topic + strlen("$SYS/rpc/dir/"));
            sprintf(msg, "%s\n%s\n", redir, (char*)msg);
            _sterm->printf(msg);
            Heap::memFree(msg);
        }
    }
}


//------------------------------------------------------------------------------------
void ConsoleManager::publicationCb(const char* topic, int32_t result){
    // Hacer algo si es necesario...
}  




