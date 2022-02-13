# HTTP server poskytující informace o systému
Implementace serveru komunikujícího prostřednictvím protokolu HTTP poskytujícího následující informace o systému.

## Informace o systému
- Doménové jméno
- Informace o CPU
- Aktuální zátěž

## Návod použití
* Spuštění serveru
	* Server je spuštěn s argumentem, který určuje port, na kterém server bude naslouchat. Následující příklad obsahuje spuštění serveru s portem 12345.
```console
./hinfosvc 12345
```
* Získání systémových informací - systémové informace lze získat prostřednictvím webového rozhraní, pomocí programu *curl* a nebo s využitím programu *wget*. Následující příklady obsahují výčet všech variant použití.
	
	1. Získání doménového jména
	
	Webové rozhraní
	```console
	http://localhost:12345/hostname
	```
	Program *curl*
	```console
	curl http://localhost:12345/hostname
	```
	Program *wget*
	```console
	wget http://localhost:12345/hostname
	```
	1. Získání informací o CPU
	
	Webové rozhraní
	```console
	http://localhost:12345/cpu-name
	```
	Program *curl*
	```console
	curl http://localhost:12345/cpu-name
	```
	Program *wget*
	```console
	wget http://localhost:12345/cpu-name
	```
	
	1. Aktuální zátěž
	
	Webové rozhraní
	```console
	http://localhost:12345/load
	```
	Program *curl*
	```console
	curl http://localhost:12345/load
	```
	Program *wget*
	```console
	wget http://localhost:12345/load
	```
