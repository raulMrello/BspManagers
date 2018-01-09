#include "mbed.h"
#include "MQLib.h"
#include "MQSerialBridge.h"
#include "ServoManager.h"
#include "NVFlash.h"

// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresión de trazas de depuración */
#define DEBUG_TRACE(format, ...)    if(logger){logger->printf(format, ##__VA_ARGS__);}


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************

/** Canal de comunicación remota */
static MQSerialBridge* qserial;
static Logger* logger;

/** Control de servos */
static ServoManager* servoman;
/** Número de servos máximo */
static const uint8_t SERVO_COUNT = 3;



// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************


//------------------------------------------------------------------------------------
void test_ServoManager(){
            
    // --------------------------------------
    // Inicia el canal de comunicación remota
    //  - Pines USBTX, USBRX a 115200bps y 256 bytes para buffers
    //  - Configurado por defecto en modo texto
    qserial = new MQSerialBridge(USBTX, USBRX, 115200, 256);
    logger = (Logger*)qserial;
    DEBUG_TRACE("\r\nIniciando test_ServoManager...\r\n");

    // --------------------------------------
    // Creo manager de control para los servos
    //  - Dirección I2C = 0h
    //  - Número de servos controlables = 3 (0 al 2)    
    DEBUG_TRACE("\r\nCreando ServoManager...");    
    servoman = new ServoManager(PB_7, PB_6, SERVO_COUNT);
    servoman->setDebugChannel(logger);
    
    // espero a que esté listo
    DEBUG_TRACE("\r\n¿Listo?... ");
    do{
        Thread::yield();
    }while(!servoman->ready());
    DEBUG_TRACE(" OK");

    // recupera parámetros de calibración NV
    uint32_t* caldata = (uint32_t*)Heap::memAlloc(NVFlash::getPageSize());
    NVFlash::init();
    NVFlash::readPage(0, caldata);
    if(servoman->setNVData(caldata) != 0){
        DEBUG_TRACE("\r\nERR_NVFLASH_READ, borrando...");
        NVFlash::erasePage(0);
        // establezco rangos de funcionamiento por defecto
        DEBUG_TRACE("\r\nAjustando rangos por defecto... ");
        for(uint8_t i=0;i<SERVO_COUNT;i++){
            if(servoman->setServoRanges(i, 0, 120, 180, 480) != PCA9685_ServoDrv::Success){
                DEBUG_TRACE("ERR_servo_%d\r\n...", i);
            }            
        }
        servoman->getNVData(caldata);
        NVFlash::writePage(0, caldata);
        DEBUG_TRACE("OK");
    }
    else{
        DEBUG_TRACE("\r\nNVFLASH_RESTORE... OK!");
    }
    Heap::memFree(caldata);
    
    // situo todos a 0º y doy la orden sincronizada
    DEBUG_TRACE("\r\nGirando servos a 0º... ");
    for(uint8_t i=0;i<SERVO_COUNT;i++){
        if(servoman->setServoAngle(i, 0) != PCA9685_ServoDrv::Success){
            DEBUG_TRACE("ERR_servo_%d\r\n...", i);
        }               
    }
    if(servoman->updateAll() != PCA9685_ServoDrv::Success){
        DEBUG_TRACE("ERR_update");
    }                   
    DEBUG_TRACE("OK");
    
    // establezco topic base 'breathe'
    servoman->setSubscriptionBase("breathe/cmd");
    
    // --------------------------------------
    // Arranca el test
    DEBUG_TRACE("\r\n...................INICIO DEL TEST.........................\r\n");    
    DEBUG_TRACE("\r\n- Mover servo a grados: breathe/cmd/servo S,G");    
    DEBUG_TRACE("\r\n- Mover servo a duty:   breathe/cmd/duty S,D");    
    DEBUG_TRACE("\r\n- Iniciar trayectoria:  breathe/cmd/move/start T,N,S,D,Ai,Af");    
    DEBUG_TRACE("\r\n- Detener trayectoria:  breathe/cmd/move/stop 0");    
    DEBUG_TRACE("\r\n- Obtener info servo:   breathe/cmd/info S");    
    DEBUG_TRACE("\r\n- Calibrar servo:       breathe/cmd/cal S,Ai,Af,Di,Df");    
    DEBUG_TRACE("\r\n- Leer servo del chip:  breathe/cmd/read S\r\n");    
    
}

