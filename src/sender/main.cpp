#include <ESP32-TWAI-CAN.hpp>

#define CAN_TX D8
#define CAN_RX D7

struct MotorDataLarge {
    uint8_t flags;
    int16_t speed;
    int32_t targetPosition;
    int32_t currentPosition;
    uint32_t timestamp;
    uint8_t checksum;
};

MotorDataLarge motorData = {0x01, 1200, 50000, 10000, 0, 0xAA};
MotorDataLarge storedData;

void sendSegmentedData(uint16_t msgID, void* data, size_t dataSize) {
    const uint8_t chunkSize = 8;
    uint8_t* dataPtr = (uint8_t*)data;

    for (uint8_t i = 0; i < (dataSize + chunkSize - 1) / chunkSize; i++) {
        CanFrame frame;
        frame.identifier = msgID;
        frame.extd = 0;
        frame.data_length_code = chunkSize;

        frame.data[0] = i;                      
        frame.data[1] = (i == 0) ? 1 : 0;       
        frame.data[2] = (i == (dataSize / chunkSize)) ? 1 : 0; 

        memcpy(&frame.data[3], dataPtr + (i * chunkSize), chunkSize - 3);

        if (!ESP32Can.writeFrame(&frame)) {
            Serial.printf("Chunk %d send failed\n", i);
        }
    }
}

void setup() {
    Serial.begin(115200);
    ESP32Can.setPins(CAN_TX, CAN_RX);
    ESP32Can.begin(ESP32Can.convertSpeed(500));

    motorData.timestamp = millis();
}

void loop() {
    CanFrame frame;
    if (ESP32Can.readFrame(&frame)) {
        if (frame.identifier == 0x300) {  // Request for updated MotorData
            Serial.println("MotorData Request Received");
            sendSegmentedData(0x301, &motorData, sizeof(motorData));
        } 
        else if (frame.identifier == 0x302) {  // Modified MotorData received
            memcpy(&storedData, &frame.data[3], sizeof(MotorDataLarge));
            Serial.println("Modified MotorData Received:");
            Serial.printf("Flags: %d, Speed: %d, TargetPosition: %ld, CurrentPosition: %ld, Timestamp: %lu\n",
                          storedData.flags, storedData.speed, storedData.targetPosition, storedData.currentPosition, storedData.timestamp);
        }
    }
}
