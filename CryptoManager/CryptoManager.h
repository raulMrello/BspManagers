/*
 * CryptoManager.h
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 *
 *	CryptoManager es el m�dulo encargado de gestionar el acceso al chip de encriptaci�n y proporcionar una API com�n
 *  para obtener diferentes funcionalidades criptogr�ficas. Por defecto utiliza un puerto I2C para acceder al chip.
 *
 *  Este m�dulo se ejecuta como una librer�a pasiva, es decir, corriendo en el contexto del objeto llamante, y por lo
 *  tanto carece de thread asociado. Aunque utiliza un mutex para el acceso secuencial a los recursos.
 */
 
#ifndef __CryptoManager__H
#define __CryptoManager__H

#include "mbed.h"

/** Librer�as relativas a m�dulos software */
#include "Heap.h"


   
class CryptoManager{
  public:
              
    /** Constructor
     *  Crea el gestor criptogr�fico
     *  @param sda L�nea de datos I2C
     *  @param scl L�nea de reloj I2C
     *  @param freq Frecuencia I2C
     */
    CryptoManager(PinName sda, PinName scl, int freq);
  
  
    /** getUUID
     *  Obtiene el UUID del chip criptogr�fico en formato texto y/o en formato binario.
     *  @param uuid_text Puntero al buffer que recibe el UUID en modo texto (acabado en '\0').
     *         El buffer debe tener espacio suficiente para albergar dicho nombre.
     *  @param uuid_array Puntero al buffer que recibe el UUID en modo binario. El buffer debe
     *         tener espacio suficiente para albergar dicho array.
     */
    void getUUID(char* uuid_text, char* uuid_array=0);
    
  protected:

    I2C* _i2c;                  /// Puerto I2C utilizado para acceder al chip
    Mutex _mutex;               /// Mutex de acceso a los recursos criptogr�ficos
};
     
#endif /*__CryptoManager__H */

/**** END OF FILE ****/


