# BspManagers

BspManagers es una librer�a orientada a MBED, que proporciona una serie de manejadores de alto nivel, para
uso avanzado de drivers. Generalmente estos m�dulos hacen uso de servicios del sistema operativo (tareas,
se�ales, etc...), as� como de otras librer�as como MQLib (publicaci�n-suscripci�n) proporcionando una gesti�n
de los eventos de los drivers en un contexto de tarea (fuera de las ISRs)


  
## Changelog

----------------------------------------------------------------------------------------------
##### 02.11.2017 ->commit:"Incluyo managers por defecto"
- [x] Incluyo los drivers que ya tengo hechos, que son:
	* TouchManager: Gestor que implementa la parte de alto nivel del driver MPR121_CapTouch I2C
	* CryptoManager: Gestor que implementa funcionalidades de acceso a un chip criptogr�fico I2C
	* ConsoleManager: Gestor que implementa un terminal remoto de comandos mediante un canal serie UART
	* FSManager: Gestor que implementa un manejador del sistema de ficheros
	* RelayManager: Gestor que maneja la activaci�n de varios rel�s, sincronizados con paso por cero 
	

