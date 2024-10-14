#include <EEPROM.h>
#include <Kalman.h>
//hello
// Khởi tạo Kalman filter với 5 kênh
KalmanFilter kalman(5);

// Khai báo biến toàn cục
int t1[3] = {40, 7, 6};
int p2[3] = {44, 2, 3};
int t2[3] = {42, 5, 4};
int p1[3] = {38, 9, 8};
int bt[6] = {26, 28, 30, 32, 34, 36};
int cb_truoc[5] = {A0, A1, A2, A3, A4};
int cb_trai[5] = {A9, A7, A8, A5, A6};
int cb_phai[5] = {A10, A12, A14, A13, A11};
bool bit_phim = true;
int set_up = 0;

// Điều khiển các bánh
void thuan(int banh[], int speed)
{
  digitalWrite(banh[0], HIGH);
  analogWrite(banh[1], speed);
  analogWrite(banh[2], 0);
}

void nguoc(int banh[], int speed)
{
  digitalWrite(banh[0], HIGH);
  analogWrite(banh[1], speed);
  analogWrite(banh[2], 0);
}

void stop(int banh[])
{
  digitalWrite(banh[0], LOW);
  analogWrite(banh[1], 0);
  analogWrite(banh[2], 0);
}

void stop_all()
{
  stop(t1);
  stop(t2);
  stop(p1);
  stop(p2);
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

// Đọc mẫu từ các cảm biến
int *get_sample(int cb[])
{
  int *buffer_adc = new int[5];
  for (int i = 0; i < 5; i++)
  {
    buffer_adc[i] = analogRead(cb[i]);
    delay(5);
  }
  return buffer_adc;
}

// Sử dụng Kalman filter để lọc giá trị
int *kalman_values(int cb[])
{
  int *estimate = new int[5];
  for (int i = 0; i < 5; i++)
  {
    int buffer_adc = analogRead(cb[i]);
    estimate[i] = kalman.updateEstimate(buffer_adc, i);
  }
  return estimate;
}

// Lưu giá trị nền
int *Save_Template_background_value(int cb[])
{
  int *buffer_adc_background = new int[5];
  int *buffer_adc = get_sample(cb);

  for (int j = 0; j < 5; j++)
  {
    buffer_adc_background[j] = buffer_adc[j];
    Serial.print("Buffer_adc_background ");
    Serial.print(j);
    Serial.print(": ");
    Serial.println(buffer_adc_background[j]);
  }
  delete[] buffer_adc;
  return buffer_adc_background;
}

// Lưu giá trị line
int *Save_Template_line_value(int cb[])
{
  int *buffer_adc_line = new int[5];
  int *buffer_adc = get_sample(cb);

  for (int j = 0; j < 5; j++)
  {
    buffer_adc_line[j] = buffer_adc[j];
    Serial.print("Buffer_adc_line ");
    Serial.print(j);
    Serial.print(": ");
    Serial.println(buffer_adc_line[j]);
  }
  delete[] buffer_adc;
  return buffer_adc_line;
}

// Tính toán giá trị ngưỡng
int *threshold_value_calculation(int buffer_adc_background[], int buffer_adc_line[])
{
  int *threshold_value = new int[5];

  for (int j = 0; j < 5; j++)
  {
    threshold_value[j] = (buffer_adc_background[j] + buffer_adc_line[j]) / 2;
    Serial.print("threshold_value ");
    Serial.print(j);
    Serial.print(": ");
    Serial.println(threshold_value[j]);
  }

  return threshold_value;
}

// Xử lý dữ liệu ADC dựa trên ngưỡng
int *ADC_Data_Processing(int cb[], int threshold_value[])
{
  Serial.println("ADC_Data_Processing");

  // Cấp phát động bộ nhớ cho mảng lưu trữ kết quả
  int *buffer_adc_bits = new int[5];

  // Lấy mẫu giá trị ADC
  int *buffer_sample = get_sample(cb);

  // So sánh với giá trị ngưỡng và gán giá trị cho buffer_adc_bits
  for (int i = 0; i < 5; i++)
  {
    if (buffer_sample[i] > threshold_value[i])
    {
      buffer_adc_bits[i] = 0; // Nếu giá trị ADC vượt ngưỡng, gán giá trị 0
    }
    else
    {
      buffer_adc_bits[i] = 1; // Nếu giá trị ADC không vượt ngưỡng, gán giá trị 1
    }

    // In giá trị từng bit sau khi xử lý
    Serial.print("bit ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(buffer_adc_bits[i]);
  }

  // Giải phóng bộ nhớ của buffer_sample
  delete[] buffer_sample;

  // Trả về con trỏ đến mảng đã xử lý
  return buffer_adc_bits;
}

// Tính vị trí dựa trên dữ liệu ADC
int vi_tri(int buffer_adc_bits[])
{
  int vitri = 0;
  int sum = 0;
  int sum_chanel = 0;

  for (int i = 0; i < 5; i++)
  {
    if (buffer_adc_bits[i] == 1)
    {
      sum += i;
      sum_chanel++;
    }
  }
  vitri = sum * 100 / sum_chanel - 200;
  return vitri;
}

// Lưu dữ liệu vào EEPROM
void save_buffer(int *RX_Data, int DATA_START_ADDRESS)
{
  for (int i = 0; i < 5; i++)
  {
    EEPROM.write(DATA_START_ADDRESS + (i * 4), RX_Data[i] & 0xFF);
    EEPROM.write(DATA_START_ADDRESS + (i * 4) + 1, (RX_Data[i] >> 8) & 0xFF);
    EEPROM.write(DATA_START_ADDRESS + (i * 4) + 2, (RX_Data[i] >> 16) & 0xFF);
    EEPROM.write(DATA_START_ADDRESS + (i * 4) + 3, (RX_Data[i] >> 24) & 0xFF);
  }
}

// Đọc dữ liệu từ EEPROM
void read_buffer(int *buffer_data, int DATA_START_ADDRESS)
{
  for (int i = 0; i < 5; i++)
  {
    buffer_data[i] = EEPROM.read(DATA_START_ADDRESS + (i * 4));
    buffer_data[i] |= EEPROM.read(DATA_START_ADDRESS + (i * 4) + 1) << 8;
    buffer_data[i] |= EEPROM.read(DATA_START_ADDRESS + (i * 4) + 2) << 16;
    buffer_data[i] |= EEPROM.read(DATA_START_ADDRESS + (i * 4) + 3) << 24;
  }
}

////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(9600);

  for (int i = 0; i < 3; i++)
  {
    pinMode(t1[i], OUTPUT);
    pinMode(t2[i], OUTPUT);
    pinMode(p1[i], OUTPUT);
    pinMode(p2[i], OUTPUT);
  }

  for (int i = 0; i < 5; i++)
  {
    kalman.MultiKalmanFilter(1.0, 1.0, 0.1, i);
  }

  for (int i = 0; i < 6; i++)
  {
    pinMode(bt[i], INPUT_PULLUP);
  }
}

void loop()
{
  int *back_ground_values_buffer;
  int *line_values_buffer;
  int *threshold_value_buffer;
  int back_ground_values_buffer_truoc[5];
  int back_ground_values_buffer_phai[5];
  int back_ground_values_buffer_trai[5];
  int line_values_buffer_truoc[5];
  int line_values_buffer_phai[5];
  int line_values_buffer_trai[5];
  int threshold_values_buffer_truoc[5];
  int threshold_values_buffer_phai[5];
  int threshold_values_buffer_trai[5];
  if ((digitalRead(bt[0]) == 0) & bit_phim)
  {
    bit_phim = false;
    switch (set_up)
    {
    case 0:
      Serial.println("line truoc");
      back_ground_values_buffer = Save_Template_background_value(cb_truoc);
      for (int i = 0; i < 5; i++)
      {
        back_ground_values_buffer_truoc[i] = back_ground_values_buffer[i];
      }
      delete[] back_ground_values_buffer;
      break;
    case 1:
      Serial.println("line phai");
      back_ground_values_buffer = Save_Template_background_value(cb_phai);
      for (int i = 0; i < 5; i++)
      {
        back_ground_values_buffer_phai[i] = back_ground_values_buffer[i];
      }
      delete[] back_ground_values_buffer;
      break;
    case 2:
      Serial.println("line trai");
      back_ground_values_buffer = Save_Template_background_value(cb_trai);
      for (int i = 0; i < 5; i++)
      {
        back_ground_values_buffer_trai[i] = back_ground_values_buffer[i];
      }
      delete[] back_ground_values_buffer;
      break;
    }
  }
  else
  {
    bit_phim = true;
    delay(100);
  }

  if ((digitalRead(bt[1]) == 0) & bit_phim)
  {
    bit_phim = false;
    switch (set_up)
    {
    case 0:
      Serial.println("line truoc");
      line_values_buffer = Save_Template_line_value(cb_truoc);
      for (int i = 0; i < 5; i++)
      {
        line_values_buffer_truoc[i] = line_values_buffer[i];
      }
      delete[] line_values_buffer;
      break;
    case 1:
      Serial.println("line phai");
      line_values_buffer = Save_Template_line_value(cb_phai);
      for (int i = 0; i < 5; i++)
      {
        line_values_buffer_phai[i] = line_values_buffer[i];
      }
      delete[] line_values_buffer;
      break;
    case 2:
      Serial.println("line trai");
      line_values_buffer = Save_Template_line_value(cb_trai);
      for (int i = 0; i < 5; i++)
      {
        line_values_buffer_trai[i] = line_values_buffer[i];
      }
      delete[] line_values_buffer;
      break;
    }
  }
  else
  {
    bit_phim = true;
    delay(100);
  }

  if ((digitalRead(bt[2]) == 0) & bit_phim)
  {
    bit_phim = false;
    switch (set_up)
    {
    case 0:
      Serial.println("line truoc");
      threshold_value_buffer = threshold_value_calculation(back_ground_values_buffer_truoc, line_values_buffer_truoc);
      for (int i = 0; i < 5; i++)
      {
        threshold_values_buffer_truoc[i] = threshold_value_buffer[i];
      }
      delete[] threshold_value_buffer;
      break;
    case 1:
      Serial.println("line phai");
      threshold_value_buffer = threshold_value_calculation(back_ground_values_buffer_phai, line_values_buffer_phai);
      for (int i = 0; i < 5; i++)
      {
        threshold_values_buffer_phai[i] = threshold_value_buffer[i];
      }
      delete[] threshold_value_buffer;
      break;
    case 2:
      Serial.println("line trai");
      threshold_value_buffer = threshold_value_calculation(back_ground_values_buffer_trai, line_values_buffer_trai);
      for (int i = 0; i < 5; i++)
      {
        threshold_values_buffer_trai[i] = threshold_value_buffer[i];
      }
      delete[] threshold_value_buffer;
      break;
    }
  }
  else
  {
    bit_phim = true;
    delay(100);
  }
  if ((digitalRead(bt[3]) == 0) & bit_phim)
  {
    bit_phim = false;
    set_up++;
    Serial.print("Setup: ");
    switch (set_up)
    {
    case 0:
      /* code */
      Serial.println("Line truoc");
      break;
    case 1:
      /* code */
      Serial.println("Line phai");
      break;
    case 2:
      /* code */
      Serial.println("Line trai");
      break;

    default:
      set_up = 0;
      break;
    }
  }
  else
  {
    bit_phim = true;
    delay(100);
  }
  if ((digitalRead(bt[4]) == 0) & bit_phim)
  {
    bit_phim = false;
    int vt[5];
    Serial.println("ok");
    int *adc_bits = ADC_Data_Processing(cb_truoc, threshold_values_buffer_truoc); // ADC Data Processing
    for (int i = 0; i < 5; i++)
    {
      vt[i] = adc_bits[i];
    }
    int vtri = vi_tri(vt);
    Serial.println("vitri:");
    Serial.println(vtri);
  }
  else
  {
    bit_phim = true;
    delay(100);
  }
}
