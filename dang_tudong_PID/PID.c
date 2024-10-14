// Định nghĩa các chân kết nối
const int ENA_LEFT = 9;   // Chân PWM điều khiển tốc độ bánh trái
const int ENA_RIGHT = 10; // Chân PWM điều khiển tốc độ bánh phải
const int IN1_LEFT = 7;   // Chân điều khiển chiều quay bánh trái
const int IN2_LEFT = 8;
const int IN1_RIGHT = 5; // Chân điều khiển chiều quay bánh phải
const int IN2_RIGHT = 6;
const int encoderPinA = 2; // Chân encoder A
const int encoderPinB = 3; // Chân encoder B

// Biến PID
double Kp = 0, Ki = 0, Kd = 0;
double setpointLeft = 120;  // Tốc độ mục tiêu bánh trái (RPM)
double setpointRight = 120; // Tốc độ mục tiêu bánh phải (RPM)
double currentSpeed = 0;    // Tốc độ hiện tại của động cơ
double e = 0, last_e = 0, sum_e = 0;
double dt = 0.1; // Thời gian mẫu
double de = 0;   // Đạo hàm của sai số

// Biến encoder
volatile long encoderCount = 0;
double pulsesPerRevolution = 360.0; // Số xung encoder mỗi vòng quay

// Biến mạng RBF
const int NUM_NEURONS = 3;
double mu[NUM_NEURONS] = {0, 20, 100};    // Trung tâm neuron cho các giá trị sai số
double sigma[NUM_NEURONS] = {10, 15, 25}; // Độ rộng của hàm Gaussian
double weights[NUM_NEURONS][3];           // Trọng số cho Kp, Ki, Kd

// Biến vị trí của xe trên line (-200 đến 200)
int position = 0;

void setup()
{
    Serial.begin(9600);

    // Khởi tạo các chân động cơ và encoder
    pinMode(ENA_LEFT, OUTPUT);
    pinMode(IN1_LEFT, OUTPUT);
    pinMode(IN2_LEFT, OUTPUT);
    pinMode(ENA_RIGHT, OUTPUT);
    pinMode(IN1_RIGHT, OUTPUT);
    pinMode(IN2_RIGHT, OUTPUT);
    pinMode(encoderPinA, INPUT);
    pinMode(encoderPinB, INPUT);
    attachInterrupt(digitalPinToInterrupt(encoderPinA), encoderISR, RISING);

    // Khởi tạo trọng số cho mạng RBF
    initializeWeights();
}

void loop()
{
    // Đọc vị trí của xe trên line
    position = readPosition();

    // Tính toán tốc độ động cơ từ encoder
    currentSpeed = calculateSpeed();

    // Cập nhật tốc độ mục tiêu cho hai bánh xe dựa trên vị trí của xe
    double baseSpeed = 120;             // Tốc độ cơ bản
    double adjustment = position * 0.5; // Điều chỉnh tốc độ dựa trên vị trí

    setpointLeft = baseSpeed - adjustment;  // Bánh trái chậm lại nếu lệch phải
    setpointRight = baseSpeed + adjustment; // Bánh phải chậm lại nếu lệch trái

    // Giới hạn setpointLeft và setpointRight trong khoảng 0 đến 255
    setpointLeft = constrain(setpointLeft, 0, 255);
    setpointRight = constrain(setpointRight, 0, 255);

    // Tính sai số cho mỗi bánh xe
    double errorLeft = setpointLeft - currentSpeed;
    double errorRight = setpointRight - currentSpeed;

    // Tính toán đạo hàm sai số
    if (dt > 0)
    {
        de = (e - last_e) / dt;
    }
    else
    {
        de = 0;
    }

    // Tính toán tích phân và giới hạn tích phân
    double MAX_INTEGRAL = 1000;
    sum_e += e * dt;
    sum_e = constrain(sum_e, -MAX_INTEGRAL, MAX_INTEGRAL);

    // Tính toán tham số PID bằng mạng RBF cho mỗi bánh xe
    computePIDRBF(errorLeft); // Tính PID cho bánh trái
    double controlSignalLeft = Kp * errorLeft + Ki * sum_e + Kd * de;
    controlSignalLeft = constrain(controlSignalLeft, 0, 255); // Giới hạn PWM từ 0 đến 255

    computePIDRBF(errorRight); // Tính PID cho bánh phải
    double controlSignalRight = Kp * errorRight + Ki * sum_e + Kd * de;
    controlSignalRight = constrain(controlSignalRight, 0, 255); // Giới hạn PWM từ 0 đến 255

    // Điều khiển tốc độ động cơ
    analogWrite(ENA_LEFT, controlSignalLeft);   // Xuất tín hiệu PWM cho bánh trái
    analogWrite(ENA_RIGHT, controlSignalRight); // Xuất tín hiệu PWM cho bánh phải

    // Điều khiển chiều quay động cơ cho cả hai bánh
    if (controlSignalLeft >= 0)
    {
        digitalWrite(IN1_LEFT, HIGH);
        digitalWrite(IN2_LEFT, LOW);
    }
    else
    {
        digitalWrite(IN1_LEFT, LOW);
        digitalWrite(IN2_LEFT, HIGH);
    }

    if (controlSignalRight >= 0)
    {
        digitalWrite(IN1_RIGHT, HIGH);
        digitalWrite(IN2_RIGHT, LOW);
    }
    else
    {
        digitalWrite(IN1_RIGHT, LOW);
        digitalWrite(IN2_RIGHT, HIGH);
    }

    last_e = e;

    delay(dt * 1000); // Chu kỳ lặp
}

// Hàm ngắt encoder
void encoderISR()
{
    encoderCount++;
}

// Hàm tính tốc độ từ encoder
double calculateSpeed()
{
    static long lastEncoderCount = 0;
    static unsigned long lastTime = 0;

    // Tính toán thời gian và số xung đã quay được
    unsigned long currentTime = millis();
    long pulses = encoderCount - lastEncoderCount;

    // Tránh chia cho 0 khi tính tốc độ
    if (currentTime - lastTime > 0)
