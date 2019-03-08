

#include <SoftwareSerial.h>//LIBRERIA PARA CREAR TX RX
#include <TinyGPS.h> // LIBRERIA DEL GPS MIKAL HART GITHUB
#include <SD.h> // LIBRERIA SD
#include <stdlib.h>// INCLUDES PARA EL MODULO A7
#include<stdio.h> 
#include<string.h> //  LIBRERIA PARA LAS CADENAS DE VALORES
#define DEBUG true
#include <LiquidCrystal_I2C.h> // Libreria LCD_I2C

const int sensorPin= A0; //PIN SENSOR LM35
const int sensorPin1= A1; //PIN SENSOR LM35

File dataFile;
LiquidCrystal_I2C lcd(0x27,16,2); // si no te sale con esta direccion  puedes usar (0x3f,16,2) || (0x27,16,2)  ||(0x20,16,2) DEPENDIENDO DE TU PANTALLA
TinyGPS gps;
SoftwareSerial nss(10, 11); // 10(tx) 11 (rx) MODULO GPS
SoftwareSerial mySerial(12, 13);// 12(tx) 13 (rx) MODULO A7 LLAMADA TELEFONO

static char databuffer[20];
int CS = 53; // PIN CS de la SD RX
int led = 2; //  SI NO TENEMOS LED FISICO PODEMOS PONER EL PIN 13 DE LA PLACA ARDUINO

//Definiciones de cadenas

String SD_date_time = "invalid"; // VALOR DONDE SE GUARDA LA Fecha y Hora
String SD_lat = "invalid"; // VALOR DONDE SE GUARDA LA Latitud
String SD_lon = "invalid"; // VALOR DONDE SE GUARDA LA Longitud
String SD_vel = "invalid"; // VALOR DONDE SE GUARDA LA Velocidad
String SD_cur = "invalid"; // VALOR DONDE SE GUARDA EL Curso (Rumbo)
String SD_gra = "invalid"; // VALOR DONDE SE GUARDA LOS Grados (respecto al GPS)

String dataString = "";

static void gpsdump(TinyGPS &gps);
static bool feedgps();
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void setup()
{
   Serial.begin(9600);
   mySerial.begin(9600);// INICIAMOS PUERTO SERIAL CON MODULO A7 TX RX PIN 12 13 
   delay(1000);
   
   sendData("AT+CPIN=1522",1000,DEBUG); //ACTIVAR AL PRINCIPO UNA VEZ PARA CONECTAR
   delay(1000);
   sendData("AT+CPIN?",1000,DEBUG); // PREGUNTAMOS ESTADO DE LA SIM
   delay(1000);

   sendData("ATD662425496;",1000,DEBUG); //REALIZAR LLAMADA A NUMERO DE TELEFONO
   delay(1000);
   //sendData("ATA",1000,DEBUG);// COMANDO PARA COGER LA LLAMADA
   delay(1000);

  pinMode(CS, OUTPUT); // PIN 53
  pinMode(led, OUTPUT); // LED 13 PLACA ARDUINO

// Interfaces seriales
 Serial.begin(9600);
 nss.begin(9600); //Velc de Tx del GPS 
 
// Conexion a la tarjeta SD
  Serial.print(F("Iniciando SD ..."));
  if(!SD.begin(53));
  {
    Serial.println("Tarjeta SD Inicializada");
    return;
  }

 // Impresion en el monitor serial del Arduino
 Serial.print("MATXINGO Navigation System Tracking "); 
 Serial.println(TinyGPS::library_version());
 Serial.println("by SERGEI SUSPERREGUI");
 Serial.println();
 Serial.print("Sizeof(gpsobject) = "); 
 Serial.println(sizeof(TinyGPS));
 Serial.println();         
 Serial.println("Sats HDOP Latitud  Longitud  Fix  Fecha      Hora      Fecha Alt     Curso Velc Grados  Distanc Curso Grados  Chars Sentences Checksum");
 Serial.println("          grados   grados    Age                        Age  (m)     --- del  GPS ----  ----  hacia WP  ----  RX    RX        Fail");
 Serial.println("--------------------------------------------------------------------------------------------------------------------------------------");

}
void loop()
{
  lcd.display(); //LCD
  delay(500);
  //lcd.noDisplay(); // SI DESACTIVAMOS QUITAMOS EL PARPADEO
  //delay(500);
  
  int value = analogRead(sensorPin); // SENSOR 0
  float millivolts = (value / 1023.0) * 5000;
  float celsius = millivolts / 10; 
  delay(1000);
  int value2 = analogRead(sensorPin1); //SENSOR 1
  millivolts = (value2 / 1023.0) * 5000;
  float celsius1 = millivolts / 10; 
  
  Serial.print(celsius); //VALOR TEMP MOTOR
  Serial.println(" C MOTOR");
  Serial.print(celsius1); // VALOR TEMP BATERIA
  Serial.println(" C BATERIA");

  lcd.init();   //PARTE PROGRAMA LCD
  lcd.backlight();
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print("MOTOR"); //VEMOS POR LCD
  lcd.setCursor (0,1);
  lcd.print(celsius); //VEMOS POR LCD
  lcd.setCursor (5,1);
  lcd.print("C");
  
  lcd.setCursor(7,0);
  lcd.print("BATERIA"); //VEMOS POR LCD
  lcd.setCursor (7,1);
  lcd.print(celsius1); //VEMOS POR LCD
  lcd.setCursor (12,1);
  lcd.print("C");
  
  delay(1000);
  
 bool newdata = false;
 unsigned long start = millis();

 // Every second we print an update
 while (millis() - start < 1000)
 {
   if (feedgps())
     newdata = true;
 }

 gpsdump(gps);
 
 // Escritura de la info en la SD
 
  //AQUI PONGO ALGUNAS SENTENCIAS DE EJEMPLO DE ESCRITURA SELECCCIONAMOS LA QUE MAS NOS CONVENGA
  
 //dataString = SD_date_time + "," + SD_lat + "," + SD_lon + "," + SD_vel + "," + SD_cur + "," + SD_gra; //ESCRIBE EN LA SD FECHA HORA LATITUD LONGITUD VELOCIDAD CURSO Y GRADOS 
 
 //dataString = SD_date_time + "," + SD_lat + "," + SD_lon; //ESCRIBE EN LA SD FECHA HORA LATITUD Y LONGITUD

 //dataString =  SD_lat + "," + SD_lon + "," + celsius+ "," + celsius1;// ESCRIBE EN LA SD LATITUD TEMP MOTOR Y TEMP BATERIA
 
   dataString = SD_lat + "," + SD_lon;
  
 if(SD_date_time != "invalid")
 
  //digitalWrite(led, HIGH); // FUNCIONANDO (SI NO QUEREMOS USAR FUNCIONES) 
  blinkSlowly(); // FUNCIONANDO (LLAMADA A LA FUNCION DE PARPADEO DEL LED)
 else
  //digitalWrite(led,LOW); //CUANDO NO FUNCIONA (SI NO QUEREMOS USAR FUNCIONES)
  blinkQuickly(); //LLAMADA A LA FUNCION DE PARPADEO DEL LED
  
// Abriendo el archivo CSV para grabar los datos
  File dataFile = SD.open("ruta.csv", FILE_WRITE);  //  ruta.csv EXCEL separado por comas    PODEMOS PONER .txt PARA FORMATO NOTAS
  if (dataFile)
  {
    dataFile.println(dataString);
    Serial.println(dataString);
    dataFile.close();
  }
  else
  {
    Serial.println("\nNo se puede abrir el archivo DE LA SD!");
  }
}

static void gpsdump(TinyGPS &gps)
{
 float flat, flon;
 unsigned long age, date, time, chars = 0;
 unsigned short sentences = 0, failed = 0;
 static const float LONDON_LAT = 51.508131, LONDON_LON = -0.128002; // PUNTO DE REFERENCIA RESPECTO A LONDRES , SI CAMBIAMOS LOS VALORES DE LATITUD Y LONGITUD POR OTRA POSICION, CAMBIAREMOS SU REFERENCIA RESPECTO A LAS NUEVAS COORDENADAS INTRODUCIDAS

 print_int(gps.satellites(), TinyGPS::GPS_INVALID_SATELLITES, 5);
 print_int(gps.hdop(), TinyGPS::GPS_INVALID_HDOP, 5);
 gps.f_get_position(&flat, &flon, &age);
 print_float(flat, TinyGPS::GPS_INVALID_F_ANGLE, 9, 5, 1); // Latitud
 print_float(flon, TinyGPS::GPS_INVALID_F_ANGLE, 10, 5, 2); // Longitud
 print_int(age, TinyGPS::GPS_INVALID_AGE, 5);

 print_date(gps); // Fecha y Hora

 print_float(gps.f_altitude(), TinyGPS::GPS_INVALID_F_ALTITUDE, 8, 2, 0);
 print_float(gps.f_course(), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2, 0);
 print_float(gps.f_speed_kmph(), TinyGPS::GPS_INVALID_F_SPEED, 6, 2, 0);
 print_str(gps.f_course() == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(gps.f_course()), 6);
 print_int(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0UL : (unsigned long)TinyGPS::distance_between(flat, flon, LONDON_LAT, LONDON_LON) / 1000, 0xFFFFFFFF, 9);
 print_float(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : TinyGPS::course_to(flat, flon, 51.508131, -0.128002), TinyGPS::GPS_INVALID_F_ANGLE, 7, 2, 0);
 print_str(flat == TinyGPS::GPS_INVALID_F_ANGLE ? "*** " : TinyGPS::cardinal(TinyGPS::course_to(flat, flon, LONDON_LAT, LONDON_LON)), 6);

 gps.stats(&chars, &sentences, &failed);
 print_int(chars, 0xFFFFFFFF, 6);
 print_int(sentences, 0xFFFFFFFF, 10);
 print_int(failed, 0xFFFFFFFF, 9);
 Serial.println();
 
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
 char sz[32];
 if (val == invalid)
   strcpy(sz, "*******");
 else
   sprintf(sz, "%ld", val);
 sz[len] = 0;
 for (int i=strlen(sz); i<len; ++i)
   sz[i] = ' ';
 if (len > 0) 
   sz[len-1] = ' ';
 Serial.print(sz);
 feedgps();
}

static void print_float(float val, float invalid, int len, int prec, int SD_val)
{
 char sz[32];
 if (val == invalid)
 {
   strcpy(sz, "*******");
   sz[len] = 0;
    if (len > 0) 
      sz[len-1] = ' ';
   for (int i=7; i<len; ++i)
    sz[i] = ' ';
   Serial.print(sz);
  if(SD_val == 1) SD_lat = sz;
   else if(SD_val == 2) SD_lon = sz;
 }
 else
 {
   Serial.print(val, prec);
   if (SD_val == 1) SD_lat = dtostrf(val,10,5,databuffer);
   else if (SD_val == 2) SD_lon = dtostrf(val,10,5,databuffer);
  int vi = abs((int)val);
   int flen = prec + (val < 0.0 ? 2 : 1);
   flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
   for (int i=flen; i<len; ++i)
     Serial.print(" ");
 }
 feedgps();
}

static void print_date(TinyGPS &gps)
{
 int year;
 byte month, day, hour, minute, second, hundredths;
 unsigned long age;
 gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
 if (age == TinyGPS::GPS_INVALID_AGE)
 {
   Serial.print("*******    *******    ");
  SD_date_time = "invalid";
 }
 else
 {
   char sz[32];
   sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d   ",
   day, month, year, hour, minute, second); //configuracion formato fecha y hora
   Serial.print(sz);
  SD_date_time = sz;
 }
 print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
 feedgps();
}

static void print_str(const char *str, int len)
{
 int slen = strlen(str);
 for (int i=0; i<len; ++i)
   Serial.print(i<slen ? str[i] : ' ');
 feedgps();
}

static bool feedgps()
{
 while (nss.available())
 {
   if (gps.encode(nss.read()))
     return true;
 }
 return false;
}

String sendData(String command, const int timeout, boolean debug) // FUNCION PARA MANDAR COMANDOS AT AL MODULO A7
{
    String response = "";    
    mySerial.println(command); 
    long int time = millis();   
    while( (time+timeout) > millis())
    {
      while(mySerial.available())
      {       
        char c = mySerial.read(); 
        response+=c;
      }  
    }    
    if(debug)
    {
      Serial.print(response);
    }    
    return response;
}
void blinkQuickly () // FUNCION PARPADEO RAPIDO DEL LED
{
  digitalWrite(led,HIGH);
  delay(100);
  digitalWrite(led,LOW);
  delay(100);
}

void blinkSlowly () // FUNCION PARPADEO LENTO DEL LED
{
  digitalWrite(led,HIGH);
  delay(500);
  digitalWrite(led,LOW);
  delay(500);
}
