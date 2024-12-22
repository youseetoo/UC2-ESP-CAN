#include <Arduino.h>
#include "iso-tp-twai/CanIsoTp.hpp"

#ifdef XIAO
uint8_t pinTX = D8;
uint8_t pinRX = D7;
#else
uint8_t pinTX = GPIO_NUM_13;
uint8_t pinRX = GPIO_NUM_12;
#endif

typedef struct
{
    uint32_t counter;
    uint32_t counter1;
    uint32_t counter2;
    uint32_t counter3;
    uint32_t counter4;
    uint32_t counter5;
    uint32_t counter6;
    uint32_t counter7;
    uint32_t counter8;
    uint32_t counter9;
    uint32_t counter10;
    uint32_t counter11;

} MessageData;

CanIsoTp isoTpSender;
MessageData txData, rxData;
pdu_t txPdu, rxPdu;

unsigned long lastSend = 0;

void dispatchIsoTpData(uint32_t id, const uint8_t* data, size_t size)
{
    switch (id)
    {
        case 0x123:
        {
            // Parse as SomeEngineData
            MessageData engine;
            if (size >= sizeof(engine)) {
                memcpy(&engine, data, sizeof(engine));
                // Do something with engine data
                Serial.printf("Engine RPM: %u\n", engine.counter);
            }
            break;
        }
        case 0x456:
        {
            // Parse as SomeOtherData
            break;
        }
        default:
        {
            // Unknown ID
            Serial.println("Unknown message ID");
            // Possibly dump raw bytes
        }
    }
}


void setup()
{
    Serial.begin(115200);
    if (!isoTpSender.begin(500, pinTX, pinRX))
    {
        Serial.println("Failed to start TWAI");
        while (1)
            ;
    }

    // Initialize data
    txData.counter = 0;

    // Setup Tx PDU
    txPdu.txId = 0x123;
    txPdu.rxId = 0x456;
    txPdu.data = (uint8_t *)&txData;
    txPdu.len = sizeof(txData);
    txPdu.cantpState = CANTP_IDLE;
    txPdu.blockSize = 0;
    txPdu.separationTimeMin = 5;

    // Setup Rx PDU for responses
    rxPdu.txId = 0x456; // Receiver's ID
    rxPdu.rxId = 0;     // broadcast - listen to all ids; 0x123; // Sender's ID
    rxPdu.data = (uint8_t *)&rxData;
    rxPdu.len = sizeof(rxData);
    rxPdu.cantpState = CANTP_IDLE;
    rxPdu.blockSize = 0;
    rxPdu.separationTimeMin = 0;
}

void loop()
{
    // Send a message every 1 second
    if (millis() - lastSend >= 100)
    {
        lastSend = millis();
        txData.counter++;
        txPdu.data = (uint8_t *)&txData;
        txPdu.len = sizeof(txData);
        if (isoTpSender.send(&txPdu) == 0)
        {
            Serial.print("Sender: Sent counter = ");
            Serial.println(txData.counter);
        }
        else
        {
            Serial.println("Sender: Error sending");
        }
    }

    
    // receive data from receiver always
    int result = isoTpSender.receive(&rxPdu);
    if (result == 0 && rxPdu.cantpState == CANTP_END)
    {
        /*
        Serial.print("Sender: Received response counter = ");
        Serial.println(rxData.counter);
        Serial.print("Sender ID: ");
        Serial.println(rxPdu.rxId);
            // Dispatch by ID
            */
        dispatchIsoTpData(rxPdu.rxId, rxPdu.data, rxPdu.len);

        // Free the data once done
        // free(rxPdu->data); // will be done inside the receive function
        
    }
    else
    {
        Serial.print("Sender: No response or error");
    }
}