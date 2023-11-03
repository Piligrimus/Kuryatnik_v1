#include "RTClib.h"
#include <TM1637Display.h>
// #include <GyverEncoder.h>
// #include <EEPROM.h>
#include <microDS18B20.h>
#include "GyverButton.h"

#include <GyverOS.h>



// Define the connections pins:
#define pin_LED2                14   // LED индикация включения обогрева
#define pin_button              5    // кнопка
#define ONE_WIRE_BUS            11   // DS18B20 подключаем на D11 на плате  (a 4.7K resistor is necessary)
#define rele1                   7    // пин реле дуйчика
#define CLK                     10   // для тм1637
#define DIO                     12   // для тм1637
#define fadePin                 6    // пин управления MOSFET транзистором

// Define timeless:
#define timless_ON              2350  // продолжительность ВКЛючения (размер шага переменной i (0-255) в миллисекундах) света утром  (1 мин = 60 000мс, 1 сек = 1 000мс)
#define timless_OFF             2350  // продолжительность ВЫКЛючения света вечером (2350 это около 5мин) (1 мин = 60 000мс, 1 сек = 1 000мс)

#define TIME_ON_morn            600   // время ВКЛючения света утром  6:00   (время пишется целым числом суммы цифр)
#define TIME_OFF_morn           1000   // время ВЫКЛючения света утром 10:00   (время пишется целым числом суммы цифр)

#define TIME_ON_evening         1600  // время ВКЛючения света вечером 16:00   (время пишется целым числом суммы цифр)
#define TIME_OFF_evening        2100  // время ВЫКЛючения света вечером 21:00   (время пишется целым числом суммы цифр)

#define tempmax                 8     //верхняя граница диапазона температуры обогрева (градус)
#define tempmin                 3     //нижняя граница диапазона температуры обогрева (градус)

// The amount of time (in milliseconds) between tests
#define TEST_DELAY              500   //временная задержка вывода символов при стартовом тесте дисплея
GyverOS<6> OS;	                      // указать макс. количество задач
#define OS_BENCH                      // дефайн до подключения работы по задачам (таскам)

uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };
int k;
// float temp_1;
MicroDS18B20<ONE_WIRE_BUS> sensor;

float Current_Temp;  //currentTempDS18B20;
const int select_button = pin_button;     // переменная кнопки
// boolean DOTSflag = false;     //мигание двоеточием с помощью изменения флага при каждом выполнении TIME

RTC_DS3231 rtc;
TM1637Display display = TM1637Display(CLK, DIO);
// int light;                                            
bool current_state_light;                                                   //переменная для хранения состояния света (вкл/выкл)
             // GButton butt1(pin_button);  
GButton butt1(pin_button, HIGH_PULL, NORM_OPEN);                           // указываем пин, тип и вид кнопки
uint32_t myTimer1, myTimer2, myTimer3, myTimer4, myTimer5, myTimer6;
// int tempbutton;
// int value = 0;


void setup()  //**********************************************************************************************************************
{
  
  //------ подключаем задачи (порядковый номер, имя функции, период в мс) ----
  OS.attach(0, TIME1, 501);
  OS.attach(1, TIME2, 1000);
  OS.attach(2, periodtemp, 60000);
  OS.attach(3, reletemp, 70000);
  OS.attach(4, getSendData_DS18B20, 15000);
  OS.attach(5, chek_light, 30000);
  //--------------------------------------------------------------------------
  current_state_light == false;      // флаг состояния света ставим в положениы ВЫКЛ
  digitalWrite(rele1, HIGH);
  Serial.begin(9600);
  pinMode(pin_LED2, OUTPUT);        // выход на светодиод обогрева
  pinMode(rele1, OUTPUT);           // выход на реле обогрева

  //================= тест светодиода ==================
  digitalWrite(pin_LED2, HIGH);
        delay (500);
  digitalWrite(pin_LED2, LOW);
        delay (500);
          digitalWrite(pin_LED2, HIGH);
        delay (500);
  digitalWrite(pin_LED2, LOW);
        delay (500);
          digitalWrite(pin_LED2, HIGH);
        delay (500);
  digitalWrite(pin_LED2, LOW);
        delay (1000);
  //================= тест реле ==================
  digitalWrite(rele1, LOW);
        delay (1000);
  digitalWrite(rele1, HIGH);
  delay (1000);
    digitalWrite(rele1, LOW);
        delay (1000);
  digitalWrite(rele1, HIGH);
//================ тест LED-ленты ================
  delay (1000);
  analogWrite(fadePin, 85);
  delay (500);
  analogWrite(fadePin, 170);
  delay (500);
  analogWrite(fadePin, 255);
  delay (500);
  analogWrite(fadePin, 170);
  delay (500);
  analogWrite(fadePin, 85);
  delay (500);
  analogWrite(fadePin, 0);
  delay (50);
//================================================
  rtc.begin();
  // rtc.adjust(DateTime(2023, 10, 23, 5, 58, 0));
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // Check if RTC is connected correctly:
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
  }
  // Check if the RTC lost power and if so, set the time:
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
  }
///////////////////////////// BUTTON ///////////////////////////////////////////////////

  butt1.setDebounce(80);        // настройка антидребезга (по умолчанию 80 мс)
  butt1.setTimeout(2000);        // настройка таймаута на удержание (по умолчанию 500 мс)
  butt1.setClickTimeout(300);   // настройка таймаута между кликами (по умолчанию 300 мс)

  // HIGH_PULL - кнопка подключена к GND, пин подтянут к VCC (BTN_PIN --- КНОПКА --- GND)
  // LOW_PULL  - кнопка подключена к VCC, пин подтянут к GND
  // по умолчанию стоит HIGH_PULL
  butt1.setType(HIGH_PULL);

  // NORM_OPEN - нормально-разомкнутая кнопка
  // NORM_CLOSE - нормально-замкнутая кнопка
  // по умолчанию стоит NORM_OPEN
  butt1.setDirection(NORM_OPEN);
//////////////////////////////////////////////////////////////////////////////////////

  // Set the display brightness (0-7):
  display.setBrightness(3);
  display.clear();
  display.setSegments(data + 2, 2, 2);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data + 2, 2, 1);
  delay(TEST_DELAY);

  display.clear();
  display.setSegments(data + 1, 3, 1);
  delay(TEST_DELAY);

  // All segments on
  display.setSegments(data);
  delay(TEST_DELAY);

  display.clear();
  display.setBrightness(0); 
  delay(1000); //даём паузу перед отображением времени


    display.setBrightness(1);
    // Clear the display:
    display.clear();
  
}
void loop()  //**********************************************************************************************************************
{  
  OS.tick();	// Функция работы с задачами. вызывать как можно чаще, задачи выполняются здесь.
  //---------------------------------------------------------------------
  // TIME();
  // periodtemp();
  // reletemp();
  // getSendData_DS18B20();
  // chek_light();

  butt1.tick();  // обязательная функция отработки нажатий кнопок(ки). Должна постоянно опрашиваться.

  //==================================================== Это можно удалить =======================================================================================================
  // if (butt1.isClick()) Serial.println("Click");         // проверка на один клик
  // if (butt1.isSingle()) Serial.println("Single");       // проверка на один клик
  // if (butt1.isDouble()) Serial.println("Double");       // проверка на двойной клик
  // if (butt1.isTriple()) Serial.println("Triple");       // проверка на тройной клик

  // if (butt1.hasClicks())                                // проверка на наличие нажатий
  //   Serial.println(butt1.getClicks());                  // получить (и вывести) число нажатий

  // // if (butt1.isPress()) Serial.println("Press");         // нажатие на кнопку (+ дебаунс)
  // // if (butt1.isRelease()) Serial.println("Release");     // отпускание кнопки (+ дебаунс)
  // if (butt1.isHold()) {									// если кнопка удерживается
  //   Serial.print("Holding ");							// выводим пока удерживается
    // Serial.println(butt1.getHoldClicks());				// можно вывести количество кликов перед удержанием!
  // }
  // if (butt1.isHold()) Serial.println("Holding");        // проверка на удержание
  // //if (butt1.state()) Serial.println("Hold");          // возвращает состояние кнопки

  // if (butt1.isStep()) {                                 // если кнопка была удержана (это для инкремента)
  //   value++;                                            // увеличивать/уменьшать переменную value с шагом и интервалом
  //   Serial.println(value);                              // для примера выведем в порт
  // }
  //============================================================================================================================================================================

//отработка долгого нажатия кнопки и вывод всех текущих настроек на дисплей  (период = каждый цикл)
    if (butt1.isHold()) 
    {
    display.setBrightness(3);
    sensor.requestTemp();
    Current_Temp = sensor.getTemp();
    display.showNumberDecEx(Current_Temp, false);                       //вывод текщей температуры
    Serial.println("PUSH BUTTON - SHOW CURRENT SETTINGS ON DISPLAY");
        delay (3000);
    display.showNumberDecEx(TIME_ON_morn, 0b11100000, true);            //вывод настройки времени утреннего включения света
        delay (2000);
    display.showNumberDecEx(TIME_OFF_morn, 0b11100000, true);           //вывод настройки времени утреннего вЫключения света
        delay (2000);
    display.showNumberDecEx(TIME_ON_evening, 0b11100000, true);         //вывод настройки времени вечернего включения света
        delay (2000);
    display.showNumberDecEx(TIME_OFF_evening, 0b11100000, true);        //вывод настройки времени вечернего вЫключения света
        delay (2000);
    display.showNumberDecEx(tempmax, false);                            //вывод настройки верхнего порога значения температуры (дуйчик вЫключить если температура больше)
        delay (2000);
    display.showNumberDecEx(tempmin, false);                            //вывод настройки нижнего порога значения температуры (дуйчик включить если температура ниже)
      delay (3000);
    }

    if (butt1.isClick())     //отработка однократного нажатия кнопки и вывод текущей температуры на дисплей  (период = каждый цикл)
    {
      display.setBrightness(3);
      sensor.requestTemp();
      Current_Temp = sensor.getTemp();
      display.showNumberDecEx(Current_Temp, false);
      Serial.println("Click");
      delay (3000);
    }
}
void TIME1()    // процедура получения текущего времени от DS3231 и вывод времени на дисплей БЕЗ ДВОЕТОЧИЯ (период = каждый цикл)
{
    display.setBrightness(1);
    DateTime now = rtc.now();
    int displaytime = (now.hour() * 100) + now.minute();
    display.showNumberDec(displaytime, true);    // Отображение текущего времени в 24-часовом формате с включенными ведущими нулями и центральным двоеточием.
    Serial.println(displaytime);
}
void TIME2()    // процедура получения текущего времени от DS3231 и вывод времени на дисплей С ДВОЕТОЧИЕМ
{
    display.setBrightness(1);
    DateTime now = rtc.now();
    int displaytime = (now.hour() * 100) + now.minute();
    display.showNumberDecEx(displaytime, 0b11100000, true);
}
//==============================================================================================================================================================================
void periodtemp()   // процедура вывода актуальной температуры на дисплей каждые 60 сек (период = 60сек)
{
        display.clear();
        display.setBrightness(3);
        sensor.requestTemp();
        delay(50);
        Current_Temp = sensor.getTemp();
        display.showNumberDecEx(Current_Temp, false);
        // Serial.println(displaytime);
        delay(5000);
}
//==============================================================================================================================================================================
void reletemp()  // проверка условий и выполнение включения/выключения обогрева (период = 70 сек)
{
        // *************** ОБОГРЕВ ***********************//
        if (Current_Temp >= tempmax) {
          digitalWrite(rele1, HIGH);        // HIGH - для реле с HIGH = ВЫКЛ 
          Serial.println("обогрев ВЫКЛЮЧЕН");
          digitalWrite(pin_LED2, LOW);      // светодиод индикации включения обогрева - ВыКЛЮЧИТЬ
          Serial.println(Current_Temp);
        }
        if (Current_Temp <= tempmin) {
          digitalWrite(rele1, LOW);        // LOW- для реле с LOW = ВКЛ 
          Serial.println("обогрев ВКЛЮЧЁН");
          digitalWrite(pin_LED2, HIGH);    // светодиод индикации включения обогрева - ВКЛЮЧИТЬ
          Serial.println(Current_Temp);
        }
}
//==============================================================================================================================================================================
void getSendData_DS18B20()  // процедура получения и обновления данных о температуре (период = 15 сек)
{
        // запрос температуры
        sensor.requestTemp();
        // приравниваем запрошенное значение температуры к глобальной переменной для отображения и манипуляций в других тасках
        Current_Temp = sensor.getTemp();
        if (sensor.readTemp()) Serial.println(sensor.getTemp());
        else Serial.println("error connecting DS18B20");
}
//==============================================================================================================================================================================
void chek_light()  // условия включения/выключения света (период = 30 сек)
{
        DateTime now = rtc.now();
        int displaytime = (now.hour() * 100) + now.minute();
        int s; // переменная плавного вкл/выкл света

      if (displaytime == TIME_ON_morn && current_state_light == false)  //условие ВКЛючения света УТРОМ (время = 6:00 и флаг "свет не горит")
        {
          display.setBrightness(5);
          Serial.println("Время ВКЛючени света");
          current_state_light = true;              // Флаг состояния света (ВКЛ или ВЫКЛ)
          for (int i = 0; i <= 255; i++)           //то плавно включаем свет
          {
            analogWrite(fadePin, i);
             
            s = i/2.55; //преобразование значения i в процентное соотношение
            
            display.showNumberDec(s, false);      // отображает на дисплее текущую ситуацию по изменению переменной i, т.е.на дисплее вместо времени по шкале 0->255 показывает степень зажигания света
            delay(timless_ON);                    //каждые timless_ON увеличение переменной i на 1 из 255(1мин = 60000мс, 1сек = 1000мс)
          }
        }

        if (displaytime == TIME_OFF_morn && current_state_light == true)  //условие ВЫКЛючения света УТРОМ (время = 9:00 и флаг "свет горит")
        {
          display.setBrightness(5);
          Serial.println("Время ВЫКЛючения света");
          current_state_light = false;              // Флаг состояния света (ВКЛ или ВЫКЛ)
          for (int i = 255; i >= 0; i--)            //плавно гасим его
          {
            analogWrite(fadePin, i);

              s = i/2.55;                           //преобразование значения i в процентное соотношение

            display.showNumberDec(s, false);        //отображает на дисплее текущую ситуацию по изменению переменной i, т.е.на дисплее вместо времени по шкале 255->0 показывает степень затухания света
            delay(timless_OFF);                     //каждые timless_OFF уменьшение переменной i на 1 из 255 (1мин = 60000мс, 1сек = 1000мс)
          }
        }

        if (displaytime == TIME_ON_evening && current_state_light == false)  //условие ВКЛючения света УТРОМ (время = 17:00 и флаг "свет не горит")
        {
          display.setBrightness(5);
          Serial.println("Время ВКЛючения света");
          current_state_light = true;              // Флаг состояния света (ВКЛ или ВЫКЛ)
          for (int i = 0; i <= 255; i++)           //то плавно включаем свет
          {
            analogWrite(fadePin, i);

            s = i/2.55;                            //преобразование значения i в процентное соотношение

            display.showNumberDec(s, false);       // отображает на дисплее текущую ситуацию по изменению переменной i, т.е.на дисплее вместо времени по шкале 0->255 показывает степень зажигания света
            delay(timless_ON);                     //каждые timless_ON увеличение переменной i на 1 из 255(1мин = 60000мс, 1сек = 1000мс)
          }
        }
        if (displaytime == TIME_OFF_evening && current_state_light == true)  //условие ВЫКЛючения света УТРОМ (время = 21:00 и флаг "свет горит")
        {
          display.setBrightness(5);
          Serial.println("Время ВЫКЛючения света");
          current_state_light = false;              // Флаг состояния света (ВКЛ или ВЫКЛ)
          for (int i = 255; i >= 0; i--)            //плавно гасим его
          {
            analogWrite(fadePin, i);

            s = i/2.55;                          //преобразование значения i в процентное соотношение

            display.showNumberDec(s, false);     //отображает на дисплее текущую ситуацию по изменению переменной i, т.е.на дисплее вместо времени по шкале 255->0 показывает степень затухания света
            delay(timless_OFF);                  //каждые timless_OFF уменьшение переменной i на 1 из 255 (1мин = 60000мс, 1сек = 1000мс)
          }
        }    

     //======= при пропуске времени включения света (при выключении платы) проверяется флаг должен ли в это время гореть свет =================================================================================

      if (displaytime > TIME_ON_morn && displaytime < TIME_OFF_morn && current_state_light == false || displaytime > TIME_ON_evening && displaytime < TIME_OFF_evening && current_state_light == false)  //условие ВКЛючения света в промежуток времени, когда свет должен гореть
        {
          display.setBrightness(5);
          Serial.println("ВКЛючене света пропущеного по времени");
          for (int i = 0; i <= 255; i++)  //то плавно включаем свет
          {
            analogWrite(fadePin, i);
             
            s = i/2.55; //преобразование значения i в процентное соотношение
            
            display.showNumberDec(s, false);  // отображает на дисплее текущую ситуацию по изменению переменной i, т.е.на дисплее вместо времени по шкале 0->255 показывает степень зажигания света
            delay(timless_ON);               //каждые timless_ON увеличение переменной i на 1 из 255(1мин = 60000мс, 1сек = 1000мс)
          }

          current_state_light = true;
        }

      //=====================================================================================================================


}