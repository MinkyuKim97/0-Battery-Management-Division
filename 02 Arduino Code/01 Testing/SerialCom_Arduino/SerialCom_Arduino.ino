#include <Servo.h>
Servo myservo;
int pos = 0; 
bool dir = true;

// String line;
// #include <SoftwareSerial.h>
// SoftwareSerial LinkSerial(10, 11); // RX=D10, TX=D11(안써도 됨)


void doAction(){
    //     if(dir && pos>=180){
    //     dir =!dir;
    //   }else if(!dir && pos <=0){
    //     dir= !dir;
    // }
    //   if(dir){
    //     pos+=30;
    //   }else{
    //     pos-=30;
    //   }
    //   myservo.write(pos);
    if(dir){
      myservo.write(180);
      
    }else{
      myservo.write(00);
    }
    dir = !dir;
}
// void setup() {
//   pinMode(LED_BUILTIN, OUTPUT);
//   myservo.attach(9);
//   // LinkSerial.begin(115200);
//   LinkSerial.begin(9600);
//   // myservo.write(0);
// }

// void loop() {
//   while (LinkSerial.available()) {
//     char c = (char)LinkSerial.read();
//     if (c == '\r') continue;

//     if (c == '\n') {
//       line.trim();
//       if (line.startsWith("@CMD:")) {
//         String cmd = line.substring(5);
//         cmd.trim();
//         if (cmd == "DO_ACTION") doAction();
//       }
//       line = "";
//     } else {
//       if (line.length() < 80) line += c;
//       else line = "";
//     }
//   }
// }



// UNO (Hardware Serial) - @CMD: prefix만 처리


String line;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);  // ESP32와 반드시 동일 baud!

  myservo.attach(9);
  myservo.write(0);
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      line.trim();

      // ✅ 접두어 검사
      if (line.startsWith("@CMD:")) {
        String cmd = line.substring(5); // "@CMD:" 이후
        cmd.trim();

        if (cmd == "DO_ACTION") {
          doAction();
        }
        // 필요하면 else-if로 더 추가:
        // else if (cmd == "NEXT") ...
      }

      line = "";
    } else {
      if (line.length() < 80) line += c;  // 안전 제한
      else line = "";
    }
  }
}
