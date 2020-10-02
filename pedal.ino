#include <TM1637Display.h>
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
#define BAT_PIN A7   //пин измерения батареи
#define CALIB_TIME 5000 // времф калибровки 5 сек
#define CALIB_PIN  16
#define EEPROM_ADDR 10  //1-0x1FF 
#define STEPS 31  //от 0 до 30
#define BUT1_PIN 7  // кнопка 1

//Pins for 7segment display
#define CLK 4
#define DIO 3
TM1637Display display(CLK, DIO);
const uint8_t SEG_LO_b[] = {
  SEG_F | SEG_E | SEG_D ,                          // L
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  0,                                               //
  SEG_F |  SEG_E | SEG_D | SEG_C | SEG_G           // b
};

const uint8_t SEG_CALI[] = {
  SEG_G,                                           // -
  SEG_A | SEG_D | SEG_E | SEG_F        ,           // C
  SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G,   // A
  SEG_F | SEG_E | SEG_D,                           // L
  SEG_E | SEG_F                                    // I
};
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
#define BAT_NOM 90 //Номинальное напряжене батарии *10
#define BAT_DIV 49 //напряжени на делители при BAT_NOM
#define BAT_LOW 70 //минимально допустимое Vbatt*10   

//Возвращает напляжение на батарее *10 в виде целого
//числа.
uint8_t GetBatteryV(uint8_t pin)
{
  uint32_t sum = 0;
  analogRead(pin);
  for (int i = 0; i < 100; i++) sum += analogRead(pin);
  sum = sum * BAT_NOM * 5 / BAT_DIV / 10240;
  return (uint8_t) sum;
}

void setup() {
  int8_t ret;

  Serial.begin(115200);
  Serial.println("Setup");
  display.setBrightness(0x0f);
  pinMode(CALIB_PIN, INPUT_PULLUP);
  /*
    int8_t Vbat = GetBatteryV(BAT_PIN);
    Serial.println((String)"Vbat=" + Vbat);  Vbat = 80;
    if (Vbat < BAT_LOW)
    { //Батарея разряжена. выводим сообщение и зависаем
    while (1) {
      display.setSegments(SEG_LO_b); delay(700);
      display.clear(); delay(200);
    }
    }
  */
  uint8_t pstart, pend;
  /*
  if (digitalRead(CALIB_PIN) == 0)
  { //была нажата кнопка в момент включени. Делаем калибровку
    display.setSegments(&SEG_CALI[1]);
    Serial.print("waiting for rlease//");
    while (!digitalRead(CALIB_PIN)) //Ждем пока отпустят
    { //и моргаем надписью
      if ((millis() & 0x3FF) == 500) display.clear();
      if ((millis() & 0x3FF) == 1000) display.setSegments(&SEG_CALI[1]);
    }
    display.setSegments(&SEG_CALI[1]);
    Serial.println("waiting for rleased");
    Serial.print("Start Calibration..");
    ret = CalibratePedal(POT_PIN, CALIB_TIME, EEPROM_ADDR, pstart, pend);
  }
  else
  {
    Serial.print("Trying to init from EEPROM");
    ret = InitPedalEEPROM(POT_PIN, STEPS, EEPROM_ADDR);
  }
  if (ret != 1) */
  { //не удалось получить калибровку
    //показываем сообщение и используем дефолт
    Serial.print("..Failed! Use default");
    display.setSegments(SEG_CALI);
    delay(3000);
    pstart = 0;
    pend = 255;
    InitPedal(POT_PIN, pstart, pend, STEPS);
  }
  Serial.println("");
 // Serial.println((String) "ret " + ret +   "Start=" + pstart + "   End=" + pend  );
   PedalStart();



}

void TempR2(int16_t var)
{
  // Serial.println(var);
  display.showNumberDec(var, false);
}
uint16_t preset = 300;
void loop() {

  TempR2(GetPedalDisplay(preset));  // фунция вывода на индикатор
  ++preset;
  if (preset > 999) preset = 300;
  // delay(500);  //из-за этой большой задержки на "дисплей" будет выводится с пропусками,
  //но "в синтезатор" должно все идти без пропусков.
  //при работе с настояшим  индикатором надо просто убрать этот delay

}
