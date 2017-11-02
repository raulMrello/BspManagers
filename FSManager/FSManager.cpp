/*
 * FSManager.cpp
 *
 *  Created on: Sep 2017
 *      Author: raulMrello
 */

#include "FSManager.h"


//------------------------------------------------------------------------------------
//--- EXTERN TYPES ------------------------------------------------------------------
//------------------------------------------------------------------------------------


 
//------------------------------------------------------------------------------------
//-- PUBLIC METHODS IMPLEMENTATION ---------------------------------------------------
//------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------
FSManager::FSManager(const char *name, PinName mosi, PinName miso, PinName sclk, PinName csel, int freq) : FATFileSystem(name) {

    // Creo el block device para la spi flash
    _name = name;
    _bd = new SPIFBlockDevice(mosi, miso, sclk, csel, freq);
    _bd->init();
    
    // Monto el sistema de ficheros en el blockdevice
    _error = FATFileSystem::mount(_bd);
    
    // Chequeo si hay información de formato, en caso de error formateo y creo archivo
    if(!ready()){
        _error = FATFileSystem::format(_bd);
        FILE* fd = fopen("/fs/format_info.txt", "w");
        if(fd){
            _error = fputs("Formateado correctamente\r\n", fd);
            _error = fclose(fd);
        }
    }
}


//------------------------------------------------------------------------------------
bool FSManager::ready() {
    FILE* fd = fopen("/fs/format_info.txt", "r");
    if(!fd){
        return false;
    }
    char buff[32] = {0};
    if(fgets(&buff[0], 32, fd) == 0){
        _error = fclose(fd);
        return false;
    }
    _error = fclose(fd);
    if(strcmp(buff, "Formateado correctamente\r\n") != 0){
        return false;
    }
    return true;
}


//------------------------------------------------------------------------------------
int FSManager::save(const char* data_id, void* data, uint32_t size){
    char * filename = (char*)Heap::memAlloc(strlen(data_id) + strlen("/fs/.dat") + 1);
    if(!filename){
        return -1;
    }
    sprintf(filename, "/fs/%s.dat", data_id);
    FILE* fd = fopen(filename, "w");
    int written = 0;
    if(fd){
        // reescribe desde el comienzo
        fseek(fd, 0, SEEK_SET);
        if(data && size){
            written = fwrite(data, 1, size, fd);
        }
        fclose(fd);
    }
    Heap::memFree(filename);
    return written;
}


//------------------------------------------------------------------------------------
int FSManager::restore(const char* data_id, void* data, uint32_t size){
    char * filename = (char*)Heap::memAlloc(strlen(data_id) + strlen("/fs/.dat") + 1);
    if(!filename){
        return -1;
    }
    sprintf(filename, "/fs/%s.dat", data_id);
    FILE* fd = fopen(filename, "r");
    int rd = 0;
    if(fd){
        if(data && size){
            rd = fread(data, 1, size, fd);
        }
        fclose(fd);
    }
    Heap::memFree(filename);
    return rd;
}


//------------------------------------------------------------------------------------
int32_t FSManager::openRecordSet(const char* data_id){
    char * filename = (char*)Heap::memAlloc(strlen(data_id) + strlen("/fs/.dat") + 1);
    if(!filename){
        return 0;
    }
    sprintf(filename, "/fs/%s.dat", data_id);
    FILE* fd = fopen(filename, "r+");
    Heap::memFree(filename);
    return (int32_t)fd;    
}


//------------------------------------------------------------------------------------
int32_t FSManager::closeRecordSet(int32_t recordset){
    return (int32_t)fclose((FILE*)recordset);
}


//------------------------------------------------------------------------------------
int32_t FSManager::writeRecordSet(int32_t recordset, void* data, uint32_t record_size, int32_t* pos){
    if(!recordset || !data || !record_size){
        return 0;
    }
    int wr = 0;
    int32_t vpos = (pos)? (*pos) : 0;
    // se sitúa en la posición deseada
    if(pos){
        fseek((FILE*)recordset, vpos, SEEK_SET);
    }
    // actualiza el registro
    wr = fwrite(data, 1, record_size, (FILE*)recordset);    
    vpos += wr;
    if(pos){
        *pos = vpos;
    }
    return wr;
}

//------------------------------------------------------------------------------------
int32_t FSManager::readRecordSet(int32_t recordset, void* data, uint32_t record_size, int32_t* pos){
    if(!recordset || !data || !record_size){
        return 0;
    }
    int rd = 0;
    int32_t vpos = (pos)? (*pos) : 0;
    // se sitúa en la posición deseada, o a continuación
    if(pos){
        fseek((FILE*)recordset, vpos, SEEK_SET);
    }
    // lee el registro
    rd = fread(data, 1, record_size, (FILE*)recordset);
    vpos += rd;
    if(pos){
        *pos = vpos;
    }
    return rd;
}


//------------------------------------------------------------------------------------
int32_t FSManager::getRecord(const char* data_id, void* data, uint32_t record_size, int32_t* pos){
    if(!data || !record_size){
        return 0;
    }
    char * filename = (char*)Heap::memAlloc(strlen(data_id) + strlen("/fs/.dat") + 1);
    if(!filename){
        return -1;
    }
    sprintf(filename, "/fs/%s.dat", data_id);
    FILE* fd = fopen(filename, "r");
    int rd = 0;
    int32_t vpos = (pos)? (*pos) : 0;
    if(fd){
        // se sitúa en la posición deseada
        fseek(fd, vpos, SEEK_SET);
        // lee el registro
        rd = fread(data, 1, record_size, fd);
        fclose(fd);
    }
    vpos += rd;
    if(pos){
        *pos = vpos;
    }
    return rd;
}


//------------------------------------------------------------------------------------
int32_t FSManager::setRecord(const char* data_id, void* data, uint32_t record_size, int32_t* pos){
    if(!data || !record_size){
        return 0;
    }
    char * filename = (char*)Heap::memAlloc(strlen(data_id) + strlen("/fs/.dat") + 1);
    if(!filename){
        return -1;
    }
    sprintf(filename, "/fs/%s.dat", data_id);
    FILE* fd = fopen(filename, "r+");
    int wr = 0;
    int32_t vpos = (pos)? (*pos) : 0;
    if(fd){
        // se sitúa en la posición deseada
        fseek(fd, vpos, SEEK_SET);
        // actualiza el registro
        wr = fwrite(data, 1, record_size, fd);
        fclose(fd);
    }
    vpos += wr;
    if(pos){
        *pos = vpos;
    }
    return wr;
}


