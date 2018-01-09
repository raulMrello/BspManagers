#include "mbed.h"
#include "MQLib.h"
#include "Logger.h"
#include "TouchManager.h"


// **************************************************************************
// *********** DEFINICIONES *************************************************
// **************************************************************************


/** Macro de impresión de trazas de depuración */
#define DEBUG_TRACE(format, ...)    if(logger){logger->printf(format, ##__VA_ARGS__);}


// **************************************************************************
// *********** OBJETOS  *****************************************************
// **************************************************************************


/** Canal de depuración */
static Logger* logger;
/** Driver control detector */
static TouchManager* touchman;


// **************************************************************************
// *********** TEST  ********************************************************
// **************************************************************************



//------------------------------------------------------------------------------------
void touchEvtSubscription(const char* name, void* msg, uint16_t msg_len){
    DEBUG_TRACE("TOPIC:%s MESSAGE:%s\r\n", name, (char*)msg);
}


//------------------------------------------------------------------------------------
void test_TouchManager(){
            
    // --------------------------------------
    // Inicia el canal de comunicación remota
    //  - Pines USBTX, USBRX a 115200bps y 256 bytes para buffers
    //  - Configurado por defecto en modo texto
    logger = new Logger(USBTX, USBRX, 16, 115200);
    DEBUG_TRACE("\r\nIniciando test_TouchManager...\r\n");


    // --------------------------------------
    // Creo driver de control para el medidor de distancia
    DEBUG_TRACE("\r\nCreando Driver de proximidad...");    
    touchman = new TouchManager(PB_7, PB_6, PB_1, 0x1ff);
    touchman->setDebugChannel(logger);
    while(!touchman->ready()){
        Thread::yield();
    }
    DEBUG_TRACE("OK!");    
        
    // establezco topic base 'touch'
    touchman->setPublicationBase("touch");
    
    DEBUG_TRACE("\r\nSuscripción a touch ...");
    MQ::MQClient::subscribe("touch", new MQ::SubscribeCallback(&touchEvtSubscription));
    DEBUG_TRACE("OK!\r\n");
    
    // --------------------------------------
    // Arranca el test
    DEBUG_TRACE("\r\n...................INICIO DEL TEST.........................\r\n");    
    for(;;){
        Thread::yield();
    }
}

