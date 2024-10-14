#include <PS2X_lib.h> //for v1.6
#include <Stepper.h>
#include <SoftPWM.h>
#include <Servo.h>

int servoPin = 14;
/////////////////////////////////
// dong co ban
int tocdoban = 50;
int pwm_ban = A12;
bool ban_da_bat = false;
bool pittong_down = true;
bool pittong_up = true;
bool bit_led = true;
bool dongco = false;
bool bit_ban = true;
/////////////////////////////////
#define PS2_DAT A3 // 14
#define PS2_CMD A1 // 15
#define PS2_SEL A2 // 16
#define PS2_CLK A0 // 17

/******************************************************************
 * select modes of PS2 controller:
 *   - pressures = analog reading of push-butttons
 *   - rumble    = motor rumbling
 * uncomment 1 of the lines for each mode selection
 ******************************************************************/
// #define pressures   true
#define pressures false
// #define rumble      true
#define rumble false

PS2X ps2x; // create PS2 Controller Class
///////////////////////// ///////////////////
Servo servo;

///////////////////////// ///////////////////
int error = 0;
byte type = 0;
byte vibrate = 0;

////////////////////////////////////////////
int led[2] = {A13, A14};
int t2[3] = {A5, 7, 6};
int t1[3] = {A7, 3, 2};
int p1[3] = {A6, 5, 4};
int p2[3] = {A4, 9, 8};
int pittong[2] = {A11, A10};
int stepper[6] = {10, 11, 12, 13, A8, A9}; // 22 s24 en a enb
bool motorRunning = false;

const int stepsPerRevolution = 5; // Số bước mỗi vòng của động cơ bước
int fixedSpeed = 100;
Stepper myStepper(stepsPerRevolution, stepper[0], stepper[1], stepper[2], stepper[3]);
///////////////////////////////////////////
void pittong_len(int pittong[])
{
  digitalWrite(pittong[0], HIGH);
  digitalWrite(pittong[1], LOW);
}
void pittong_xuong(int pittong[])
{
  digitalWrite(pittong[0], LOW);
  digitalWrite(pittong[1], HIGH);
}
void pittong_stop()
{
  digitalWrite(pittong[0], LOW);
  digitalWrite(pittong[1], LOW);
}
/// //////////////////////////////////////////
void led_on()
{
  digitalWrite(led[0], HIGH);
  digitalWrite(led[1], LOW);
}
void led_off()
{
  digitalWrite(led[0], LOW);
  digitalWrite(led[1], HIGH);
}
///  //////////////////////////////////////

void thuan(int banh[], int speed)
{
  digitalWrite(banh[0], HIGH);
  if (banh[1] == 9) //| banh[1] == 5
    SoftPWMSetPercent(banh[1], map(speed, 0, 255, 0, 100));
  else
    analogWrite(banh[1], speed);
  // analogWrite(banh[1], speed);
  digitalWrite(banh[2], 0);
}
void nguoc(int banh[], int speed)
{
  digitalWrite(banh[0], HIGH);
  if (banh[1] == 9)
    SoftPWMSetPercent(banh[1], 0);
  else
    analogWrite(banh[1], 0);
  // digitalWrite(banh[1], 0);
  analogWrite(banh[2], speed);
}

void stop(int banh[])
{
  digitalWrite(banh[0], LOW);
  analogWrite(banh[1], 0);
  analogWrite(banh[2], 0);
}
void tien(int speed)
{
  thuan(t1, speed);
  thuan(t2, speed);
  thuan(p1, speed);
  thuan(p2, speed);
}
void lui(int speed)
{
  nguoc(t1, speed);
  nguoc(t2, speed);
  nguoc(p1, speed);
  nguoc(p2, speed);
}
void sangphai(int speed)
{
  thuan(t1, speed);
  nguoc(t2, speed);
  nguoc(p1, speed);
  thuan(p2, speed);
}
void sangtrai(int speed)
{
  nguoc(t1, speed);
  thuan(t2, speed);
  thuan(p1, speed);
  nguoc(p2, speed);
}
void xoayphai(int speed)
{
  thuan(t1, speed);
  thuan(t2, speed);
  nguoc(p1, speed);
  nguoc(p2, speed);
}
void xoaytrai(int speed)
{
  nguoc(t1, speed);
  nguoc(t2, speed);
  thuan(p1, speed);
  thuan(p2, speed);
}
void stop_all()
{
  stop(t1);
  stop(t2);
  stop(p1);
  stop(p2);
}
//////////////////////////////////////
void rotateServo(bool forward)
{
  Serial.println(forward ? "Đang quay servo tiến..." : "Đang quay servo lùi...");

  if (forward)
  {
    for (int pos = 0; pos <= 180; pos += 1)
    {
      servo.write(pos); // Điều khiển servo đến vị trí pos
      delay(10);        // Chờ servo di chuyển đến vị trí
    }
    Serial.println("Servo đã quay xong 180 độ và dừng lại.");
  }
  else
  {
    for (int pos = 180; pos >= 0; pos -= 1)
    {
      servo.write(pos); // Điều khiển servo đến vị trí pos
      delay(10);        // Chờ servo di chuyển đến vị trí
    }
    Serial.println("Servo đã quay ngược về 0 độ và dừng lại.");
  }
}
////////////////////////////////
void setup()
{
  //////////////////////////////
  SoftPWMBegin();
  //////////////////////////////

  for (int i = 0; i < 3; i++)
  {
    pinMode(t1[i], OUTPUT);
    pinMode(t2[i], OUTPUT);
    pinMode(p1[i], OUTPUT);
    pinMode(p2[i], OUTPUT);
  }
  // led
  pinMode(led[0], OUTPUT);
  pinMode(led[1], OUTPUT);
  // pittong
  pinMode(pittong[0], OUTPUT);
  pinMode(pittong[1], OUTPUT);
  // stepper
  pinMode(stepper[4], OUTPUT);
  pinMode(stepper[5], OUTPUT);
  digitalWrite(stepper[4], LOW);
  digitalWrite(stepper[5], LOW);
  myStepper.setSpeed(200);
  servo.attach(servoPin);
  servo.write(0);
  Serial.begin(2000000);

  delay(300); // added delay to give wireless ps2 module some time to startup, before configuring it

  // CHANGES for v1.6 HERE!!! **************PAY ATTENTION*************
  // setup pins and settings: GamePad(clock, command, attention, data, Pressures?, Rumble?) check for error
  error = ps2x.config_gamepad(PS2_CLK, PS2_CMD, PS2_SEL, PS2_DAT, pressures, rumble);

  if (error == 0)
  {
    Serial.print("Found Controller, configured successful ");
    Serial.print("pressures = ");
    if (pressures)
      Serial.println("true ");
    else
      Serial.println("false");
    Serial.print("rumble = ");
    if (rumble)
      Serial.println("true)");
    else
      Serial.println("false");
  }
  else if (error == 1)
    Serial.println("No controller found, check wiring");

  else if (error == 2)
    Serial.println("Controller found but not accepting commands");

  else if (error == 3)
    Serial.println("Controller refusing to enter Pressures mode, may not support it. ");

  //  Serial.print(ps2x.Analog(1), HEX);

  type = ps2x.readType();
  switch (type)
  {
  case 0:
    Serial.print("Unknown Controller type found ");
    break;
  case 1:
    Serial.print("DualShock Controller found ");
    break;
  case 2:
    Serial.print("GuitarHero Controller found ");
    break;
  case 3:
    Serial.print("Wireless Sony DualShock Controller found ");
    break;
  }
}

void loop()
{
  int speed;
  ps2x.read_gamepad(false, vibrate); // read controller and set large motor to spin at 'vibrate' speed
  vibrate = ps2x.Analog(PSAB_CROSS); // this will set the large motor vibrate speed based on how hard you press the blue (X) button

  ////////////////////////////////////////////
  int LY = ps2x.Analog(PSS_LY);
  int LX = ps2x.Analog(PSS_LX);
  int RY = ps2x.Analog(PSS_RY);
  int RX = ps2x.Analog(PSS_RX);
    Serial.println(LY);
    Serial.println(RY);
    Serial.println(LX);
    Serial.println(RY);
  if((LY==123)&&(RY==123)&&(LX==123)&&(RX==123))
  stop_all();
  //// RY SERVO

  if (error == 1) // skip loop if no controller found
    return;
  //////////////////////////////////////////////
  /*Joystick left*/
  if (ps2x.Button(PSB_R1) && !dongco) // Bật động cơ
  {
    dongco = true;
    delay(300);
    Serial.println("Bật động cơ");
  }
  else if (ps2x.Button(PSB_R1) && dongco) // Tắt động cơ
  {
    dongco = false;
    delay(300);
    Serial.println("Dừng động cơ");
    stop_all(); // Thêm dấu ngoặc để gọi hàm
  }
  if ((LY > 125) && dongco && (LX > 123 - 10) && (LX < 123 + 10))
  {
    Serial.println("Lùi");
    speed = map(LY, 124, 255, 0, 255);
    lui(speed);
    Serial.println(speed);
    // lui(200);
  }

  if ((LY < 120) && dongco && (LX > 123 - 10) && (LX < 123 + 10))
  {
    Serial.println("Tien");
    speed = map(LY, 122, 0, 0, 255);
    tien(speed);
    Serial.println(speed);
  }
  if ((LX > 125) && dongco && (LY > 123 - 10) && (LY < 123 + 10))
  {
    Serial.println("Sang phai");
    speed = map(LX, 124, 255, 0, 255);
    sangphai(speed);
    Serial.println(speed);
  }
  if ((LX < 120) && dongco && (LY > 123 - 10) && (LY < 123 + 10))
  {
    Serial.println("Sang trai");
    speed = map(LX, 122, 0, 0, 255);
    sangtrai(speed);
    Serial.println(speed);
  }

  //////////////////////////////////////////////
  /*Joystick right*/
  // if ((RY > 123) && dongco && (RX > 123 - 10) && (RX < 123 + 10))
  // {
  //   // Serial.println("Xuống");
  //
  // }
  // else
  // {
  //
  // }

  // if ((RY < 123) && dongco && (RX > 123 - 10) && (RX < 123 + 10))
  // {
  //   // Serial.println("Lên");
  //   //
  // }
  // else
  // {
  //
  // }
  if ((RX > 125) && dongco && (RY > 123 - 10) && (RY < 123 + 10))
  {
    Serial.println("R2 Xoay phải");
    xoayphai(100);
  }
  if ((RX < 120) && dongco && (RY > 123 - 10) && (RY < 123 + 10))
  {
    Serial.println("R2 Xoay trái");
    xoaytrai(100);
  }

  //////////////////////////////////////////////
  /*Button START and SELECT*/
  if (ps2x.Button(PSB_START)) // will be TRUE as long as button is pressed
    Serial.println("Start is being held");

  if (ps2x.Button(PSB_SELECT))
    Serial.println("Select is being held");
  //////////////////////////////////////////////
  /*Button PAD*/
  if (ps2x.Button(PSB_PAD_UP)) // will be TRUE as long as button is pressed
  {
    if (pittong_up)
    {
      pittong_up = false;
      pittong_len(pittong);
      Serial.println("Nút UP được nhấn - pittong lên");
      delay(100);
    }
    else
    {
      pittong_up = true;
      pittong_down = true;
      pittong_stop();
      Serial.println("Nút UP được nhấn - pittong dừng");
      delay(100);
    }
  }

  if (ps2x.Button(PSB_PAD_LEFT))
  {
    digitalWrite(stepper[4], HIGH);
    digitalWrite(stepper[5], HIGH);
    Serial.println("Quay động cơ bước sang trái");
    myStepper.step(-stepsPerRevolution);
  }
  else
  {
    digitalWrite(stepper[4], LOW);
    digitalWrite(stepper[5], LOW);
  }

  if (ps2x.Button(PSB_PAD_RIGHT))
  {
    digitalWrite(stepper[4], HIGH);
    digitalWrite(stepper[5], HIGH);
    Serial.println("Quay động cơ bước sang phải");
    myStepper.step(stepsPerRevolution);
  }
  else
  {
    digitalWrite(stepper[4], LOW);
    digitalWrite(stepper[5], LOW);
  }
  if (ps2x.Button(PSB_PAD_DOWN))
  {
    if (pittong_down)
    {
      pittong_down = false;
      pittong_xuong(pittong);
      Serial.println("Nút DOWN được nhấn - pittong Xuống");
      delay(100);
    }
    else
    {
      pittong_down = true;
      pittong_up = true;

      pittong_stop();
      Serial.println("Nút DOWN được nhấn - pittong dừng");
      delay(100);
    }
  }
  // else
  // {
  //   pittong_stop();
  // }
  //////////////////////////////////////////////
  /*Button L3, R3, L2, R2*/
  if (ps2x.NewButtonState()) // will be TRUE if any button changes state (on to off, or off to on)
  {
    if (ps2x.Button(PSB_L3))
      Serial.println("L3 pressed");
    if (ps2x.Button(PSB_R3))
      Serial.println("R3 pressed");
    if (ps2x.Button(PSB_L2))
      Serial.println("L2 pressed");
    if (ps2x.Button(PSB_R2))
      Serial.println("R2 pressed");
  }
  int servoPosition;
  // = map(RY, 0, 255, 0, 180); // DualShock Controller
  // servo.write(servoPosition);
  // Serial.println("vitri servo: ");
  // Serial.println(servoPosition);
  if (ps2x.Button(PSB_R2))
  {
    if (bit_ban)
    {
      Serial.println("ban ");
      servoPosition = 120;
      servo.write(servoPosition);
      Serial.println(servoPosition);
      delay(300);
      servoPosition = 45;
      servo.write(servoPosition);
      Serial.println("vitri servo: ");
      Serial.println(servoPosition);
      delay(200);
      bit_ban = false;
    }
    else
    {
      // servoPosition = 85;
      // servo.write(servoPosition);
      // Serial.println("vitri servo: ");
      // delay(200);
      // servoPosition = 110;
      // servo.write(servoPosition);
      // Serial.println(servoPosition);
      // delay(200);
      bit_ban = true;
    }
  }
  else
  {
    servoPosition = 85;
    servo.write(servoPosition);
  }
  if (ps2x.Button(PSB_L2))
  {
    // Serial.println("R2 Xoay trái");
    // xoaytrai(100);
  }
  if (ps2x.Button(PSB_L1))
  {
    // Serial.println("L1 pressed-Dừng động cơ");
    // stop_all();
  }
  //////////////////////////////////////////////
  /*Button GREEN, RED, BLUE, PINK*/
  if (ps2x.Button(PSB_GREEN) && ban_da_bat)

  {
    Serial.println("GREEN pressed-Tăng tốc độ bắn ");
    SoftPWMSetPercent(pwm_ban, tocdoban + 10);
  }

  if (ps2x.ButtonPressed(PSB_RED)) // will be TRUE if button was JUST pressed
  {
    if (bit_led)
    {
      bit_led = false;
      Serial.println("led on");
      led_on();
    }
    else
    {
      bit_led = true;
      Serial.println("led off");
      led_off();
    }
  }

  if (ps2x.NewButtonState(PSB_BLUE) && ban_da_bat) // will be TRUE if button was JUST pressed OR released
  {
    Serial.println("Nút BLUE được nhấn- giảm tốc độ bắn ");
    SoftPWMSetPercent(pwm_ban, tocdoban - 10);
  }

  //////////////////////////////////////////////
  if (ps2x.ButtonPressed(PSB_PINK) && !ban_da_bat)
  {
    Serial.println("Nút PINK được nhấn-Đã bật động cơ bắn");
    tocdoban = 70;
    SoftPWMSetPercent(pwm_ban, tocdoban);
    ban_da_bat = true;
  }
  else if (ps2x.ButtonPressed(PSB_PINK) && ban_da_bat)
  {
    Serial.println("Nút PINK được nhấn-Đã tắt động cơ bắn");
    SoftPWMSetPercent(pwm_ban, 0);
    ban_da_bat = false;
  }

  delay(50);
}
