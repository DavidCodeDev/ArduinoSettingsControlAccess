/* Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             49         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            48        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 */

#include <Key.h>
#include <Keypad.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>

EthernetUDP Udp;

#define RST_PIN 49  // Configurable, see typical pin layout above
#define SS_PIN 48   // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);

LiquidCrystal lcd(29, 31, 33, 35, 37, 39);

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 32, 30, 28, 26 };
byte colPins[COLS] = { 40, 38, 36, 34 };

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

const int rele1 = 22;
const int rele2 = 23;
const int rele3 = 24;
const int rele4 = 25;

bool rele1State = false;
bool rele2State = false;
bool rele3State = false;
bool rele4State = false;
bool displayControl = true;

static int scrollPosition = 0;

String userInput = "";
bool enteringCode = false;  // Variable para rastrear si se está ingresando un código
bool codeEntered = false;   // Variable para rastrear si se ha ingresado un código válido

String scrollingText = "Introduce codigo o pase tarjeta";

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  lcd.setCursor(0, 1);
  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  pinMode(rele3, OUTPUT);
  pinMode(rele4, OUTPUT);

  digitalWrite(rele1, HIGH);
  digitalWrite(rele2, HIGH);
  digitalWrite(rele3, HIGH);
  digitalWrite(rele4, HIGH);
}

void loop() {
  controlCard();
  releController();
  handleKeypadInput();
  handleScrollingText();
}

void handleKeypadInput(){
    if (codeEntered) { //codeEntered está iniciado en false
      lcd.setCursor(0, 1);
    }
    handleScrollingText();  // Maneja el desplazamiento del texto
    lcd.setCursor(0, 1);
    char key = keypad.getKey();  // Obtener la tecla dentro del bucle
    if (key != NO_KEY) {
      if (!enteringCode) {  // Si no se está ingresando un código, limpiar la línea
        lcd.setCursor(0, 1);
        lcd.print("                  ");
      }
      enteringCode = true;  // Se está ingresando un código

      if (key == 'A') {
        if (userInput.length() > 0) {
          userInput.remove(userInput.length() - 1);
          // Limpiar la visualización del último carácter en la pantalla LCD
          lcd.setCursor(userInput.length(), 1);
          lcd.print(" ");
           }
      } else if (key >= '0' && key <= '9') {
        if (!enteringCode) {
          enteringCode = true;
          lcd.clear();  // Limpiar la pantalla si estamos ingresando un código
          lcd.setCursor(0, 1);
        }
        if (userInput.length() < 5) {
          userInput += key;
          lcd.print(key);  // Muestra el número en la pantalla
        }
      } else if (key == '#') {
        if (userInput.length() == 5) {
          lcd.setCursor(0, 1);
          lcd.print("Codigo correcto");
          Serial.println(userInput);
          codeEntered = true;
          delay(2000);
          userInput = "";
        } else {
          lcd.setCursor(0, 1);
          lcd.print("Codigo invalido");
          delay(2000);
          lcd.clear();
          resetProgram();
          return;
        }
        if (enteringCode == true) {
          lcd.clear();
          lcd.print("Acceso permitido");
          lcd.setCursor(0, 1);
          lcd.print("Enjoy the match!");
          delay(3500);
          resetProgram();
        }
      }
      if (!codeEntered) {
        lcd.setCursor(0, 1);
        int numAsteriscos = userInput.length();
        for (int i = 1; i < numAsteriscos; i++) {
          lcd.print("*");
        }
        if (userInput.length() > 0) {
          lcd.print(userInput.substring(userInput.length() - 1));
        }
      }
    }
}

void controlCard(){
  static unsigned long lastCheck = 0;
  const long waitInterval = 3000;
  static bool clearDisplay = false;
     // Verifica si se ha detectado una tarjeta RFID
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    displayControl = false; 
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Tarjeta: ");
    String content = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    lcd.setCursor(0,1);
    lcd.print(content);
    Serial.println(content);
    delay(3000);
    lastCheck = millis();
    clearDisplay = true;
    mfrc522.PICC_HaltA();
  } if (millis() - lastCheck >= waitInterval && clearDisplay) {
    lcd.clear();
    clearDisplay = false;
    displayControl = true;
  }
}

void releController() {
  if (Serial.available() > 0){
    String strComando = Serial.readString();
    int comando = strComando.toInt();
      switch (comando){
        case 1: 
          if (digitalRead(rele1) == HIGH) {
            digitalWrite(rele1, LOW);
          } else {
            digitalWrite(rele1, HIGH);
          }
          break;
        case 2: 
          if (digitalRead(rele2) == HIGH) {
            digitalWrite(rele2, LOW);
          } else {
            digitalWrite(rele2, HIGH);
          }
          break;
        case 3: 
          if (digitalRead(rele3) == HIGH) {
            digitalWrite(rele3, LOW);
          } else {
            digitalWrite(rele3, HIGH);
          }
          break;
        case 4: 
          if (digitalRead(rele4) == HIGH) {
            digitalWrite(rele4, LOW);
          } else {
            digitalWrite(rele4, HIGH);
          }
          break;
          default: 
          lcd.setCursor(0,0);
      lcd.clear();
      lcd.setCursor(0,1);
      lcd.print(strComando);
      unsigned long startTime = millis();
      while (millis() - startTime < 2000) {
        // Mantiene la pantalla durante 5 segundos sin bloquear el programa
      }
      lcd.setCursor(0,1);
      lcd.clear();

      } 
}

}
void handleScrollingText() {
 if(displayControl) {
  static unsigned long lastScrollTime = 0;
  lcd.setCursor(0, 0);

  unsigned long currentTime = millis();
  if (currentTime - lastScrollTime >= 200) {  // Controla la velocidad de desplazamiento
    lcd.setCursor(0, 0);
    if (scrollPosition <= scrollingText.length() - 16) {
      lcd.print(scrollingText.substring(scrollPosition, scrollPosition + 16));  // Muestra el texto actual
    } else {
      int remainingChars = scrollingText.length() - scrollPosition;
      lcd.print(scrollingText.substring(scrollPosition));
      for (int i = 0; i < 16 - remainingChars; i++) {
        lcd.print(" ");  // Añadir espacios entre el final y el principio del texto
      }
      lcd.print(scrollingText.substring(0, 16 - remainingChars));
    }

    scrollPosition++;  // Desplaza una posición hacia la derecha

    if (scrollPosition >= scrollingText.length()) {
      // Si llega al final del texto, reinicia el desplazamiento
      scrollPosition = 0;
    }

    lastScrollTime = currentTime;
  }
  }
}

void resetProgram() {
  lcd.setCursor(0, 1);
  lcd.clear();
  scrollPosition = 0;
  lcd.setCursor(0, 0);
  lcd.clear();
  handleScrollingText();
  userInput = "";
  codeEntered = false;
  enteringCode = false;
  displayControl = true;
}
