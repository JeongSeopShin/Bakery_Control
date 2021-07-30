/*----------------------------------------------------------------------*
 * NodeMCU로 오픈 컨트롤 패널 키 원격 제거 프로그램
 * 2021.07.07. Kwanghyun Ro
 *----------------------------------------------------------------------*/
// Load Wi-Fi library
#include <Arduino.h>
#include <ESP8266WiFi.h>

// 본인이 사용하는 공유기의 ssid와 psaaword로 설정 하여야 한다.
#ifndef STA_SSID
#define STA_SSID      "공유기 ID"
#define STA_PASSWORD  "공유기 PASSWORD"
#endif

#define BAUD_SERIAL 115200
#define BAUD_LOGGER 115200
// Message logging Serial Port(UART0와 UART1 중에서 선택)을 설정 한다.
// UART0를 Microcontroller 와 Serial 통신에 사용하는 경우 UART1을 Logger로 사용 한다.
// ESP8266 NodeMCU 보드에는 TxD1(GPIO2) Pin 만 있기 때문에 UART1에는 출력만 가능 하다.
#define logger (&Serial)

const char* ssid     = STA_SSID;
const char* password = STA_PASSWORD;

String myLocalIP;

// Web server port 번호를 80 번으로 한다.
WiFiServer server(80);
 
const int interruptPin14 = 14;
const int interruptPin12 = 12;
const int interruptPin13 = 13;
const int interruptPin15 = 15;
int numberOfInterrupts14 = 0;
int numberOfInterrupts12 = 0;
int numberOfInterrupts13 = 0;
int numberOfInterrupts15 = 0;

//variable interruptCounter will be used in the interrupt service routine. Thus, it needs to be declared as "volatile"
volatile boolean interruptFlag14 = 0;
volatile boolean interruptFlag12 = 0;
volatile boolean interruptFlag13 = 0;
volatile boolean interruptFlag15 = 0;

int command = 0;
int command2 = 0;
int comm_count = 0;
#define BUTTON_UP   1
#define BUTTON_DOWN 2

unsigned long currentTime = millis();   // 현재 시간을 mSec로 저장하는 변수
unsigned long previousTime = 0;         // 이전 시간을 mSec로 저장하는 변수
// Web request timeout 시간을 mSec 단위로 설정(Example: 2000ms = 2s)한다.
const long timeoutTime = 2000;

ICACHE_RAM_ATTR void myInterruptServiceRoutine14() {
  interruptFlag14=1;
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
 // digitalWrite(D3, HIGH);
//  digitalWrite(D4, HIGH);
}
ICACHE_RAM_ATTR void myInterruptServiceRoutine12() {  // 출력 검정생, 입력 흰색 Up, 검은색 Down
  interruptFlag12=1;
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
//  digitalWrite(D3, HIGH);
//  digitalWrite(D4, HIGH);
  
  if (command == BUTTON_UP) {
     digitalWrite(5, LOW); 
     comm_count++;
     if(comm_count == 2) {
        command = 0;
        comm_count = 0;
     }
  } else if (command == BUTTON_DOWN) {
     digitalWrite(4, LOW);
     comm_count++;
     if(comm_count == 2) {
        command = 0;
        comm_count = 0;
     }
  }
}

ICACHE_RAM_ATTR void myInterruptServiceRoutine13() {
  interruptFlag13=1;
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
 // digitalWrite(D3, HIGH);
//  digitalWrite(D4, HIGH);
}

ICACHE_RAM_ATTR void myInterruptServiceRoutine15() {
  interruptFlag15=1;
  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
//  digitalWrite(D3, HIGH);
//  digitalWrite(D4, HIGH);
}

void setup() {
  logger->begin(115200);

  logger->println();
  logger->print("Connecting to ");
  logger->println(ssid);

  IPAddress ip(000,000,0,0); // 사용할 IP 주소 (오븐)  
  IPAddress gateway(000,000,0,0); // 게이트웨이 주소
  IPAddress subnet(000,000,0,0); // 서브넷 주소
  WiFi.config(ip, gateway, subnet);
  // SSID 와 password를 사용하여 Wi-Fi network에 연결 한다.
  WiFi.begin(ssid, password);
  // WIFi 연결을 기다린다. 이 동안 Logger에 0.5초 간격으로 . 을 출력 한다.
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logger->print(".");
  }
  // Local IP address를 출력한다. 이 IP address를 이용하여 Web browser에서 Server에 연결한다.
  logger->println("");
  logger->println("WiFi connected.");
  myLocalIP = WiFi.localIP().toString();
  logger->println("");
  logger->println("IP: " + myLocalIP);
  logger->println(WiFi.localIP());
  
  // Web server를 시작 한다.
  server.begin();
  
  pinMode(5, OUTPUT);
  pinMode(4, OUTPUT);
//  pinMode(D3, OUTPUT);
//  pinMode(D4, OUTPUT);

  digitalWrite(5, HIGH);
  digitalWrite(4, HIGH);
//  digitalWrite(D3, HIGH);
//  digitalWrite(D4, HIGH);
          
//sets interruptPin (pin 13) as input with pull-up
  pinMode(interruptPin14, INPUT_PULLUP);
  pinMode(interruptPin12, INPUT_PULLUP);
  pinMode(interruptPin13, INPUT_PULLUP);
  pinMode(interruptPin15, INPUT_PULLUP);
//sets interruptPin (pin13) as interrupt that is triggered on a falling edge. 
//when interrupt is triggered function "myInterruptServiceRoutine" is called.
  attachInterrupt(digitalPinToInterrupt(interruptPin14), myInterruptServiceRoutine14,FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin12), myInterruptServiceRoutine12,FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin13), myInterruptServiceRoutine13,FALLING);
  attachInterrupt(digitalPinToInterrupt(interruptPin15), myInterruptServiceRoutine15,FALLING);
}

void loop() {
  // 서버에 연결되어 있고 읽을 수있는 데이터가 있는 클라이언트가 있는 경우 Non zero(Client Pointer)가 Return 된다.
  WiFiClient client = server.available();
  if (client) {
    // 만약 새로운 Client 가 열결되었으면 "New Client." 메세지를 출력한다.
    logger->println("New Client.");
    // Client로 부터 전송되는 String의 현재 Line을 저장하는 변수
    // 이 변수에 저장된 문자 수가 0인 상태에서 '\n' 코드가 전송되는 경우 Client HTTP request의 종료를 표시 한다.
    String currentLine = "";
    String message = "";
    currentTime = millis();
    previousTime = currentTime;
    // Client 가 연결되어 있고 while loop 내의 처리 시간이 timeoutTime 시간 보다 작은 경우 while loop를 계속 한다.
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();         // 현재 시간을 Update 한다.        
      if (client.available()) {       // 만약 Client로 부터 읽을 Data(Byte) 가 있으면,
        char comm_data = client.read();       // Data를 읽어 c에 저장하고,
        logger->write(comm_data);             // Logger(Serial monitor)에 출력 한다.
        message += comm_data;
        if (comm_data == '\n') {
          // currentLine에 입력된 문자 수 가 0인 상태에서 '\n' 코드가 입력되면(두 개의 '\n' Code 가 입력 되면),
          // client HTTP request의 끝을 표시하기 때문에 Response을 Send 한다.
          if (currentLine.length() == 0) {
            // HTTP Header는 항상 Response code(예: HTTP/1.1 200 OK)와 
            // Client 가 무슨 문서가 올 것 인지 알 수 있게 content-type을 보내고 Blank line을 보낸다.
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");  // Response 완료되면 후 연결이 Close 된다.
            client.println();

            // 문자열 파싱
            int index1, index2;
            
            index1 = message.indexOf('=');
            index2 = message.indexOf('H', index1+1);
            
            command2 = message.substring(index1+1,index2-1).toInt();
            message="";
            Serial.println(command2);

            if ( command2 == 1 ) {
              command = BUTTON_UP;
              logger->println("TOP 온도 UP");
            }

            else if ( command2 == 2 ) {
              command = BUTTON_DOWN;
              logger->println("TOP 온도 DOWN");
            }

            else if ( command2 == 3 ) {
              command = BUTTON_UP;
              logger->println("BOTTOM 온도 UP");
            }

            else if ( command2 == 4 ) {
              command = BUTTON_DOWN;
              logger->println("BOTTOM 온도 DOWN");
            }

            else if ( command2 == 5 ) {
              command = BUTTON_UP;
              logger->println("TIMER UP");
            }

            else if ( command2 == 6 ) {
              command = BUTTON_DOWN;
              logger->println("TIMER DOWN");  
            }

            else if ( command2 == 7 ) {
              command = BUTTON_UP;
              logger->println("TIMER START");  
            }

            else if ( command2 == 8 ) {
              command = BUTTON_DOWN;
              logger->println("TIMER STOP");  
            }

            else if ( command2 == 9 ) {
              command = BUTTON_UP;
              logger->println("STEAM ON");  
            }

            else if ( command2 == 10 ) {
              command = BUTTON_DOWN;
              logger->println("STEAM OFF");  
            }

            else if ( command2 == 11 ) {
              command = BUTTON_UP;
              logger->println("LIGHT ON");  
            }

            else if ( command2 == 12 ) {
              command = BUTTON_DOWN;
              logger->println("LIGHT OFF");  
            }

            else if ( command2 == 13 ) {
              command = BUTTON_UP;
              logger->println("OVEN ON");  
            }

            else if ( command2 == 14 ) {
              command = BUTTON_DOWN;
              logger->println("OVEN OFF");  
            }
  
          if(interruptFlag14==1){
            interruptFlag14=0;
            numberOfInterrupts14++;
              if (numberOfInterrupts14 % 10 == 0) {
                Serial.print("An interrupt14 has occurred. Total: ");
                Serial.println(numberOfInterrupts14);
             }
          }

          if(interruptFlag12==1){
            interruptFlag12=0;
            numberOfInterrupts12++;
              if (numberOfInterrupts12 % 10 == 0) {
                Serial.print("An interrupt12 has occurred. Total: ");
                Serial.println(numberOfInterrupts12);
              }
          }

          if(interruptFlag13==1){
            interruptFlag13=0;
            numberOfInterrupts13++;
              if (numberOfInterrupts13 % 10 == 0) {
                Serial.print("An interrupt13 has occurred. Total: ");
                Serial.println(numberOfInterrupts13);
              }
          }

          if(interruptFlag15==1){
            interruptFlag15=0;
            numberOfInterrupts15++;
              if (numberOfInterrupts15 % 10 == 0) {
                Serial.print("An interrupt15 has occurred. Total: ");
                Serial.println(numberOfInterrupts15);
              }
          }
            // 버퍼에 남아있는 데이터 삭제
            client.flush();
            // 새로운 Blank line을 전송하여 HTTP response의 종료를 표시한다.
            client.println();
            // while loop를 Break out 한다.
         } else {
          // '/n' (Newline)문자를 받은 경우 currentLine을 Clear(문자 수가 0 이 됨) 한다. client HTTP request의 끝을 판단하는데 이용.
            currentLine = "";
           }

        } else if (comm_data != '\r') {
          // 입력된 문자가 '\r'(carriage return) 문자가 아닌 경우 currentLine의 끝에 문자를 첨부한다.
          currentLine += comm_data;
        }
      }
    }
    // Header variable을 Clear 한다. 
    message = "";
    // Close the connection
    client.stop();
    logger->println("Client disconnected.");
    logger->println("");
  }
}
