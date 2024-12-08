#include <SPI.h>
#include <mcp_canbus.h>

#define SPI_CS_PIN D7
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
uint8_t rxBuffer[sizeof(MotorData)]; // Buffer to collect the structure
int rxBufferIndex = 0;

void printMotorData(MotorData &data) {
    Serial.println("Received MotorData:");
    Serial.print("Speed: ");
    Serial.println(data.speed);
    Serial.print("Target Position: ");
    Serial.println(data.targetPosition);
}

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
    unsigned char len = 0;
    unsigned char buf[8];

    if (CAN_MSGAVAIL == CAN.checkReceive()) { // Check if data is available
        CAN.readMsgBuf(&len, buf);            // Read data

        // Append received chunk to the rxBuffer
        memcpy(rxBuffer + rxBufferIndex, buf, len);
        rxBufferIndex += len;

        // If all bytes of MotorData are received
        if (rxBufferIndex >= sizeof(MotorData)) {
            memcpy(&motorData, rxBuffer, sizeof(MotorData)); // Deserialize data
            printMotorData(motorData);

            // Reset buffer index for next message
            rxBufferIndex = 0;
        }
    }
}
