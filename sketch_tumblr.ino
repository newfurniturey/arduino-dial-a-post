/**
 * Tumblr Dial-a-Post
 * 
 * This sketch, for the Arduino GSM shield, waits for audio
 * input and then initiates an outgoing call. After the
 * call disconnects, a text message is sent.
 * Incoming calls are also accepted.
 * 
 * Circuit:
 * GSM shield attached to and Arduino
 * SIM card that can place/receive phone calls and
 *   send/receive SMS messages
 * Microphone attached to GSM shield
 */

#include <GSM.h>

/**
 * Your SIM's PIN number, if it requires one
 * @var string
 */
#define PINNUMBER ""

/**
 * Library initialization
 */
GSM gsmAccess; // include a 'true' parameter for debug enabled
GSMVoiceCall vcs;
GSM_SMS sms;

/**
 * Holds the phone number for the sender of the received SMS.
 * @var string
 */
char smsFromNumber[20];

/**
 * Tumblr's Dial-a-Post number
 * @var string
 */
char tumblrNumber[20] = "18665846757";

/**
 * Telephone number to send the initial "we're connected" SMS to.
 * Prefix with the country-code, such as `1` for the U.S.
 * Sample: `18005551212`
 * @var string
 */
char notificationNumber[20] = "";

const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)
unsigned int sample;

void setup() {
    // setup the LEDs that indicate connection status
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    // initialize serial communications
    Serial.begin(9600);
    while (!Serial) {
        ; // wait for the port to connect
    }

    Serial.println("Tumblr Dial-a-Post");
    Serial.println("------------------");

    // wait for the GSM connection
    Serial.print("Connecting to GSM Network");
    while (gsmAccess.begin(PINNUMBER) != GSM_READY) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nConnected!");
}

void receiveTextMsg() {
    char c;
    
    Serial.println("Message received from:");
    sms.remoteNumber(smsFromNumber, 20);
    Serial.println(smsFromNumber);
 
    /**
     * @note We can use `sms.peek()` here, or check the contents
     * of the SMS after parsed, to setup a Command-and-Control
     * style device.
     * For now, we just output the message.
     */
 
    // read in the message
    while (c = sms.read()) {
        Serial.print(c);
    }
 
    Serial.println("\n-");
    sms.flush();
}

void sendNotificationMsg() {
    Serial.println("Sending notification to:");
    Serial.println(notificationNumber);

    sms.beginSMS(notificationNumber);
    sms.print("A new message has been recorded.");
    sms.endSMS();
    Serial.println("\nSENT!\n");
}

void makeRecording() {
    Serial.print("Calling: ");
    Serial.println(tumblrNumber);
    Serial.println();

    if (vcs.voiceCall(tumblrNumber)) {
        Serial.println("Call Established. Enter newline to end (or wait for the other side to disconnect)");
        while (Serial.read() != '\n' && (vcs.getvoiceCallStatus() == TALKING)) {
            ; // busy wait
        }
        vcs.hangCall();
    }
    
    Serial.println("Call Finished");
    return;
}

void loop() {
    // check for any SMS messages
    if (sms.available() > 0) {
        // alternate the LEDs while reading in the message
        digitalWrite(4, HIGH);
        digitalWrite(5, LOW);
        receiveTextMsg();
        digitalWrite(5, HIGH);
        digitalWrite(4, LOW);
    }

    // monitor Analog-In to detect "noise"
    unsigned long startMillis = millis();
    unsigned int signalMax = 0;
    unsigned int signalMin = 1024;
     
    // collect data for 50 mS
    while (millis() - startMillis < sampleWindow) {
        sample = analogRead(0);
        if (sample < 1024) {
            if (sample > signalMax) {
                signalMax = sample;  // save just the max levels
            } else if (sample < signalMin) {
                signalMin = sample;  // save just the min levels
            }
        }
    }

    // if the voltage has changed beyond our threshold (10%), there
    // is ample noise in the area - most likely from people talking
    double volts = ((signalMax - signalMin) * 5.0) / 1024;
    Serial.println(volts);
    if (volts > 0.10) {
        // alternate the LEDs while we call Tumblr (^_^)
        digitalWrite(4, HIGH);
        digitalWrite(5, LOW);
        makeRecording();
        sendNotificationMsg();
        digitalWrite(5, HIGH);
        digitalWrite(4, LOW);
    }

    delay(1000);
}

