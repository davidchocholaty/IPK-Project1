# HTTP server poskytující informace o systému
Implementace serveru komunikujícího prostřednictvím protokolu HTTP poskytujícího následující informace o systému.

## Informace o systému
- Doménové jméno
- Informace o CPU
- Aktuální zátěž

## Autor
David Chocholatý (xchoch09@stud.fit.vutbr.cz)

## Návod použití

### Předpoklady
* gcc
* zip (Nutné pouze při vytváření archivu)

### Vytvoření projektu
Projekt lze vytvoři pomocí Makefile následujícím příkazem
```console
make
```

### Spuštění serveru
Server je spuštěn s argumentem, který určuje port, na kterém server bude naslouchat. Následující příklad obsahuje spuštění serveru s portem 12345.
```console
./hinfosvc 12345
```

### Ukončení serveru
Server lze ukončit pomocí kombinace kláves Ctrl+C

### Získání systémových informací
Systémové informace lze získat prostřednictvím webového rozhraní, příkazu GET, pomocí programu *curl* nebo s využitím programu *wget*. Následující příklady obsahují výčet všech variant použití.
	
#### Získání doménového jména

Webové rozhraní
```console
http://localhost:12345/hostname
```
Příkaz GET
```console
GET http://localhost:12345/hostname
```
Program *curl*
```console
curl http://localhost:12345/hostname
```
Program *wget*
```console
wget http://localhost:12345/hostname
```
#### Získání informací o CPU
	
Webové rozhraní
```console
http://localhost:12345/cpu-name
```
Příkaz GET
```console
GET http://localhost:12345/cpu-name
```
Program *curl*
```console
curl http://localhost:12345/cpu-name
```
Program *wget*
```console
wget http://localhost:12345/cpu-name
```
	
#### Aktuální zátěž
	
Webové rozhraní
```console
http://localhost:12345/load
```
Příkaz GET
```console
GET http://localhost:12345/load
```
Program *curl*
```console
curl http://localhost:12345/load
```
Program *wget*
```console
wget http://localhost:12345/load
```
## Ukázkový příklad - program curl

#### Vytvoření projektu
```console
make
```

#### Spuštění serveru
```console
./hinfosvc 12345 &
```

#### Získání doménového jména
```console
curl http://localhost:12345/hostname
```

merlin.fit.vutbr.cz

#### Získání informací o CPU
```console
curl http://localhost:12345/cpu-name
```

Intel(R) Xeon(R) Silver 4214R CPU @ 2.40GHz

#### Aktuální zátěž
```console
curl http://localhost:12345/load
```
4%
