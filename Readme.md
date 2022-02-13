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
	* Získání doménového jména
	1. Webové rozhraní
	```console
	http://localhost:12345/hostname
	```
	2. Program *curl*
	```console
	curl http://localhost:12345/hostname
	```
	3. Program *wget*
	```console
	wget http://localhost:12345/hostname
	```
	* Získání informací o CPU
	1. Webové rozhraní
	```console
	http://localhost:12345/cpu-name
	```
	2. Program *curl*
	```console
	curl http://localhost:12345/cpu-name
	```
	3. Program *wget*
	```console
	wget http://localhost:12345/cpu-name
	```
	* Aktuální zátěž
	1. Webové rozhraní
	```console
	http://localhost:12345/load
	```
	2. Program *curl*
	```console
	curl http://localhost:12345/load
	```
	3. Program *wget*
	```console
	wget http://localhost:12345/load
	```
