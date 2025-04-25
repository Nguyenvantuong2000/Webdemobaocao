#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include <WiFi.h>

// Thông tin kết nối WiFi
#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""

// Thông tin Firebase
#define FIREBASE_HOST "https://smart-25b7d-default-rtdb.firebaseio.com/"
#define FIREBASE_AUTH "Uffn2iuXmJPbF0O1KIS1OXWr9EswvYPhqLalf1Zs"

// Cấu hình cảm biến DHT
#define DHTTYPE DHT22
#define DHTPIN_1 14
#define DHTPIN_2 15
#define DHTPIN_3 16
#define DHTPIN_4 17

// Cấu hình cảm biến MQ2
#define MQ2PIN_1 32
#define MQ2PIN_2 33
#define MQ2PIN_3 34
#define MQ2PIN_4 35
#define R0 10.0  // Điện trở MQ2 trong môi trường sạch, cần điều chỉnh tùy hệ thống


// Cấu hình LED
#define LED1 0
#define LED2 2
#define LED3 4

DHT dht1(DHTPIN_1, DHTTYPE);
DHT dht2(DHTPIN_2, DHTTYPE);
DHT dht3(DHTPIN_3, DHTTYPE);
DHT dht4(DHTPIN_4, DHTTYPE);

FirebaseData fbdo;  
FirebaseAuth auth;
FirebaseConfig config;

void setup() {
    Serial.begin(115200);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    Serial.print("Đang kết nối WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("\nĐã kết nối WiFi!");
    Serial.println(WiFi.localIP());

    // Khởi động các cảm biến DHT
    dht1.begin();
    dht2.begin();
    dht3.begin();
    dht4.begin();

    // Cấu hình Firebase
    config.host = FIREBASE_HOST;
    config.signer.tokens.legacy_token = FIREBASE_AUTH;
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);

    // Cấu hình chân LED
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);

    // Cấu hình chân cảm biến MQ2
    pinMode(MQ2PIN_1, INPUT);
    pinMode(MQ2PIN_2, INPUT);
    pinMode(MQ2PIN_3, INPUT);
    pinMode(MQ2PIN_4, INPUT);
}

bool checkFirebaseWrite(String path, float value) {
    if (Firebase.RTDB.setFloat(&fbdo, path, value)) {
        Serial.println("Đã gửi dữ liệu thành công: " + String(path));
        return true;
    } else {
        Serial.print("Lỗi khi gửi dữ liệu: ");
        Serial.println(fbdo.errorReason().c_str());
        return false;
    }
}
// Hàm tính giá trị ppm từ giá trị ADC dựa trên tỷ lệ Rs/R0
float calculatePPM(int analogValue) {
    float Vout = analogValue * (5.0 / 4095.0);  // Giả sử tham chiếu ADC là 5V, độ phân giải 12-bit (ESP32)
    float Rs = (5.0 - Vout) / Vout * R0;        // Tính Rs dựa trên Vout và điện trở R0
    float ratio = Rs / R0;                      // Tính tỷ lệ Rs/R0 để dùng cho biểu đồ
    float ppm = (1000 * pow(ratio, -2.62))/362;       // Công thức tính ppm dựa trên biểu đồ log-log của MQ2
    return ppm;
}
void sendDataToFirebase(String path, float temperature, float humidity) {
    checkFirebaseWrite(path + "/Nhietdo", temperature);
    checkFirebaseWrite(path + "/Doam", humidity);
}

void sendMQ2DataToFirebase(String path, int gasValue) {
    checkFirebaseWrite(path + "/Nongdogas", gasValue);
}

void getLedStateFromFirebase() {
    int led1State, led2State, led3State;
    
    if (Firebase.RTDB.getInt(&fbdo, "/thietbi1/dienthoai")) {
        led1State = fbdo.intData();
        digitalWrite(LED1, led1State);
        Serial.print("LED1: "); Serial.println(led1State);
    }
    if (Firebase.RTDB.getInt(&fbdo, "/thietbi2/den1")) {
        led2State = fbdo.intData();
        digitalWrite(LED2, led2State);
        Serial.print("LED2: "); Serial.println(led2State);
    }
    if (Firebase.RTDB.getInt(&fbdo, "/thietbi3/loa")) {
        led3State = fbdo.intData();
        digitalWrite(LED3, led3State);
        Serial.print("LED3: "); Serial.println(led3State);
    }
}

void loop() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Mất kết nối WiFi, đang thử kết nối lại...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(5000);
        return;
    }

    // Đọc dữ liệu từ từng cảm biến DHT
    float temp1 = dht1.readTemperature();
    float hum1 = dht1.readHumidity();
    float temp2 = dht2.readTemperature();
    float hum2 = dht2.readHumidity();
    float temp3 = dht3.readTemperature();
    float hum3 = dht3.readHumidity();
    float temp4 = dht4.readTemperature();
    float hum4 = dht4.readHumidity();

    // Đọc giá trị từ cảm biến MQ2
    int mq2Value1 = analogRead(MQ2PIN_1);
    int mq2Value2 = analogRead(MQ2PIN_2);
    int mq2Value3 = analogRead(MQ2PIN_3);
    int mq2Value4 = analogRead(MQ2PIN_4);
    
    float mq2PPM1 = calculatePPM(mq2Value1);
    float mq2PPM2 = calculatePPM(mq2Value2);
    float mq2PPM3 = calculatePPM(mq2Value3);
    float mq2PPM4 = calculatePPM(mq2Value4);

    // Kiểm tra lỗi đọc dữ liệu
    if (isnan(temp1) || isnan(hum1) || isnan(temp2) || isnan(hum2) || 
        isnan(temp3) || isnan(hum3) || isnan(temp4) || isnan(hum4)) {
        Serial.println("Lỗi: Không đọc được dữ liệu từ một hoặc nhiều cảm biến DHT!");
        return;
    }

    Serial.println("--- Dữ liệu từ các cảm biến DHT ---");
    Serial.printf("DHT1 - Nhiệt độ: %.2f°C | Độ ẩm: %.2f%%\n\r", temp1, hum1);
    Serial.printf("DHT2 - Nhiệt độ: %.2f°C | Độ ẩm: %.2f%%\n\r", temp2, hum2);
    Serial.printf("DHT3 - Nhiệt độ: %.2f°C | Độ ẩm: %.2f%%\n\r", temp3, hum3);
    Serial.printf("DHT4 - Nhiệt độ: %.2f°C | Độ ẩm: %.2f%%\n\r", temp4, hum4);

    Serial.println("--- Dữ liệu từ các cảm biến MQ2 ---");
    Serial.printf("MQ2_1 Gas PPM: %.2f ppm\n\r", mq2PPM1);
    Serial.printf("MQ2_2 Gas PPM: %.2f ppm\n\r", mq2PPM2);
    Serial.printf("MQ2_3 Gas PPM: %.2f ppm\n\r", mq2PPM3);
    Serial.printf("MQ2_4 Gas PPM: %.2f ppm\n\r", mq2PPM4);

    // Gửi dữ liệu lên Firebase
    sendDataToFirebase("/phong101", temp1, hum1);
    sendDataToFirebase("/phong102", temp2, hum2);
    sendDataToFirebase("/phong103", temp3, hum3);
    sendDataToFirebase("/phong104", temp4, hum4);

    sendMQ2DataToFirebase("/phong101", mq2PPM1);
    sendMQ2DataToFirebase("/phong102", mq2PPM2);
    sendMQ2DataToFirebase("/phong103", mq2PPM3);
    sendMQ2DataToFirebase("/phong104", mq2PPM4);
    
    // Lấy trạng thái LED từ Firebase
    getLedStateFromFirebase();

    delay(5000); // Đợi 5 giây trước khi đọc và gửi dữ liệu tiếp theo
}
