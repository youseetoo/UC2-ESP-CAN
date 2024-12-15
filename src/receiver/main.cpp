#include <ESP32-TWAI-CAN.hpp>

#ifdef XIAO
#define CAN_TX D8
#define CAN_RX D7
#else
#define CAN_TX GPIO_NUM_13
#define CAN_RX GPIO_NUM_12
#endif

struct MotorDataLarge
{
    uint8_t flags;
    int16_t speed;
    int32_t targetPosition;
    int32_t currentPosition;
    uint32_t timestamp;
    uint8_t checksum;
};

MotorDataLarge motorData;
uint8_t reassemblyBuffer[32];
uint8_t chunkCount = 0;

void sendModifiedData()
{
    const uint8_t chunkSize = 8;
    uint8_t *dataPtr = (uint8_t *)&motorData;

    for (uint8_t i = 0; i < (sizeof(motorData) + chunkSize - 1) / chunkSize; i++)
    {
        CanFrame frame;
        frame.identifier = 0x302;
        frame.extd = 0;
        frame.data_length_code = chunkSize;

        frame.data[0] = i;
        frame.data[1] = (i == 0) ? 1 : 0;
        frame.data[2] = (i == (sizeof(motorData) / chunkSize)) ? 1 : 0;

        memcpy(&frame.data[3], dataPtr + (i * chunkSize), chunkSize - 3);

        if (!ESP32Can.writeFrame(&frame))
        {
            Serial.printf("Chunk %d send failed\n", i);
        }
    }
}

void receiveSegmentedData(CanFrame *frame)
{
    uint8_t chunkNum = frame->data[0];
    bool isStartFrame = frame->data[1];
    bool isEndFrame = frame->data[2];

    memcpy(reassemblyBuffer + (chunkNum * 5), &frame->data[3], 5);

    if (isStartFrame)
    {
        chunkCount = 0; // Reset reassembly
    }

    chunkCount++;

    if (isEndFrame)
    {
        memcpy(&motorData, reassemblyBuffer, sizeof(motorData));
        Serial.println("MotorData Reassembled:");
        Serial.printf("Flags: %d, Speed: %d, TargetPosition: %ld, CurrentPosition: %ld, Timestamp: %lu\n",
                      motorData.flags, motorData.speed, motorData.targetPosition, motorData.currentPosition, motorData.timestamp);

        // Modify motor data values
        motorData.speed += 100;
        motorData.targetPosition += 1000;
        motorData.currentPosition += 500;
        motorData.timestamp = millis();

        sendModifiedData();
    }
}

void requestMotorData()
{
    CanFrame frame;
    frame.identifier = 0x300; // Request ID
    frame.extd = 0;
    frame.data_length_code = 1;
    frame.data[0] = 0x01;

    if (ESP32Can.writeFrame(&frame))
    {
        Serial.println("MotorData Request Sent");
    }
    else
    {
        Serial.println("MotorData Request Failed");
    }
}

void setup()
{
    Serial.begin(115200);
    ESP32Can.setPins(CAN_TX, CAN_RX);
    ESP32Can.begin(ESP32Can.convertSpeed(500));
}

void loop()
{
    static uint32_t lastRequest = 0;
    if (millis() - lastRequest > 2000)
    {
        lastRequest = millis();
        requestMotorData();
    }

    CanFrame frame;
    if (ESP32Can.readFrame(&frame))
    {
        if (frame.identifier == 0x301)
        { // Incoming MotorData
            receiveSegmentedData(&frame);
        }
    }
}
