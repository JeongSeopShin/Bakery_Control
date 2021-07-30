// Load Wi-Fi library
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Relay.h>

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

int command = 0;  // 문자열 파싱 값

int Relay = 5;  // ESP8266 GPIO pin to use. Recommended: 5 (D1).

String myLocalIP;

// Web server port 번호를 80 번으로 한다.
WiFiServer server(80);

unsigned long currentTime = millis();   // 현재 시간을 mSec로 저장하는 변수
unsigned long previousTime = 0;         // 이전 시간을 mSec로 저장하는 변수
// Web request timeout 시간을 mSec 단위로 설정(Example: 2000ms = 2s)한다.
const long timeoutTime = 2000;

void setup() {
  logger->begin(115200);

  logger->println();
  logger->print("Connecting to ");
  logger->println(ssid);
  IPAddress ip(000,000,0,0); // 사용할 IP 주소 (환풍기)
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
  pinMode(Relay, OUTPUT);
}

void loop(){
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
        char c = client.read();       // Data를 읽어 c에 저장하고,
        logger->write(c);             // Logger(Serial monitor)에 출력 한다.
        message += c;
        if (c == '\n') {
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
            
            command = message.substring(index1+1,index2-1).toInt();
            message="";
            Serial.println(command);

            if ( command == 401 ) {
              digitalWrite(Relay, HIGH);
              logger->println("환풍기 On");
              }

            else if ( command == 402 ) {
              digitalWrite(Relay, LOW);
              logger->println("환풍기 Off");
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

        } else if (c != '\r') {
          // 입력된 문자가 '\r'(carriage return) 문자가 아닌 경우 currentLine의 끝에 문자를 첨부한다.
          currentLine += c;
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
