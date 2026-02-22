//v 1.3 vamos a incluir el lanzamiento de emulador al subir el pulsador.

#include<SPI.h>
// DIRECCION DE MEMORIA DEL PAIS FFD9. VALOR 0 ES JAPONES, VALOR 2 ES PAL.
// FFD6, TIPO DE MEMORIA. VALOR 0 = LOW ROM ???
const int AddressLatchPin = 10;
byte solounavez=0;
byte solounavez_interruptor=0;
int fin=0;
unsigned int numero;
unsigned int numerorom;
unsigned int specialchip;
unsigned int endbank;
unsigned int addr = 0x0000; //;   DIRECCION INICIAL  
unsigned int endAddr = 0x0000; //; DIRECCION FINAL 
unsigned int estado=0;
unsigned int banksize;
unsigned int data;
unsigned int bank = 0;
char Byte_recibido;
char Byte_recibido2;
byte romsize;
unsigned int ramsize;
byte errorRom=0;
String cadena;
String numero_fix;
String letra;



const int snesReadPin = A1; //Cart pin 23 - aka /RD - Address bus read
const int snesWritePin = A2; //Cart pin 54 - aka /WR - Address bus write
const int snesCartPin = A3; //Cart pin 49 - aka /CS, /ROMSEL, /Cart - Goes low when reading ROM
const int snesResetPin = A4; //Cart pin 26 - SNES reset pin. Goes high when reading cart
const int pinbutton = A5; // PUSH BUTTON
const int interruptor = A6; //INTERRUPTOR PARA LANZAR EL CARTUCHO O SALIR DEL JUEGO.
const int pinswitchdown = A7; //INTERRUPTOR PARA LANZAR EL CARTUCHO O SALIR DEL JUEGO. // aunque sea un pin analogico A7 puedo leerlo como digital si lo uso como pin 19
int onoff;
int switchdown;
int pushbutton;
String cadenanumero;
typedef enum COMMANDS {
  CTRL,
  READSECTION,
  WRITESECTION
};



void setup() {
  Serial.begin(250000);
 // UBRR0 = 0; //max baud rate
  bitSet(UCSR0A, U2X0); // change UART divider from 16 to 8  for double transmission speed

  //begin spi
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2); //8MHz

  setDataBusDir(INPUT);

  //setup cart control pins
  pinMode(snesCartPin, OUTPUT);
  pinMode(snesReadPin, OUTPUT);
  pinMode(snesWritePin, OUTPUT);
  pinMode(snesResetPin, OUTPUT);
  pinMode(interruptor, INPUT);

}

void loop() {




//Serial.println(onoff);
//pushbutton = analogRead(pinbutton);
//switchdown = analogRead(pinswitchdown);

//recibimos un caracter desde el programa de pyhton para saber que tiene que hacer arduino
// a Envia la rom por puerto serie
// b Envia la información del cartucho
// c descargo la sram del cartucho
// d escribo sram en el cartucho
// e entro en modo player


if (Serial.available() > 0){  //<", ">"   
estado=0;
Byte_recibido = Serial.read();

} 

if(Byte_recibido == 'e'){ // entro en modo player, leo el interrupto para lanzar el emulador

while(true){// LEO INTERRUPTOR Y PULSADOR
onoff = analogRead(interruptor);
//onoff=1000; // esto es para desactivar el interruptor
//Serial.println(onoff);
if (onoff<500 && solounavez_interruptor==0){

      delay(100);
      digitalWrite(snesReadPin, LOW);
      digitalWrite(snesWritePin, LOW);
      digitalWrite(snesCartPin, LOW);
      digitalWrite(snesResetPin, HIGH);
     
      byte bank = 0;
      unsigned int addr = 0xFFC0; //65472;   DIRECCION INICIAL 
      unsigned int endAddr = 0xFFDF; //65503; DIRECCION FINAL UN BYTE MENOS PORQUE EL ULTIMO SE LO PASA.
      
      setDataBusDir(INPUT);
     
        while (true){
          if (solounavez==0 && ((addr>=65472 && addr <65476) || (addr>=65500) )){//cojo las 4 primeras leetras del nombre y los 4 ultimos bytes que son checksum
        
          writeAddrBus(bank, addr);
          delay(20);
          numero = (readDataBus());
          letra = String(numero);         
           
           if ((numero) < 16){ // add a leading zero when necesary
            numero_fix = String("0"+ String(numero,HEX));
           }else{
            numero_fix= String(numero,HEX);
           }     
        
        cadena.concat(letra); // the title is readed char by char, this line put togheter all the character
        cadenanumero.concat(numero_fix);
          }
          
          if (addr == endAddr ) {
            if(solounavez==0){
              Serial.println(cadenanumero);
               solounavez_interruptor=1;
               cadena="";
              cadenanumero="";
              }
              
            break;
          }
        Serial.flush();
        addr++;    
   }
     

}

  if (onoff>500 && solounavez_interruptor==1){
  Serial.println("OFF");
  solounavez_interruptor=0;
  delay(100);
  }
}
}

if(Byte_recibido == 'd'){ // grabo sram al cartucho . sram --> cart

  //ESTE IF ES PARA COMPROBAR SI ES LOROM O HIROM
  digitalWrite(snesReadPin, LOW);
  digitalWrite(snesWritePin, HIGH);
  digitalWrite(snesCartPin, LOW);
  digitalWrite(snesResetPin, HIGH);
              
  addr = 0xFFD5; //65472;  address for room tipe, hirom or lorom
  setDataBusDir(INPUT);
  writeAddrBus(bank, addr); // read byte for type rom 
  delayMicroseconds(5);
  numerorom = (readDataBus()); //49-HIROM   32-LOROM   48-LOROM-FASTROM

  addr = 0xFFD6; //  address for special chips
  bank=0;
  setDataBusDir(INPUT);
  writeAddrBus(bank, addr); // read byte for type room 
  delayMicroseconds(5);
  specialchip = (readDataBus());

  ///////// RAM SIZE //////////////

        ////*****///
       addr = 0xFFDA; // addres for ID DEVELOPER AND TO CHECK IF WE HAVE AN EXTENDED HEADER
      writeAddrBus(bank, addr); // read byte for ram size
      delayMicroseconds(5);
      data = (readDataBus());
      if (data==51){ //0X33 =51 //SHOW THAT THERE IS A EXPANDED HEADER
        addr = 0xFFBD; // addres for SRAM SIZE IN EXTENDED HEADER
        writeAddrBus(bank, addr); // read byte for ram size
        delayMicroseconds(5);
        data = (readDataBus());   //puede ser que tenga cabecera extendida pero la ram la tenga en el lugar normal, en ese caso ffbd será 0
        if (data !=0){ // expanded header and sram information in extender header
          ramsize= round(pow(2,data)*1024); //sram KBytes
        }else{ //expanded header but sram information in standar address
          addr = 0xFFD8; // addres for SRAM SIZE
          writeAddrBus(bank, addr); // read byte for ram size
          delayMicroseconds(5);
          data = (readDataBus());
          ramsize= round(pow(2,data)*1024); //sram KBytes

        }
      }else{ //not expanded but standar header
        addr = 0xFFD8; // addres for SRAM SIZE
        writeAddrBus(bank, addr); // read byte for ram size
        delayMicroseconds(5);
        data = (readDataBus());
        ramsize= round(pow(2,data)*1024); //sram KBytes

      }///****/////
    
    if(numerorom==49){ // HIROM calculo el numero de banks que ocupa la ram, 8kb por bank
      if (ramsize==3){
        banksize =0; //exactamente ocupa 1 bank.
      }else{
        banksize = round(pow (2,ramsize)/8);
      }
    }

    if(numerorom==32 || numerorom==48){ // LOROM calculo el numero de banks que ocupa la ram
      if (ramsize==5){  //ocupa justo el primer bank
        banksize=0;

      }else{      
        banksize = round(pow (2,ramsize)/32); 
      }
    }
  ///////// END RAM SIZE //////////////

   /////////// WRITE SRAM LOROM ////////////////////
  if (numerorom==32 || numerorom==48){
    digitalWrite(snesReadPin, LOW);
    digitalWrite(snesWritePin, LOW);   //ESTAS 4 LINEAS PARA GRABAR SRAM LOROM (L,L,L,H)
    digitalWrite(snesCartPin, LOW);
    digitalWrite(snesResetPin, HIGH);
    setDataBusDir(OUTPUT);

    if ((specialchip == 19) || (specialchip == 20) || (specialchip == 21) || (specialchip == 26)) { //superFX (Yoshi island for instance)
      bank =0x70 ; // 
    } else{
      bank =240 ; // 0xF0;
    }

    addr = 0; 
    endbank= bank + banksize;
    if(banksize == 0){ endAddr = ramsize;} //si ocupa menos de un bank, solo escribo la sram hasta donde llegue su tamaño.
    if(banksize !=0){endAddr = 0x8000;} // si la sram ocupa mas de un bank, se escriben los banks completos. Hay muy pocos juegos así.
    
      while (true) {
        if(Serial.available()>0){
          int numero_dato = Serial.read();
          writeAddrBus(bank, addr);
          delayMicroseconds(5); 
          writeDataBus(numero_dato);
          
          if (addr==endAddr){
            if (bank==endbank ){break;}
            addr=0;
            bank=bank+1;
          }

          addr++;
        }
        
      }
  } //fin del if LOROM




  /////////// WRITE SRAM HIROM ////////////////////
  if (numerorom==49){
    digitalWrite(snesReadPin, LOW);
    digitalWrite(snesWritePin, LOW);   //ESTAS 4 LINEAS PARA GRABAR SRAM LOROM (L,L,L,H) //esto funciona con chronotriger y smario kart
    digitalWrite(snesCartPin, HIGH);
    digitalWrite(snesResetPin, HIGH);
    setDataBusDir(OUTPUT);
    bank= 0xB0;
    addr = 0x6000; 
    endbank= bank + banksize; // esto no esta terminado habría que calcular cual es el enbank para cada juego.
    endAddr = addr + ramsize;
    if( banksize == 0){ endAddr = ramsize;} //si ocupa menos de un bank, solo escribo la sram hasta donde llegue su tamaño.
    if(banksize !=0){endAddr = 0x8000;} // si la sram ocupa mas de un bank, se escriben los banks completos. Hay muy pocos juegos así.
    
      while (true) {
        if(Serial.available()>0){
          int numero_dato = Serial.read();
          writeAddrBus(bank, addr);
          delayMicroseconds(5); 
          writeDataBus(numero_dato);
          
          if (addr==endAddr){
            if (bank==endbank ){break;}
            addr=0x6000;
            bank=bank+1;
          }

          addr++;
        }
        
      }
  } //fin del if HIROM

} 


if (Byte_recibido == 'b'){ // enviar la informacion del cartucho
  
  
    if (estado==0){ //estado es para que se ejecute solo una vez
      //ESTE IF ES PARA COMPROBAR SI ES LOROM O HIROM
      Serial.flush();
      digitalWrite(snesReadPin, LOW);
      digitalWrite(snesWritePin, HIGH);
      digitalWrite(snesCartPin, LOW);
      digitalWrite(snesResetPin, HIGH);
              
      addr = 0xFFD5; //65472;  address for room tipe, hirom or lorom
      setDataBusDir(INPUT);
      writeAddrBus(bank, addr); // read byte for type rom 
      //delayMicroseconds(60);
      numerorom = (readDataBus());
      //Serial.print(numerorom);
      if (numerorom==49) {Serial.print("HIROM");} //HIROM
      if (numerorom==32) {Serial.print("LOROM");} //LOROM
      if (numerorom==48) {Serial.print("LOROM/FastROM");} //LOROM

      Serial.print("*");

      

      addr = 0xFFD7; // addres for ROM SIZE,
      writeAddrBus(bank, addr); // read byte for rom size
      delayMicroseconds(60);
      data = (readDataBus());
      Serial.print(round(pow (2,data))); //tamaño en Kbytes
      Serial.print("*");
      Serial.print(int(0.008*round(pow (2,data)))); // tamaño en Mbits
      Serial.print("*");




      addr = 0xFFDA; // addres for ID DEVELOPER AND TO CHECK IF WE HAVE AN EXTENDED HEADER
      writeAddrBus(bank, addr); // read byte for ram size
      delayMicroseconds(60);
      data = (readDataBus());
      if (data==51){ //0X33 =51 //SHOW THAT THERE IS A EXPANDED HEADER
        addr = 0xFFBD; // addres for SRAM SIZE IN EXTENDED HEADER
        writeAddrBus(bank, addr); // read byte for ram size
        delayMicroseconds(60);
        data = (readDataBus());   //puede ser que tenga cabecera extendida pero la ram la tenga en el lugar normal, en ese caso ffbd será 0
        if (data !=0){ // expanded header and sram information in extender header
          Serial.print(round(pow(2,data))); //sram KBytes
          Serial.print("*");
          Serial.print(8*round(pow(2,data))); //sram Kbits
          Serial.print("*");
        }else{ //expanded header but sram information in standar address
          addr = 0xFFD8; // addres for SRAM SIZE
          writeAddrBus(bank, addr); // read byte for ram size
          delayMicroseconds(60);
          data = (readDataBus());
          Serial.print(round(pow(2,data))); //sram KBytes
          Serial.print("*");
          Serial.print(8*round(pow(2,data))); //sram Kbits
          Serial.print("*");

        }
      }else{ //not expanded but standar header
        addr = 0xFFD8; // addres for SRAM SIZE
        writeAddrBus(bank, addr); // read byte for ram size
        delayMicroseconds(60);
        data = (readDataBus());
        Serial.print(round(pow(2,data))); //sram KBytes
        Serial.print("*");
        Serial.print(8*round(pow(2,data))); //sram Kbits
        Serial.print("*");

      }

      addr = 0xFFD9; // addres for REGION, 0=JAPAN, 1=EEUU 2=EUR
      writeAddrBus(bank, addr); 
      delayMicroseconds(60);
      data = (readDataBus());
      if (data==0 ){ Serial.print("JAPAN");}
      if (data==1 ){ Serial.print("EEUU");}
      if (data==2 ){ Serial.print("EUROPE");}
      Serial.print("*");

      addr = 0xFFC0; // addres for name. 21 bytes  
      for (int i=0; i<21; i++){
        writeAddrBus(bank, addr);
        delayMicroseconds(60);//delay(1); //change to 2 if you get reading errors.
        numero = (readDataBus());
        //Serial.println(numero);
        //letra = String(numero); 
        Serial.print(numero);
        Serial.print("*");        
  
        addr++;  
      }

    }
  
 estado=1;
 Byte_recibido="0";
  digitalWrite(snesReadPin, LOW);
  digitalWrite(snesWritePin, LOW);
  digitalWrite(snesCartPin, LOW);
  digitalWrite(snesResetPin, LOW);
}

////////////////////////////////////////////////////////////////////////ç//////
/////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
if (Byte_recibido == 'a'){  /// descargamos la rom

  if (estado==0){ //estado es para que se ejecute solo una vez
    //ESTE IF ES PARA COMPROBAR SI ES LOROM O HIROM

    digitalWrite(snesReadPin, LOW);
    digitalWrite(snesWritePin, HIGH);
    digitalWrite(snesCartPin, LOW);
    digitalWrite(snesResetPin, HIGH);
            
    addr = 0xFFD5; //65472;  address for room tipe, hirom or lorom
    setDataBusDir(INPUT);
    writeAddrBus(bank, addr); // read byte for type room 
    delayMicroseconds(60);
    numerorom = (readDataBus());
    Serial.println(numerorom);
  
    

    addr = 0xFFD7; // addres for rom size, I need it to calculate endbank
    setDataBusDir(INPUT);
    writeAddrBus(bank, addr); // read byte for type room 
    delayMicroseconds(60);
    romsize = (readDataBus());
    if(numerorom==49){ banksize = round(pow (2,romsize)/64);} // HIROM calculo el numero de banks que ocupa la rom
    if(numerorom==32 || numerorom==48){ banksize = round(pow (2,romsize)/32);} // LOROM calculo el numero de banks que ocupa la rom
    //Serial.println(banksize); // debug
        
     if (numerorom==32 || numerorom==48){//LOROM  0X20 = 32 // 20 en hex es 32 en decimal 
        digitalWrite(snesReadPin, LOW);
        digitalWrite(snesWritePin,HIGH); 
        digitalWrite(snesCartPin, LOW);
        digitalWrite(snesResetPin, HIGH);
        setDataBusDir(INPUT);
        bank = 0;
        endbank= bank + banksize; //uno mas del ultimo porque el if se ejecuta despues de sumar 
        addr = 0x8000; //65472;   DIRECCION INICIAL 
        endAddr = 0xFFFF; //65503; DIRECCION FINAL UN BYTE MENOS PORQUE EL ULTIMO SE LO PASA.
        estado=1; //para que no se repita este if
        Serial.println("<"); //caracter de inicio para que el programa en python comience a escribir el archivo
        //delay(20);

      }else if (numerorom==49){// HIROM  //0X30 = 48 // 31 en hex es 49 en decimal
        digitalWrite(snesReadPin, LOW);
        digitalWrite(snesWritePin,HIGH); 
        digitalWrite(snesCartPin, LOW);
        digitalWrite(snesResetPin, HIGH);
        bank = 192; 
        endbank= bank + banksize;
              
        addr = 0x0000; //;   DIRECCION INICIAL  DUMP HIROM
        endAddr = 0xFFFF; //; DIRECCION FINAL DUMP HIROM
        estado=1; //para que no se repita este if

        Serial.println("<"); //caracter de inicio para que el programa en python comience a escribir el archivo
        //delay(20);
      } else {
        Serial.println("Error reading type ROM");
        errorRom=1;
        estado=1; //para que no se repita este if
        delay(20);
        solounavez=0;
        estado=0;
      }
  } 
      
      
      //setDataBusDir(INPUT);
     //Serial.println(millis());
        while (true){
        if (solounavez==0 && errorRom==0){
          //Serial.print("bank:");//debug
          //Serial.print(bank);//debug
          //Serial.print("--"); //debug
          //Serial.print("addr:"); //debug
          //Serial.print(addr,HEX); //debug
          //Serial.print("--"); //debug

        //empezamos a mandar bytes
        //mando el byte de apertura para que el programa en python empiece a leer "<"
        
         writeAddrBus(bank, addr);
          delayMicroseconds(2);//delay(1); //change to 2 if you get reading errors.
          numero = (readDataBus());
          //Serial.println(numero);
          letra = String(numero,HEX);         
        if (numero<16){
          Serial.print("0");
        }
        Serial.println(readDataBus(),HEX);
        
        
        //cadena.concat(letra); // the title is readed char by char, this line put togheter all the character
        //cadenanumero.concat(numero_fix);
        }  
          
          if (addr == endAddr ) {
            if(numerorom==49){addr = 0x0000;} // HIROM reseteo el addr al principio para leer el siguiente bank
            if(numerorom==32 || numerorom==48){addr = 0x8000;} // LOROM reseteo el addr al principio para leer el siguiente bank
            //Serial.println(bank); //debug
            bank = bank +1;
            if (bank == endbank ) { // uno mas del final que es 0xff 255, 
                //Serial.println(millis());
                //Serial.println("fin");
                
                
              if(solounavez==0){
          //    Serial.println(cadena);
                solounavez=1;
                cadena="";
                cadenanumero="";
                Serial.println(">"); //caracter de salida//ya he terminado, Envio dato de cierre ">"
                
              
              }
            }
            
            break;
            
          }
        Serial.flush();
        addr++;    
   }

}



if (Byte_recibido == 'c'){ //  Backup Sram
//solounavez=0;
// HIROM --> SRAM FROM 0xB06000 TO 0xB07FFF /if 8kb of ram per bank // si la sram es mayor vamos recorriendo los bank hasta llegar, no incluido el C0
// LOROM --> SRAM FROM 0xF00000 TO 0xF07FFF /if 32kb of ram per bank // si la seram es mayor vamos recorriendlo los bank hasta FF
//0x00FFD8 1 byte SRAM Size

  if (estado==0){ //estado es para que se ejecute solo una vez
            //ESTE IF ES PARA COMPROBAR SI ES LOROM O HIROM

            digitalWrite(snesReadPin, LOW);
            digitalWrite(snesWritePin, HIGH);
            digitalWrite(snesCartPin, LOW);
            digitalWrite(snesResetPin, HIGH);
            
            addr = 0xFFD5; //65472;  address for room tipe, hirom or lorom
            bank=0;
            setDataBusDir(INPUT);
            writeAddrBus(bank, addr); // read byte for type room 
            delayMicroseconds(60);
            numerorom = (readDataBus());
            //Serial.print("Romtype 49hirom 32lorom: "); //debug
            //Serial.println(numerorom);  //debug

            addr = 0xFFD6; //  address for special chips
            bank=0;
            setDataBusDir(INPUT);
            writeAddrBus(bank, addr); // read byte for type room 
            delayMicroseconds(60);
            specialchip = (readDataBus());
            

      ///////// RAM SIZE //////////////

        ////*****///
        addr = 0xFFDA; // addres for ID DEVELOPER AND TO CHECK IF WE HAVE AN EXTENDED HEADER
        writeAddrBus(bank, addr); // read byte for ram size
        delayMicroseconds(60);
        data = (readDataBus());

        if (data==51){ //0X33 =51 //SHOW THAT THERE IS A EXPANDED HEADER
          addr = 0xFFBD; // addres for SRAM SIZE IN EXTENDED HEADER
          writeAddrBus(bank, addr); // read byte for ram size
          delayMicroseconds(60);
          data = (readDataBus());   //puede ser que tenga cabecera extendida pero la ram la tenga en el lugar normal, en ese caso ffbd será 0
          if (data !=0){ // expanded header and sram information in extender header
            ramsize=data;
          }else{ //expanded header but sram information in standar address
            addr = 0xFFD8; // addres for SRAM SIZE
            writeAddrBus(bank, addr); // read byte for ram size
            delayMicroseconds(60);
            ramsize = (readDataBus());
            
          }
        }else{ //not expanded but standar header
          addr = 0xFFD8; // addres for SRAM SIZE
          
          writeAddrBus(bank, addr); // read byte for ram size
          delayMicroseconds(60);
          ramsize = (readDataBus());
          
      }
      
      ///****/////
      


            if(numerorom==49){ // HIROM calculo el numero de banks que ocupa la ram, 8kb por bank
              if (ramsize==3){
                  banksize =0; //exactamente ocupa 1 bank.
              }else{
                banksize = round(pow (2,ramsize)/8);
              }
              if (banksize!=0){banksize = banksize-1;} //estoy es porque el bucle de lectura suma uno al bank al final
            }

            if(numerorom==32 || numerorom==48){ // LOROM calculo el numero de banks que ocupa la ram
              if (ramsize==5){  //ocupa justo el primer bank
                banksize=0;
              }else{      
                banksize = round(pow (2,ramsize)/32); 
              }
              if (banksize!=0){banksize = banksize-1;} //estoy es porque el bucle de lectura suma uno al bank al final
            }
       
            if (numerorom==32 || numerorom==48){//LOROM  0X20 = 32 // 20 en hex es 32 en decimal 
              digitalWrite(snesReadPin,LOW);
              digitalWrite(snesWritePin,HIGH); 
              digitalWrite(snesCartPin, LOW);
              digitalWrite(snesResetPin, HIGH);
              setDataBusDir(INPUT);

              if ((specialchip == 19) || (specialchip == 20) || (specialchip == 21) || (specialchip == 26)) { //superFX (Yoshi island, Stut race, Wildtrax...)
                bank=0x70 ; // 
              } else{
                bank =240 ; // 0xF0;
              }
              endbank= bank + banksize; //uno mas del ultimo porque el if se ejecuta despues de sumar 
              addr = 0x0000; //   DIRECCION INICIAL 
              if (ramsize>=5){ //si ramsize es 5 implica 32KB que es lo maximo para 1 bank. si es así leemos el bank entero. que acaba en 0x7fff
                endAddr=0x7FFF;
                if(ramsize==5){endbank=bank;} //para mario paint, ocupa justo un bank.
              }else {
                endAddr = round(addr + pow (2,ramsize)*1024 -1);
              }
              estado=1; //para que no se repita este if
              delay(20);
              Serial.println("<"); //caracter de inicio para que el programa en python comience a escribir el archivo

            }else if (numerorom==49){// HIROM  //0X31 = 49 // 31 en hex es 49 en decimal
              digitalWrite(snesReadPin, LOW);     //con chronotriguer funciona low,high,high,high, y sin delay despues. CON MARIO KART TAMBIEN
              digitalWrite(snesWritePin, HIGH); 
              digitalWrite(snesCartPin, HIGH);   // tiene que estar el interruptor arriba para dar los 5V al cartucho, aunque algunos juegos funcionan sin hacerlo.
              digitalWrite(snesResetPin,HIGH);
              setDataBusDir(INPUT);
              bank = 0xB0; // 0xb0; // 0XB0;  0x20
              endbank= bank + banksize;
              addr = 0x6000; //;   DIRECCION INICIAL  DUMP HIROM
              if (ramsize>=3){ //si ramsize es 3 implica 8KB que es lo maximo para 1 bank. si es así leemos el bank entero. que acaba en 0x7fff
                endAddr=0x7fff;
              }else {
                endAddr = round(addr + pow (2,ramsize)*1024 -1);
              }
            
              estado=1; //para que no se repita este if
              //delay(5000);
              //delayMicroseconds(1); // con delay de 60 micro aquí y 5 micro luego es como funcionó en el laboratorio. ahora no funciona super mario kart jap
              Serial.println("<"); //caracter de inicio para que el programa en python comience a escribir el archivo
            } else {
              Serial.println("Error reading type ROM");
              errorRom=1;
              estado=1; //para que no se repita este if

            }
          } 

        while (true){
        if (solounavez==0 && errorRom==0){
          writeAddrBus(bank, addr);
          delayMicroseconds(5);// you can change that if you are getting errors
          numero = (readDataBus());
          if (numero<16){
            Serial.print("0");
          }

          Serial.println(numero,HEX);
        

        }  
          
          if (addr == endAddr ) { 
                     
            if(numerorom==49){addr = 0x6000;} // HIROM reseteo el addr al principio para leer el siguiente bank
            if(numerorom==32 || numerorom==48){addr = 0x0000;} // LOROM reseteo el addr al principio para leer el siguiente bank
            if (bank == endbank ) { // uno mas del final que es 0xff 255, 
                 
              if(solounavez==0){
                solounavez=1;
                cadena="";
                cadenanumero="";
                Serial.println(">"); //caracter para finalizar el archivo
                digitalWrite(snesReadPin,LOW);
                digitalWrite(snesWritePin,LOW); 
                digitalWrite(snesCartPin, LOW);
                digitalWrite(snesResetPin, LOW);
                
              
              }
            }
            bank++; 
            break;
            
            
          }
        Serial.flush();
        addr++;   
        }
  
} // fin del dump sram
 

  
} //fin del void loop

byte serialReadBlocking() {
  while(Serial.available() == 0);  //para leer la cabecera llega [1010]   [read, write, cart, reset] ( en 1 están read, y cart)
  return Serial.read();
}

unsigned int bytesToInt(byte hi, byte low) {
  return ((unsigned int) hi << 8) | low;
}

void setCtrlLines(byte s) {
 // digitalWrite(snesReadPin, s & 0x8);
 // digitalWrite(snesWritePin, s & 0x4);
  //digitalWrite(snesCartPin, s & 0x2);
  //digitalWrite(snesResetPin, s & 0x1);
//PEDRO, PARA LEER LA CABECERA
digitalWrite(snesReadPin, HIGH);
 digitalWrite(snesCartPin,HIGH); 
}

/* Write a value out to the address bus
 * Uses direct port manipulation and
 * hardware spi for better performance
 */
void writeAddrBus(byte bank, unsigned int addr) {
  PORTB &= ~(B100); //Set AddressLatchPin low
  SPI.transfer(bank); // shift out bank
  SPI.transfer(addr >> 8); // shift out address upper byte
  SPI.transfer(addr); // shift out address lower byte
  PORTB |= (B100); //Set AddressLatchPin high
}

//Read byte from data bus
byte readDataBus(){
  //Digital pins 2-7 (PIND) are connected to snes data lines 2-7.
  //Digital pins 8 and 9 (PINB) are connected to data lines 0 and 1 respectively
  //This line of code takes the data from pins 2-7 and from 8&9 and combines them into one byte.
  //The resulting bye looks like this: (pin7 pin6 pin5 pin4 pin3 pin2 pin9 pin8)
  return (PIND & ~0x03) | (PINB & 0x03);

}

//Write byte to data bus
void writeDataBus(byte data) {
  digitalWrite(8, bitRead(data, 0));
  digitalWrite(9, bitRead(data, 1));
  for (int i = 2; i < 8; i++) {
    digitalWrite(i, bitRead(data, i));
  }
}

//Set the data bus to output or input
// false => INPUT, true => OUTPUT
void setDataBusDir(bool dir) {
  for (int p = 2; p <=9; p++){
    pinMode(p, dir);
  }
}
