const int channel = 5; // Số kênh cảm biến
int adc[channel];      // Mảng lưu trữ giá trị ADC
int tb = 100;          // Ngưỡng

// Hàm đọc giá trị ADC từ các cảm biến
void read_adc_all()
{
  for (int i = 0; i < channel; i++)
  {
    unsigned int temp = analogRead(A0 + i); // Đọc giá trị từ chân A0 đến A4
    
    Serial.print("adc:");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(temp);
    if (temp < tb)
    {
      adc[i] = 0; // Đặt giá trị thành 0 nếu nhỏ hơn ngưỡng
    }
    else
    {
      adc[i] = temp; // Lưu giá trị ADC
    }
  }
}

// Hàm tính toán vị trí của xe
int vi_tri()
{
  unsigned int sum1 = 0, sum2 = 0;
  float temp;

  read_adc_all(); // Gọi hàm đọc ADC

  for (int j = 0; j < channel; j++)
  {
    sum1 += adc[j] * (j + 1); // Tính tổng trọng số
    sum2 += adc[j];           // Tính tổng giá trị
  }

  if (sum2 != 0)
  {
    temp = (float)sum1 * 100; // Tính tạm thời
    temp = temp / sum2;       // Tính vị trí
    return temp - 350;        // Trả về vị trí đã điều chỉnh
  }
  else
  {
    return 0; // Trả về 0 nếu không có giá trị hợp lệ
  }
}

void setup()
{
  Serial.begin(57600); // Khởi động Serial
}

void loop()
{
  int position = vi_tri();  // Gọi hàm vi_tri để lấy vị trí
  Serial.println(position); // In vị trí ra Serial Monitor
  delay(1000);              // Đợi 1 giây trước khi lặp lại
}