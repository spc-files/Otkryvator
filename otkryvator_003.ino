
// плавная регулировка яркости индикации
// команда на привод только если он в противоположном положении
// частичное открывание (повторное нажатие при открывании в ручном режиме задает новый предел)
// индикация одним светодиодом в открытом режиме

#include <RCSwitch.h>
#include <Adafruit_NeoPixel.h>


/*

*/
#define motorPin1 5// пин мотора
#define motorPin2 4// пин мотора
#define buttonPin 3 // пин кнопки
#define ledPin 7 // пин светодиода
#define lightPin A0 // пин фоторезистора
#define openTime 600000 // время в открытом положении
#define closeTime 5400000 // время в закрытом положении
#define powerOnTime 15000 // длительность подачи питания на привод
#define remoteTime 5400000 // таймаут для перехода в автономный авторежим при потере связи
#define longPress 2000 // длинное нажатие
#define shortPress 500 // короткое нажатие
#define rainbowTime 10 // интервал обновления радуги
#define lightTime 1000 // интервал автояркости
#define adjustTime 10 // скорость изменения яркости
#define newCloseTime 4000 // коррекция для закрывания после частичного открывания
#define remoteTimeOut 3000 // таймаут блокировки между радиокомандами (чтобы не было повторного срабатывания при зажатии кнопки пульта)

#define pixels 4
// ОБЯЗАТЕЛЬНО задать актуальные коды
#define openCode 12345678 // код кнопки ручного дистанционного открывания, например 13157800
#define manOpen 12345678 // код ручного дистанционного открывания, например 560800
#define manClose 12345678 // код ручного дистанционного закрывания, например 561800
#define autoOpen 12345678 // код автоматического дистанционного открывания, например 562800
#define autoClose 12345678 // код автоматического дистанционного закрывания, например 563800

boolean remote = false; // флаг режима дистанционный/автономный
boolean manual = true; // флаг режима авто/ручной
boolean buttonLock = false; // флаг нажатия кнопки
boolean openState = false; // флаг состояния окна
boolean powerOnState = false; // флаг подачи питания на привод
boolean isShortPress = false; // флаг определения длинного/короткого нажатия
boolean makeBrighter = false; // флаг направления регулировки яркости
boolean adjustBrightness = false; // флаг необходимости регулировки яркости
boolean newOpen = false; // признак нового предела открывания
boolean remoteLock = false; // признак блокировки приема радиокоманды

unsigned long closeTimer = 0; // таймер закрытого окна
unsigned long openTimer = 0; // таймер открытого окна
unsigned long buttonTimer = 0; // таймер кнопки
unsigned long powerOnTimer = 0; // таймер мотора
unsigned long remoteTimer = 0; // таймер дистаницонного режима
unsigned long rainbowTimer = 0; // таймер обновления радуги
unsigned long remoteTimeOutTimer = 0; // таймер блокировки радиокоманды
unsigned long newOpenTime = 0; // память на новый предел открывания
unsigned long lightTimer = 0; // таймер обновления автояркости
unsigned long adjustDelay = 0; // таймер регулировки яркости
unsigned long brightness = 0; // сумма значений яркости
unsigned long newTime = 0; // длительность открытия/закрытия при частичном открытии
byte diffBrightness = 15; // разница значений яркости соседних отсчетов
byte lastBrightness = 0; // память последнено значения яркости
byte newBrightness = 0; // новое значение яркости

unsigned long rcbutton = 0; // код беспроводной кнопки

uint16_t i, j;

Adafruit_NeoPixel leds(pixels, ledPin, NEO_GRB + NEO_KHZ800);
RCSwitch openSwitch = RCSwitch();

void setup() {
  
  
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(lightPin, INPUT_PULLUP);

  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, LOW);


  leds.begin();
  leds.setBrightness(100);
  leds.show();  
  getLight();
  lastBrightness = brightness;
  newBrightness = lastBrightness;
  lightTimer = millis();
  visuals(); // установка цвета кнопки
  closeWindow();
  openSwitch.enableReceive(0);
 
}

void loop() {

 if (adjustBrightness == false) {

   if ((millis() - lightTimer) > lightTime) { // если прошло время до регулировки яркости
    getLight(); // получение яркости
  }

  autoBrightness();

 }

  if (adjustBrightness == true) {

   if (diffBrightness > 1) {
    if ((millis() - adjustDelay) > adjustTime) {
      
      diffBrightness = diffBrightness - 1;
     
      
      if (makeBrighter == false) {
         newBrightness = newBrightness - 1;
        
      }

      if (makeBrighter == true) {
         newBrightness = newBrightness + 1;
           
      }    

  

    visuals();
  
 
     adjustDelay = millis();
    }
  } else {
    adjustBrightness = false;
    lastBrightness = newBrightness;
  }
  
}

if (remoteLock == false) { // если нет блокировки приема команд
  if (openSwitch.available()) { // обработка радиокоманд
    rcbutton = openSwitch.getReceivedValue();

   if (rcbutton == openCode || rcbutton == manOpen || rcbutton == manClose || rcbutton == autoOpen || rcbutton == autoClose)  { // если принята команда
    remoteTimeOutTimer = millis(); // таймер блокировки приема команд
    remoteLock = true; // блокировка приема команд
     
    if (rcbutton == openCode) { // если нажата кнопка дистанционного ручного открывания
      processButton(true);
    }

    if (rcbutton == manOpen) { // если нажата кнопка дистанционного ручного открывания
      if (openState == false) {
        openWindow(); // открыть окно
      }
      manual = false; // если окно открыто вручную, переход в автономный режим (значит, стало душно или еще что)
      visuals(); // установка цвета кнопки
    }    

    if (rcbutton == manClose) { // если нажата кнопка дистанционного ручного открывания
      if (openState == true) {
        closeWindow(); // закрыть окно
      }
      manual = true; // если окно закрыто вручную, переход в ручной режим (значит, стало холодно или еще что)
      visuals(); // установка цвета кнопки
    }        

  if (manual == false) { // если в автоматическом режиме
    if ((rcbutton == autoOpen)||(rcbutton == autoClose)) { // если поступила дистанционная команда 
      remote = true; // флаг дистанционного режима
      visuals(); // установка цвета кнопки
      remoteTimer = millis(); // таймаут ожидания дистанционной команды

      if (rcbutton == autoOpen) {
        if (openState == false) {
          openWindow();
        }
      }

      if (rcbutton == autoClose) {
        if (openState == true) {
          closeWindow();
        }
      }
      
    }
  }
 }
}
}

if (remoteLock == true) {
  if ((millis() - remoteTimeOutTimer) > remoteTimeOut) {
    remoteLock = false;
    openSwitch.resetAvailable();
  }
}

  if ((millis() - remoteTimer) > remoteTime) { // если нет дистанционной команды за время remoteTime
    remote = false; // снятие флага дистанционного режима, переход в автономный режим
    visuals(); // установка цвета кнопки
  }

  if (powerOnState == true) { // если включено питание привода
        rainbow();

   if (newOpen == true) { // если частичное открытие активно
     
   if (openState == false) { // если окно закрывается
        newTime = newOpenTime + newCloseTime; // время закрытия больше на величину коррекции newCloseTime для надежного закрытия
   }



   if (openState == true) { // если окно открывается
    newTime = newOpenTime; // время закрытия = установленному, потому что контролировалось вручную
   }

 
   
    if ((millis() - powerOnTimer > newTime)) { // если прошло больше времени, чем новый предел открывания
      digitalWrite(motorPin1, LOW); // выключение привода
      digitalWrite(motorPin2, LOW);
      powerOnState = false;
      visuals(); // установка цвета кнопки
    }
   }

   if (newOpen == false) {    
    if ((millis() - powerOnTimer > powerOnTime)) { // если прошло больше времени, чем нужно на работу привода
      digitalWrite(motorPin1, LOW); // выключение привода
      digitalWrite(motorPin2, LOW);
      powerOnState = false;
      visuals(); // установка цвета кнопки
    }
  }
}

  if (digitalRead(buttonPin) == LOW && buttonLock == false) { // если кнопка нажата, но флаг кнопки "не нажата"
    buttonLock = true; // установка флага кнопки "нажата"
    buttonTimer = millis(); // запуск таймера для вычисления длительности нажатия
  }

  if (digitalRead(buttonPin) == LOW && buttonLock == true) { // если кнопка нажата и флаг кнопки "нажата"
    if ((millis() - buttonTimer) > longPress) { // если время нажатия кнопки больше длинного нажатия
      isShortPress = false;
      processButton(isShortPress);
    }
  }

  if (digitalRead(buttonPin) == HIGH && buttonLock == true) { // если кнопка не нажата, но флаг кнопки "нажата"
    buttonLock = false; // установка флага кнопки "не нажата"
     if ((millis() - buttonTimer) < shortPress) { // если время нажатия кнопки меньше длинного нажатия
      isShortPress = true;
      processButton(isShortPress);
    }
  }

  if (remote == false) { // если не в дистанционном режиме
  if (manual == false) { // если режим авто
    if (openState == false) {// если окно закрыто
      if ((millis() - closeTimer) > closeTime) { // если прошло больше, чем таймаут закрытого окна
        openWindow(); // открытие окна
      }
    }
    if (openState == true) {// если окно открыто
      if ((millis() - openTimer) > openTime) { // если прошло больше, чем таймаут открытого окна
        closeWindow(); // открытие окна
      }
    }
  }
 }

}

void closeWindow() {
  if (powerOnState == false) { // если мотор не работает
  openState = false; // установка флага закрытого окна
  closeTimer = millis(); // сброс таймера закрытого окна
  powerOnState = true; // установка флага питания привода
  powerOnTimer = millis(); // сброс таймера длительности питания привода
  digitalWrite(motorPin1, LOW);
  digitalWrite(motorPin2, HIGH);
  }
 
}

void openWindow() {
  if (powerOnState == false) { // если мотор не работает
  openState = true;
  openTimer = millis(); // сброс таймера открытого окна
  powerOnState = true; // установка флага питания привода
  powerOnTimer = millis(); // сброс таймера длительности питания привода  
  digitalWrite(motorPin1, HIGH);
  digitalWrite(motorPin2, LOW);
  }
 
}

void processButton(boolean pressType) {

  if (pressType == true) { // если короткое нажатие

    if (powerOnState == true && openState == true && manual == true) { // если включен мотор и если окно открывается и если в ручном режиме
      if (newOpen == false) { // если при открывании нажата кнопка и не включено частичное открывание, включение частичного открывания
        newOpen = true; // признак нового предела открывания
     
       newOpenTime = millis() - powerOnTimer; // время работы мотора для открытия на новый предел
    
        if (newOpenTime > powerOnTime) { // если почему-то новый предел открывания стал больше максимального времени мотора, сброс в начало
        newOpen = false;
      } 
     }
     visuals(); // индикация
    }
  
    if (openState == false) { // если флаг состояния окна "закрыто"
      openWindow(); // открыть окно
    } else {
      if (openState == true) { // если флаг состояния окна "открыто"
        closeWindow(); // закрыть окно
      }
    }
  }

  if (pressType == false) { // если длинное нажатие
    manual = !manual; // переключение флага авто/ручной
    visuals(); // установка цвета кнопки
    closeTimer = millis(); // сброc таймеров
    openTimer = millis();
    buttonTimer = millis() - shortPress; // установка минимального времени нажатия, чтобы не было срабатывания короткого после отпускания кнопки
  }

}

void visuals() { // установка цвета индикатора - логика
  if (powerOnState == false) { // если мотор не работает (если работает - радуга)
    if (remote == true) { // если дистанционный
      indicate(0, 0, newBrightness); // синий цвет авто
    }

    if (remote == false) { // если автономный
      if (manual == true) { // если ручной
        if (newOpen == false) {
          indicate(0, newBrightness, 0); // зеленый цвет ручной
        } else {
          indicate(newBrightness, 0, newBrightness); // цвет при установленном частичном открывании
        }
          
      }

      if (manual == false) { // если автономный
        indicate(newBrightness, 0, 0); // красный цвет автономный
      }
    }
  }

 }

void indicate(byte r, byte g, byte b) { // установка цвета индикатора - физика
   leds.clear();
   for(byte i=0; i<pixels-1; i++) { 
   leds.setPixelColor(i, leds.Color(r, g, b));
   leds.show(); 
  }
  
  if (openState == true) {
   leds.setPixelColor(pixels-1, leds.Color(newBrightness, newBrightness, newBrightness));
   leds.show();     
  } else {
   leds.setPixelColor(pixels-1, leds.Color(r, g, b));
   leds.show();    
  }

  
}

void rainbow() { // радуга

  if (j < 256) {
    if ((millis() - rainbowTimer) > rainbowTime) { 
      for(i=0; i<leds.numPixels(); i++) {
        leds.setPixelColor(i, Wheel((i+j) & 255));
      }
      rainbowTimer = millis();
      leds.show();
      j=j+1;
    }
    
  } else {
    j = 0;
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return leds.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return leds.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return leds.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}


void getLight() {

 

  for (byte j=0; j<10; j++) {
    brightness = brightness + analogRead(lightPin); // десять отсчетов яркости
  }
  
  brightness = brightness/10; // среднее арифметическое
  brightness = constrain(brightness, 25, 550); // определение границ
  brightness = map(brightness, 25, 550, 255, 10); // нормализация
  lightTimer = millis(); // таймер интервала измерения яркости
  
}


void autoBrightness() { // автояркость

  if (lastBrightness > brightness) { // вычисление изменения яркости с последнего отсчета
    diffBrightness = lastBrightness - brightness;
    makeBrighter = false;
  }

  if (lastBrightness < brightness) {
    diffBrightness = brightness - lastBrightness;
    makeBrighter = true;
  }  

 
  if ((diffBrightness > 25)) { // регулировка яркости только если яркость изменилась больше чем на 25 единиц
    adjustBrightness = true; // и если она не регулируется прямо сейчас
    newBrightness = lastBrightness;
    adjustDelay = millis();
  }
}
