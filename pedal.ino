#include "pedal.h"

/*

  Поместите файл pedal.h в ту же папку что и ваш скетч
   не меняйте файл "pedal.h", а работайте с ним как с библиотекой

   Пользоваетльские функции

   CalibratePedal()      Калибровка с опциональной записью в EEPROM
   InitPedal()           Инициализация педали с явной задачей параметров калибровки
   InitPedalEEPROM()     Инициализация педали со считыаанием калибровки из EEPROM
   GetPedalCalibration() Получить калибровку из EEPROM
   SetPedalDisplayTime() Установить время возрата индикации.
   PedalStart()          Запустить  обработку педали
   PedalStop()           Остановить  обработку педали
   GetPedalDisplay()     замены пользательской информации
                         на данные с предали если они изменяются
                         Используется в дисплейной функции
   PedalAction()         Функция в которую надо поместить код
                         для обработки нового положения педали

    Подробное описание фунций

   Калибровка
    педаль должнв быть в исходном положении (не нажатом)
    на момент запуска калибровки
    за время калибровки педаль должна быть нажата до упора
    Если потенциометр подключен "наоборот" это автоматически учтется

    Запускать калибровку надо или до иницилизации сервиса педали или после его
    остановки

    Параметры
    pin -             аналоговый пин к которому подключен потенциометр
    calibrationTime   время на калибровку в msec
    addrEEPROM        арес в EEPROM ддя сохранения калибровки.
                      Если 0 или больше размера EEPROM, то ничпего не записывается
                      В последнем случае функция возвращает -1
    startPos          Возвращаемое значение начального положения педали (0-255)
                      эквивалентко analogRead / 4
    endPos            Возвращаемое значение конечного положения педали (0-255)
                      эквивалентко analogRead / 4

    Фунция возращает:
      1 если калибровка прошла успешно
      0 если педаль не была нажата
     -1 если задан неверный адрес EEPROM
  Данные занимают 3 байта в EEPROM

  int8_t CalibratePedal(uint8_t pin, uint16_t calibrationTime, int addrEEPROM, uint8_t &startPos, uint8_t &endPos )



   Инициализаци педали с "ручным" задание начального и конечного положения
   педали

    Параметры
    pin               аналоговый пин к которому подключен потенциометр
    startPos          Начальное положения педали (0-255)
                      эквивалентко analogRead / 4
    endPos            Конечное положения педали (0-255)
                      эквивалентко analogRead / 4
    steps             Количество шагов для выдаваемого значения
  void InitPedal(uint8_t pin, uint8_t startPos, uint8_t endPos, uint8_t steps)


    Считывает сохраненные даные калибровки
    Параметры:
    addrEEPROM        арес в EEPROM ддя считывания калибровки. Должен быть больше 0

    startPos          Возвращаемое значение начального положения педали (0-255)
                      эквивалентко analogRead / 4
    endPos            Возвращаемое значение конечного положения педали (0-255)
                      эквивалентко analogRead / 4

    Фунция возращает:
      1 если все хорошо
      0 если данные калибровки не верны
     -1 если задан неверный адрес EEPROM
  int8_t GetPedalCalibration(int addrEEPROM, uint8_t &startPos, uint8_t &endPos)


   Инициализаци педали с чтением калибровки из EEPROM
   педали

    Параметры
    pin               аналоговый пин к которому подключен потенциометр
    steps             Количество шагов для выдаваемого значения
    addrEEPROM        арес в EEPROM ддя считывания калибровки. Должен быть больше 0

  Фунция возращает:
      1 если все хорошо
      0 если данные калибровки не верны
     -1 если задан неверный адрес EEPROM
  int8_t InitPedalEEPROM(uint8_t pin, uint8_t steps, int addrEEPROM)


   Установка времени в течении которого происходит возврат
   с инидикации педали на обычную индикацию
   по умолчанию 2 секунды

   ПараметрЖ
   dispTime    Время в милисекундах

  void SetPedalDisplayTime(uint16_t dispTime)



   Запустить  обработку педали

  void PedalStart()

   остановить  обработку педали
  void PedalStop()


   Функция замены пользательской информации
   на данные с предали если они изменяются

   Параметр

   Val    пользовательское чисдо для индикаци

   Функция возвращает val если педаль неподвижна
   или положение педали если педаль двигается


  uint16_t GetPedalDisplay(uint16_t val)


  //Пример использования
  //Запускается на Нано/Уно подключенной к компу
  //и потенциометром или педалью поддключенной к аналоговому входу
  //вывод в сериал монитор на 115200
*/
#define POT_PIN A0   //можно просто 0
#define CALIB_TIME 5000 // 5 сек
#define EEPROM_ADDR 10  //1-0x1FF 
#define STEPS 31  //от 0 до 30

/*
   Эта функция должна быть определена в основном сектче
   и она вызыаается и выполняет действия когда значение педали
   поменялось
   Параметры
   val  - новое положение педали от 0 до steps-1
   steps задается при вызове InitPedal
*/
void PedalAction(uint8_t val)
{
  Serial.println((String)"         в синтезатор " + val);
  //Serial.write(0xC0); // посыл на выход
  //Serial.write(temp);
}


void setup() {
  int8_t ret;
  Serial.begin(115200);
  Serial.println("Setup");
  uint8_t pstart, pend;
  Serial.print("Try to init from EEPROM");
  ret = InitPedalEEPROM(POT_PIN, STEPS, EEPROM_ADDR);
  if (ret != 1)
  {
    Serial.println("..Failed!");
    Serial.print("Start Calibration..");
    ret = CalibratePedal(POT_PIN, CALIB_TIME, EEPROM_ADDR, pstart, pend);
    if (ret != 1) {
      Serial.print("Something wrong!  ret="); Serial.println(ret);
      Serial.println("Stop!");
      while (1);
      }
    Serial.println((String) "Start=" + pstart + "   End=" + pend );
    InitPedal(POT_PIN, pstart, pend, STEPS);   
  }
  ret =  GetPedalCalibration(EEPROM_ADDR, pstart, pend); //Чисто для проверки. А так не обязательно
  Serial.println((String) "EEPROM data Start=" + pstart + "   End=" + pend + "  ret=" +ret );
  PedalStart();
}

void TempR2(int16_t var)
{
  Serial.println(var);
 /* int num0, num1;
  num0 = var / 10;
  num1 = (var - (num0 * 10));
 
  tm1637.display(2, num0);
  tm1637.display(3, num1);
  */
}
uint16_t preset = 300;  
void loop() {
     
      TempR2(GetPedalDisplay(preset));  // фунция вывода на индикатор 
      ++preset;
      if (preset> 999) preset = 300;
      delay(500);  //из-за этой большой задержки на "дисплей" будет выводится с пропусками, 
                   //но "в синтезатор" должно все идти без пропусков.
                   //при работе с настояшим  индикатором надо просто убрать этот delay
      
  }
