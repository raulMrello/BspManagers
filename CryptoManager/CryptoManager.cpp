/*
 * CryptoManager.cpp
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 */

#include "CryptoManager.h"


//------------------------------------------------------------------------------------
//--- EXTERN TYPES ------------------------------------------------------------------
//------------------------------------------------------------------------------------


 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
CryptoManager::CryptoManager(PinName sda, PinName scl, int freq) {

    // Creo el puerto serie asociado
    if(sda != NC && scl != NC){
        _i2c = new I2C(sda, scl);
        _i2c->frequency(freq);       
    }
}


//------------------------------------------------------------------------------------
void CryptoManager::getUUID(char* uuid_text, char* uuid_array) {
    // accede al chip para obtener el UUID
    _mutex.lock();
    
    #warning DE MOMENTO GENERO UN UUID 72-bit ALEATORIO
    srand((int)uuid_text);
    // genera un caracter entre 0 y 9
    for(int i=0;i<9;i++){
        char car = rand() % 10;
        if(uuid_text){
            uuid_text[i] = 48 + car;            
        }
        if(uuid_array){
            uuid_array[i] = car;
        }
    }
    if(uuid_text){
        uuid_text[9] = 0;            
    }
    
    _mutex.unlock();
}

