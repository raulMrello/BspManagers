/*
 * FSManager.h
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 *
 *	FSManager es el m�dulo encargado de gestionar el acceso al sistema de ficheros. Es una implementaci�n de la clase
 *  FATFileSystem, heredando por lo tanto sus miembros p�blicos.
 *  El soporte del sistema de ficheros corre sobre una memoria NOR-Flash SPI SST6VFX de Microchip y por lo tanto se
 *  implementa un SPIFBlockDevice.
 *
 *  Este m�dulo se ejecuta como una librer�a pasiva, es decir, corriendo en el contexto del objeto llamante, y por lo
 *  tanto carece de thread asociado
 */
 
#ifndef __FSManager__H
#define __FSManager__H

#include "mbed.h"

/** Librer�as relativas a m�dulos software */
#include "Heap.h"
#include "SPIFBlockDevice.h"
#include "FATFileSystem.h"


class FSManager : public FATFileSystem{
  public:
              
    /** Constructor
     *  Crea el gestor del sistema de ficheros FAT asociando un nombre y el los gpio del puerto spi
     *  utilizado
     *  @param name Nombre del sistema de ficheros
     *  @param mosi Salida de datos SPI
     *  @param miso Entrada de datos SPI
     *  @param sclk Reloj SPI en modo master
     *  @param csel Salida NSS en modo master gestionada por hardware
     *  @param freq Frecuencia SPI (40MHz o 20MHz dependiendo del puerto utilizado).
     */
    FSManager(const char *name, PinName mosi, PinName miso, PinName sclk, PinName csel, int freq);
  
  
    /** ready
     *  Chequea si el sistema de ficheros est� listo
     *  @return True (si tiene formato) o False (si tiene errores)
     */
    bool ready();
  
    /** getName
     *  Obtiene el nombre del sistema de ficheros
     *  @return _name Nombre asignado
     */
    const char* getName() { return _name; }
  
  
    /** getBlockDevice
     *  Obtiene una referencia al BlockDevice implementado
     *  @return _name Nombre asignado
     */
    BlockDevice* getBlockDevice() { return _bd; }
  
  
    /** save
     *  Graba datos en memoria no vol�til de acuerdo a un identificador dado
     *  @param data_id Identificador de los datos a grabar
     *  @param data  Puntero a los datos 
     *  @param size Tama�o de los datos en bytes
     *  @return Resultado de la operaci�n (error=-1, num_datos_escritos >= 0)
     */   
    int save(const char* data_id, void* data, uint32_t size);
  
  
    /** restore
     *  Recupera datos de memoria no vol�til de acuerdo a un identificador dado
     *  @param data_id Identificador de los datos a grabar
     *  @param data  Puntero que recibe los datos recuperados
     *  @param size Tama�o m�ximo de datos a recuperar
     *  @return Resultado de la operaci�n (error=-1, num_datos recuperados >= 0)
     */   
    int restore(const char* data_id, void* data, uint32_t size);
  
  
    /** openRecordSet
     *  Abre un manejador de registros a partir de un identificador
     *  @param data_id Identificador del recordset a abrir
     *  @return identificador del recordset o 0 en caso de error
     */   
    int32_t openRecordSet(const char* data_id);  
  
    /** closeRecordSet
     *  Cierra un manejador de registros a partir de un identificador
     *  @param recordset Manejador de registros a cerrar
     *  @return resultado de la operaci�n: 0 (ok), !=0 (error)
     */   
    int32_t closeRecordSet(int32_t recordset);  

    
    /** writeRecordSet
     *  Inserta un registro de un tama�o desde una posici�n dada, en un recordset abierto previamente
     *  @param recordset Identificador del manejador de registros
     *  @param data  Puntero que recibe los datos recuperados
     *  @param record_size Tama�o del registro a recuperar
     *  @param pos Puntero con la posici�n de la que leer y que recibe la nueva posici�n actualizada
     *  @return N�mero de bytes escritos. Debe coincidir con record_size
     */   
    int32_t writeRecordSet(int32_t recordset, void* data, uint32_t record_size, int32_t* pos);

    
    /** readRecordSet
     *  Obtiene un registro de un tama�o desde una posici�n dada, en un recordset abierto previamente
     *  @param recordset Identificador del manejador de registros
     *  @param data  Puntero que recibe los datos recuperados
     *  @param record_size Tama�o del registro a recuperar
     *  @param pos Puntero con la posici�n de la que leer y que recibe la nueva posici�n actualizada
     *  @return N�mero de bytes escritos. Debe coincidir con record_size
     */   
    int32_t readRecordSet(int32_t recordset, void* data, uint32_t record_size, int32_t* pos);


    /** getRecord
     *  Recupera un registro de un tama�o desde una posici�n dada. El recordset se abre y se cierra internamente
     *  @param data_id Identificador de los datos a recuperar
     *  @param data  Puntero que recibe los datos recuperados
     *  @param record_size Tama�o del registro a recuperar
     *  @param pos Puntero con la posici�n de la que leer y que recibe la nueva posici�n actualizada
     *  @return N�mero de bytes le�dos. Debe coincidir con record_size
     */   
    int32_t getRecord(const char* data_id, void* data, uint32_t record_size, int32_t* pos);
  
  
    /** setRecord
     *  Escribe un registro de un tama�o desde una posici�n dada
     *  @param data_id Identificador de los datos a grabar
     *  @param data  Puntero que recibe los datos recuperados
     *  @param record_size Tama�o del registro a recuperar
     *  @param pos Puntero con la posici�n de la que leer y que recibe la nueva posici�n actualizada
     *  @return N�mero de bytes escritos. Debe coincidir con record_size
     */   
    int32_t setRecord(const char* data_id, void* data, uint32_t record_size, int32_t* pos);
    
  protected:

    const char* _name;          /// Nombre del sistema de ficheros
    SPIFBlockDevice* _bd;       /// BlockDevice implementado
    int _error;                 /// �ltimo error registrado
};
     
#endif /*__FSManager__H */

/**** END OF FILE ****/


