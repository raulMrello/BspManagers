/*
 * CryptoManager.h
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 *
 *	CryptoManager es el módulo encargado de gestionar el acceso al chip de encriptación y proporcionar una API común
 *  para obtener diferentes funcionalidades criptográficas. Por defecto utiliza un puerto I2C para acceder al chip.
 *
 *  Este módulo se ejecuta como una librería pasiva, es decir, corriendo en el contexto del objeto llamante, y por lo
 *  tanto carece de thread asociado. Aunque utiliza un mutex para el acceso secuencial a los recursos.
 */
 
#ifndef __CryptoManager__H
#define __CryptoManager__H

#include "mbed.h"

/** Librerías relativas a módulos software */
#include "Heap.h"


   
class CryptoManager{
  public:
              
    /** Constructor
     *  Crea el gestor criptográfico
     *  @param sda Línea de datos I2C
     *  @param scl Línea de reloj I2C
     *  @param freq Frecuencia I2C
     */
    CryptoManager(PinName sda, PinName scl, int freq);
  
  
    /** getUUID
     *  Obtiene el UUID del chip criptográfico en formato texto y/o en formato binario.
     *  @param uuid_text Puntero al buffer que recibe el UUID en modo texto (acabado en '\0').
     *         El buffer debe tener espacio suficiente para albergar dicho nombre.
     *  @param uuid_array Puntero al buffer que recibe el UUID en modo binario. El buffer debe
     *         tener espacio suficiente para albergar dicho array.
     */
    void getUUID(char* uuid_text, char* uuid_array=0);
    
  protected:

    I2C* _i2c;                  /// Puerto I2C utilizado para acceder al chip
    Mutex _mutex;               /// Mutex de acceso a los recursos criptográficos
};
     
#endif /*__CryptoManager__H */

/**** END OF FILE ****/


