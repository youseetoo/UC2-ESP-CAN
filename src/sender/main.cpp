#include <SPI.h>
#include <mcp_canbus.h>

#define SPI_CS_PIN  D7
MCP_CAN CAN(SPI_CS_PIN); // Set CS pin

// Define MotorData structure
struct MotorData {
    bool directionPinInverted = false;
    long speed = 0;
    long maxspeed = 200000;
    long acceleration = 0;
    long targetPosition = 0;
    long currentPosition = 0;
    int isforever = false;
    bool isaccelerated = false;
    bool absolutePosition = false;
    bool isEnable = true;
    int qid = -1;
    bool isStop = false;
    bool isActivated = 0;
    bool stopped = true;
    bool endstop_hit = false;
    bool isTriggered = false;
    long offsetTrigger = 0;
    long triggerPeriod = -1;
    int triggerPin = -1;
    int dirPin = -1;
    int stpPin = -1;
} __attribute__((packed));

MotorData motorData;

void setup() {
    Serial.begin(115200);
    while (!Serial);

    while (CAN_OK != CAN.begin(CAN_500KBPS)) { // Initialize CAN at 500 kbps
        Serial.println("CAN BUS FAIL!");
        delay(100);
    }
    Serial.println("CAN BUS OK!");
}

void loop() {
    // Populate MotorData structure with sample data
    motorData.speed = 1000;
    motorData.targetPosition = 5000;

    // Serialize structure to a byte array
    const int dataSize = sizeof(MotorData);
    uint8_t buffer[dataSize];
    memcpy(buffer, &motorData, dataSize);

    // Send data in 8-byte chunks
    for (int i = 0; i < dataSize; i += 8) {
        uint8_t frameData[8] = {0}; // 8-byte buffer for CAN frame
        int chunkSize = min(8, dataSize - i);
        memcpy(frameData, buffer + i, chunkSize);

        // Send CAN frame with ID 0x01
        if (CAN.sendMsgBuf(0x01, 0, 8, frameData) == CAN_OK) {
            Serial.print("Sent chunk ");
            Serial.println(i / 8);
        } else {
            Serial.println("CAN send failed!");
        }
        delay(10); // Small delay between frames
    }

    Serial.println("MotorData sent!");
    delay(1000); // Send every second
}
