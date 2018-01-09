# BspManagers

BspManagers es una librer�a orientada a MBED, que proporciona una serie de manejadores de alto nivel, para
uso avanzado de drivers. Generalmente estos m�dulos hacen uso de servicios del sistema operativo (tareas,
se�ales, etc...), as� como de otras librer�as como MQLib (publicaci�n-suscripci�n) proporcionando una gesti�n
de los eventos de los drivers en un contexto de tarea (fuera de las ISRs)


  
## Changelog

----------------------------------------------------------------------------------------------
##### 09.01.2018 ->commit:"Actualiza managers de mbed-l432"
- [x] Actualizo Touch,Proximity y Servo.
	

----------------------------------------------------------------------------------------------
##### 16.11.2017 ->commit:"Actualizo TocuhManager"
- [x] Actualizo TocuhManager tras las pruebas funcionales terminadas.
	

----------------------------------------------------------------------------------------------
##### 16.11.2017 ->commit:"Incluyo ProximityManager"
- [x] A�ado ProximityManager tras las pruebas funcionales terminadas.
	

----------------------------------------------------------------------------------------------
##### 14.11.2017 ->commit:"Incluyo opciones de backup en ServoManager"
- [x] A�ado topics para funcinalidades extra, como backup
	

----------------------------------------------------------------------------------------------
##### 14.11.2017 ->commit:"A�ado funcionalidades ServoManager"
- [x] A�ado topics para funcinalidades extra, como calibraci�n, trayectorias, etc...
	

----------------------------------------------------------------------------------------------
##### 13.11.2017 ->commit:"A�ado ServoManager"
- [x] A�ado ServoManager.
	

----------------------------------------------------------------------------------------------
##### 07.11.2017 ->commit:"Quito ConsoleManager"
- [x] Quito ConsoleManager porque tiene la misma funcionalidad que /Middleware/MQSerialBridge y
	  adem�s no estaba probado.
	

----------------------------------------------------------------------------------------------
##### 06.11.2017 ->commit:"Actualizo RelayManager"
- [x] Actualizo RelayManager con funcionalidad simplificada. FALTA VERIFICAR FUNCIONAMIENTO. 
	

----------------------------------------------------------------------------------------------
##### 02.11.2017 ->commit:"Incluyo managers por defecto"
- [x] Incluyo los drivers que ya tengo hechos, que son:
	* TouchManager: Gestor que implementa la parte de alto nivel del driver MPR121_CapTouch I2C
	* CryptoManager: Gestor que implementa funcionalidades de acceso a un chip criptogr�fico I2C
	* ConsoleManager: Gestor que implementa un terminal remoto de comandos mediante un canal serie UART
	* FSManager: Gestor que implementa un manejador del sistema de ficheros
	* RelayManager: Gestor que maneja la activaci�n de varios rel�s, sincronizados con paso por cero 
	

