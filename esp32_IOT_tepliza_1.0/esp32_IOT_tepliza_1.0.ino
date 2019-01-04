/*Автор Ермеков Рустем
 *По всем вопросам rustem.erm@gmail.com
 *Проект сделан в Школе инженерного творчества STARKID.kz
 *ESP32 Tepliza AP 1.0
 *Проект теплица с подключением через созданную точку доступа.
 *Инструкция и схема проекта лежат по ссылке https://github.com/iotrusya/esp32Teplica
 *Для работы с платой ESP32 в среде Arduino IDE. Скачайте инструкцию https://github.com/iotrusya/iotESP32ssd1306OLED
 *Пример предназначен для работы с платами ESP32 с установленными OLED экранами контроллере SSD1306
 *Дата: 03.01.2019
*/

#include "starkidlogo.h"//Хранится файл с изображением
#include <OneWire.h>
#include <WiFi.h>

/* Введите данные своей точки доступа */
const char* ssid = "ESP32Tepliza 1.0";  // Название точки доступа 
const char* password = "123456789";  // Пароль для точки доступа

/* Данные сервера */
IPAddress local_ip(192,168,1,1); //IP адрес
IPAddress gateway(192,168,1,1); //Шлюз
IPAddress subnet(255,255,255,0); //Маска сети
WiFiServer server(80);

/* OLED экран SSD1306 */
#include "SSD1306.h" 
#define SDA_PIN 4// GPIO4 -> SDA
#define SCL_PIN 15// GPIO15 -> SCL
#define SSD_ADDRESS 0x3c //I2C адрес экрана
SSD1306  display(SSD_ADDRESS, SDA_PIN, SCL_PIN);

OneWire ds(17); //порт датчика температуры ds18b20. Датчик температуры для почвы.

int lightSensor = 0; // Фоторезистор переменная для хранения уровня освещения
int MoistureSensor = 0; //Датчик влажности почвы переменная для хранения влажности

/*Обьявляем порты реле*/
int relay1 = 19; //Обьявлем реле 1. Светодиодная полоса
int relay2 = 23; //Обьявлем реле 2. Насос для воды
int relay3 = 18; //Обьявлем реле 3. Кулер дял воздуха
int relay4 = 5; //Обьявлем реле 4. Запасное реле. Для дополнительных функций

/* DHT датчик температуры и влажности воздуха */
#include "DHT.h"
#define DHTPIN 22  
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
float localHum = 0; // Переменная для хранения уровня влажности воздуха
float localTemp = 0; // Переменная для хранения уровня температуры воздуха

#define ctsPin 21 // Пин Сенсорного модуля ttp223b
boolean touch = 1;// Переменная для переключения экранов

float temperature; //Переменная для хранения температуры почвы с датчика ds18b20

/*Переменные для подключения веб-сервера*/
char linebuf[80]; 
int charcount=0;

void setup() {
  Serial.begin(115200); //Настраиваем serial подключение с компьютером
  
  pinMode(ctsPin, INPUT);//Обьявляем порт как Входящий. Порт сенсорного модуля

  pinMode(relay1,OUTPUT); //Обьявляем порт как Выходящий. Реле 1
  pinMode(relay2,OUTPUT); //Обьявляем порт как Выходящий. Реле 2
  pinMode(relay3,OUTPUT); //Обьявляем порт как Выходящий. Реле 3
  pinMode(relay4,OUTPUT); //Обьявляем порт как Выходящий. Реле 4
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);
  
  /*Включаем и настраиваем экран*/
  pinMode(16,OUTPUT); 
  digitalWrite(16, LOW);    
  delay(50); 
  digitalWrite(16, HIGH); 
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10); // установка шрифта варианты 10, 15, 25
  Serial.println("OLED экран SSD1306 настроен");
  /*----------------------------*/
  
  dht.begin(); // Включаем датчик DHT22
  Serial.println("Датчик DHT22 включен");

  /*Настраиваем и включаем точку доступа*/
  WiFi.softAP(ssid, password);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  server.begin(); //Запускаем веб сервер
  Serial.println("Точка доступа и сервер запущены");
  /*-----------------------------------*/

  /*Запускаем приветсвие на экране при включении*/
  display.clear();
  display.drawXbm(0, 0, starkidlogo_width, starkidlogo_height, starkidlogo_bits); // Рисуем нашу картинку
  display.display();
  delay(2000);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 32, "IoT Tepliza 1.0 by Starkid");
  display.display();
  delay(2000);
  /*--------------------------------------------*/
}

void loop() {  
 getLight(); //Считывание данных освещения
 getMoist(); //Считывание данных влажности почвы
 getDHT(); //Считывание влажности и температуры воздуха
 getTemp(); //Считывание температуры почвы
 displayData(); //Отображение данных на экране
 wifiAPserver(); //Запуск сервера и показ данных 
 delay(100);
}

void getDHT(){
  float tempIni = localTemp;
  float humIni = localHum;
  localTemp = dht.readTemperature();
  localHum = dht.readHumidity();
  if (isnan(localHum) || isnan(localTemp))   
  {
    localTemp = tempIni;
    localHum = humIni;
    return;
  }
}

void displayData() {
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  int ctsValue = digitalRead(ctsPin);//Считывание данных с сенсорного датчика
  if (ctsValue == HIGH){
    display.clear();   // Очистка дисплея
    display.drawString(0, 0,  "light: " + String(lightSensor) + "%");
    display.drawString(0, 20, "moisture: " + String(MoistureSensor) + "%");
    display.drawString(0, 40, "air temp: " + String(localTemp) + "C");
    display.display();   // Отображение данных на экране из буфера
  }
  if (ctsValue == LOW){
    display.clear();   // Очистка дисплея
    display.drawString(0, 0, "humidity: " + String(localHum) + "%");
    display.drawString(0, 20, "gr temp: " + String(temperature) + "C");
    //display.drawString(0, 40, "r1:0 r2:0 r3:0 r4:0");
    display.display();   // Отображение данных на экране из буфера
  } 
  delay(10);
}

void getTemp() {
  // Определяем температуру от датчика DS18b20
  byte data[2]; // Место для значения температуры
  ds.reset(); // Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  ds.write(0xCC); // Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  ds.write(0x44); // Даем датчику DS18b20 команду измерить температуру. Само значение температуры мы еще не получаем - датчик его положит во внутреннюю память
  delay(1000); // Микросхема измеряет температуру, а мы ждем.    
  ds.reset(); // Теперь готовимся получить значение измеренной температуры
  ds.write(0xCC); 
  ds.write(0xBE); // Просим передать нам значение регистров со значением температуры
  // Получаем и считываем ответ
  data[0] = ds.read(); // Читаем младший байт значения температуры
  data[1] = ds.read(); // А теперь старший
  // Формируем итоговое значение: 
  //    - сперва "склеиваем" значение, 
  //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
  temperature =  ((data[1] << 8) | data[0]) * 0.0625;
  }

void getLight() {
 int lightRead = analogRead(36);//Считывание данных с фоторезистора.
 lightSensor = map(lightRead, 0, 4095, 0, 100); //Преобразовывание данных об освещенности в проценты
    }

void getMoist() {
 int moistRead  = analogRead(37); //Считывание данных с порта 
 MoistureSensor = 100 - map(moistRead, 2000, 3500, 0, 100); //Преобразование данных в проценты
  }

void wifiAPserver(){
    // анализируем канал связи, высматривая входящих клиентов:
  WiFiClient client = server.available();
  if (client) {
    Serial.println("New client");  //  "Новый клиент"
    memset(linebuf,0,sizeof(linebuf));
    charcount=0;
    // HTTP-запрос заканчивается пустой строкой:
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        // считываем HTTP-запрос, символ за символом:
        linebuf[charcount]=c;
        if (charcount<sizeof(linebuf)-1) charcount++;
        // если добрались до конца строки (т.е. получили
        // символ новой строки) и строка пуста,
        // это значит, что HTTP-запрос закончился;
        // следовательно, можно отправлять ответ:
        if (c == '\n' && currentLineIsBlank) {
          // отправляем стандартный заголовок HTTP-ответа:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");  
                     //  "Тип контента: text/html"
          client.println("Connection: close");
                     //  "Соединение: отключено";
                     //  после отправки ответа связь будет отключена
          client.println();
          client.println("<!DOCTYPE HTML><html><head>");
          client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head>");
          client.println("<h1>ESP32 - Web Server</h1>");
          client.println("<p>PUMP  <a href=\"on1\"><button>ON</button></a>&nbsp;<a href=\"off1\"><button>OFF</button></a></p>");
          client.println("<p>LED  <a href=\"on2\"><button>ON</button></a>&nbsp;<a href=\"off2\"><button>OFF</button></a></p>");
          client.println("<p>COOLER  <a href=\"on3\"><button>ON</button></a>&nbsp;<a href=\"off3\"><button>OFF</button></a></p>");
          client.println("<p>RELAY  <a href=\"on4\"><button>ON</button></a>&nbsp;<a href=\"off4\"><button>OFF</button></a></p>");
          client.println("AIR TEMPERATURE</p><p>");
          client.println(localTemp);
          client.println("*C</p><p>");
          client.println("AIR HUMIDITY</p><p>");
          client.println(localHum);
          client.println("%</p></div>");
          client.println("LIGHT SENSOR</p><p>");
          client.println(lightSensor);
          client.println("%</p></div>");
          client.println("MOISTURE SENSOR</p><p>");
          client.println(MoistureSensor);
          client.println("%</p></div>");
          client.println("GROUND TEMP</p><p>");
          client.println(temperature);
          client.println("*C</p></div>");
          client.println("</body></html>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          // начинаем новую строчку:
          currentLineIsBlank = true;
          if (strstr(linebuf,"GET /on1") > 0){
            Serial.println("Relay 1 ON");
            digitalWrite(relay1, LOW);
          }
          else if (strstr(linebuf,"GET /off1") > 0){
            Serial.println("Relay 1 OFF");
            digitalWrite(relay1, HIGH);
          }
          else if (strstr(linebuf,"GET /on2") > 0){
            Serial.println("Relay 2 ON");
            digitalWrite(relay2, LOW);
          }
          else if (strstr(linebuf,"GET /off2") > 0){
            Serial.println("Relay 2 OFF");
            digitalWrite(relay2, HIGH);
          }
          else if (strstr(linebuf,"GET /on3") > 0){
            Serial.println("Relay 3 ON");
            digitalWrite(relay3, LOW);
          }
          else if (strstr(linebuf,"GET /off3") > 0){
            Serial.println("Relay 3 OFF");
            digitalWrite(relay3, HIGH);
          }
          else if (strstr(linebuf,"GET /on4") > 0){
            Serial.println("Relay 4 ON");
            digitalWrite(relay4, LOW);
          }
          else if (strstr(linebuf,"GET /off4") > 0){
            Serial.println("Relay 4 OFF");
            digitalWrite(relay4, HIGH);
          }
          // начинаем новую строчку:
          currentLineIsBlank = true;
          memset(linebuf,0,sizeof(linebuf));
          charcount=0;
        } else if (c != '\r') {
          // в строке попался новый символ:
          currentLineIsBlank = false;
        }
      }
    }
    // даем веб-браузеру время, чтобы получить данные:
    delay(1);
 
    // закрываем соединение:
    client.stop();
    Serial.println("client disconnected");  //  "клиент отключен"
  }
  }
