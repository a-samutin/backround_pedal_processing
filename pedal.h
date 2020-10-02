#ifndef _PEDAL_H_
#define _PEDAL_H_
#include <EEPROM.h>

#define NOISE_TR  60
#define SIGNATURE 0xA5
#define PEDAL_DISPLAY_TIME 2000 //сколько времени показывать данные от педали (msec)
volatile uint8_t pedalJustStarted;
volatile uint8_t pedalAverADC; //текущее усредненное значение с АЦП (0-255)
volatile uint8_t pedalCurrentPos; //последнее запомненное положение (0-255)
volatile uint8_t pedalVal;  //mapped значение
volatile uint8_t pedalStep;
volatile uint8_t pedalSteps;
volatile uint16_t pedalActive;
volatile uint8_t pedalStart; //начальное положение педали (0-255)
volatile uint8_t pedalStop; //конечное положене          (0-255)
volatile uint8_t pMin, pMax;
volatile uint16_t pedalDisplayTime = PEDAL_DISPLAY_TIME;

extern void PedalAction(uint8_t);
//#define DEBUG     //отладочный режим если раскомментировать
#define BUF_SZ 8

/*
   Эта функция должна быть определена в основном сектче
   и она вызыаается и выполняет действия когда значение педали
   поменялось
   Параметры
   val  - новое положение педали от 0 до steps-1
   steps задается при вызове InitPedal
   /
  void PedalAction(uint8_t val)
  {
  } */


//функция вычисления скользящего среднего
#define BUF_MASK  (BUF_SZ - 1)
uint8_t GetNextAvrg(uint8_t in)
{
  static uint8_t buf[BUF_SZ];
  static uint8_t head = 0;
  static uint8_t tail = 0;
  static uint8_t num  = 0;
  static uint16_t sum = 0;
//  uint32_t ret;
  sum = sum + in;
  if (++num > BUF_SZ)
  {
    num = BUF_SZ;
    sum = sum - buf[tail];
    tail++;
    tail &=  BUF_MASK;
  }
  buf[head] = in;
  ++head;
  head &= BUF_MASK;
  return uint8_t (sum / num);
}

/* Калибровка
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
*/
int8_t CalibratePedal(uint8_t pin, uint16_t calibrationTime, uint16_t addrEEPROM, uint8_t &startPos, uint8_t &endPos )
{
  uint8_t pmax = 0;
  uint8_t pmin = 255;
  uint8_t aStart;
  if (addrEEPROM >= EEPROM.length()) return -1;

  uint32_t st = millis();
  //Работа с педалью не должна быть запущена в момент калибровки
  //но на всякий случай запрещаем прерывания по таймеру
  TIMSK0 &= ~_BV(OCIE0A);  //Disable processing interrupts
  delay(1);
  for (uint8_t i = 0; i< BUF_SZ >> 2 ; i++)
    aStart = GetNextAvrg(analogRead(pin) >> 2);
#ifdef DEBUG
  Serial.println((String)"Initial pos " + aStart);
#endif

  while ((millis() - st) < calibrationTime)
  {
    uint8_t val = GetNextAvrg(analogRead(pin) >> 2);
    if (val > pmax) pmax = val;
    if (val < pmin) pmin = val;
  }
  startPos = pmin;
  endPos = pmax;
  if (abs(pmin - pmax) < 50)    return 0; //Неудачная попытка

  if (abs(aStart - pmin) > abs(aStart - pmax)) {
    startPos = pmax;
    endPos = pmin;
  }

  if (addrEEPROM > 0)
  {        
    EEPROM.update(addrEEPROM, SIGNATURE);
    ++addrEEPROM;
    EEPROM.update(addrEEPROM, startPos);
    ++addrEEPROM;
    EEPROM.update(addrEEPROM, endPos);
  }
  return 1;
}


/*
   Инициализаци педали с "ручным" задание начального и конечного положения
   педали

    Параметры
    pin               аналоговый пин к которому подключен потенциометр
    startPos          Начальное положения педали (0-255)
                      эквивалентко analogRead / 4
    endPos            Конечное положения педали (0-255)
                      эквивалентко analogRead / 4
    steps             Количество шагов для выдаваемого значения

*/
void InitPedal(uint8_t pin, uint8_t startPos, uint8_t endPos, uint8_t steps)
{
  pedalSteps = steps;
  pedalStep = abs(endPos - startPos) / steps;
  pedalStart = startPos;
  pedalStop  = endPos;
  pMin = min(startPos, endPos);
  pMax = max(startPos, endPos);
  pedalCurrentPos = startPos;
#ifdef DEBUG
  Serial.println((String)"start " + startPos + "  end " + endPos + +"  steps " + steps +  "  Step = " + pedalStep);
#endif
  //
  cli();
  // Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion
  if (pin >= 14) pin -= 14; // allow for channel or pin numbers
  pin &= 0x7;
  cli();
  ADMUX = pin  | _BV(ADLAR) | _BV(REFS0); // устанавливаем канал, используем 8 бит, ref=Vcc
  ADCSRA = _BV(ADEN) | // ADC enable
           _BV(ADSC) | // ADC start
           _BV(ADATE) | // Auto trigger
           //       _BV(ADIE) | // Interrupt enable
           _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
  ADCSRB = 0;                // Free run mode, no high MUX bit
  DIDR0 = 1 << pin; // Turn off digital input for ADC pin
  sei();
  //Настраиваем таймер0 на прерывание по совпадению
  //Раз в 1 msec
  OCR0A = 0xAF;
  sei(); // Enable interrupts
}

/*
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
     -2 отсутсвует подпись
*/
int8_t GetPedalCalibration(uint16_t addrEEPROM, uint8_t &startPos, uint8_t &endPos)
{
  if (addrEEPROM == 0 || addrEEPROM >= EEPROM.length()) return -1;
  if (EEPROM.read(addrEEPROM) != SIGNATURE) return -2;
  ++addrEEPROM;
  startPos = EEPROM.read(addrEEPROM);
  ++addrEEPROM;
  endPos = EEPROM.read(addrEEPROM);
  if (abs(startPos - endPos) < 40)    return 0; //Неудачная попытка калибровки
  return 1;
}
/*
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

*/
int8_t InitPedalEEPROM(uint8_t pin, uint8_t steps, uint16_t addrEEPROM)
{

  uint8_t startPos, endPos;
  int8_t ret = GetPedalCalibration(addrEEPROM, startPos, endPos);
  if (ret !=1 ) return ret; //Что-то пошло не так
  InitPedal(pin,startPos, endPos, steps);
  return 1;
}

/*
 * Установка времени в течении которого происходит возврат 
 * с инидикации педали на обычную индикацию
 * по умолчанию 2 секунды
 * 
 * ПараметрЖ
 * dispTime    Время в милисекундах
 */
void SetPedalDisplayTime(uint16_t dispTime)
{
  pedalDisplayTime = dispTime;
}

/*
 * Запустить  обработку педали
 * 
*/
void PedalStart()
{
  pedalJustStarted = BUF_SZ;
  ADCSRA |= _BV(ADIE);    // Enable sampling interrupt
  TIMSK0 |= _BV(OCIE0A);  // Enable processing interrupts
}

/*
 * остановить  обработку педали
 * 
*/
void PedalStop()
{
  pedalJustStarted = 0;
  TIMSK0 &= ~_BV(OCIE0A);  //Вisable processing interrupts
  ADCSRA &= ~_BV(ADIE);    // Disable sampling interrupt
}

/*
 * Функция замены пользательской информации 
 * на данные с предали если они изменяются
 * 
 * Параметр
 * 
 * Val    пользовательское чисдо для индикаци
 * 
 * Функция возвращает val если педаль неподвижна
 * или положение педали если педаль двигается
 * 
 */
uint16_t GetPedalDisplay(int16_t val)
{
  if (pedalActive)
    return pedalVal;
  return  val;
}

//Прерывания таймера 1 раз в  msec
ISR (TIMER0_COMPA_vect)
{
  uint8_t val;
  if (pedalActive)
  {
    --pedalActive;
  }
  if (pedalJustStarted) {
    pedalCurrentPos = pedalAverADC;
    return; //данные еще не готовы
  }
  if (abs(pedalCurrentPos - pedalAverADC) >= pedalStep)
  { //поехали
    pedalActive = pedalDisplayTime ;
    pedalCurrentPos = pedalAverADC;

    val = map (pedalCurrentPos, pedalStart, pedalStop, 0, pedalSteps);
    if (val != pedalVal) {
      pedalVal = val;
      PedalAction(pedalVal);
    }
  }
}

ISR(ADC_vect) { // ADC sampling interrupt
  uint8_t  sample = ADCH; // 0-255
  uint8_t  noiseCnt = 0;
  if (pedalJustStarted) {
    --pedalJustStarted;
  }
  else  {
    if (abs(sample - pedalAverADC) >= NOISE_TR) {
      ++noiseCnt;
      if (noiseCnt < BUF_SZ)
        return; //игнорируем шум
    }
  }
  if (sample > pMax) sample = pMax;
  if (sample < pMin) sample = pMin;
  noiseCnt = 0;
  pedalAverADC = GetNextAvrg(sample);
}

#endif
