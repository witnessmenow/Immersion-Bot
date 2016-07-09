#include <UniversalTelegramBot.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>

#define SERVO_CONTROL_PIN D1

#define NEUTRAL_ANGLE 72
#define ON_ANGLE 140
#define OFF_ANGLE 0
#define STEPS 1
#define DELAY 15

#define TOGGLE_BUTTON_PIN D2
#define BOOST_BUTTON_PIN D5

//An hour in milliseconds
#define BOOST_DELAY 3600000

#define BOTtoken "XXXXXXX:XXXXXXXXXXXXXXXXXXXXXXXXX"  //Get not token from botfather

char ssid[] = "XXXXXXX";              // your network SSID (name)
char pass[] = "XXXXXXXX";                              // your network key

int boostActive = 0;
int imersionStatus = -1;
unsigned long boostStartTime;

volatile int boostButtonPressed;
volatile int toggleButtonPressed;

int lastToggleButtonState = LOW;
long lastToggleTime = 0;  // the last time the output pin was toggled
long coolDownDelay = 500;    // the debounce time; increase if the output flickers

Servo myservo;
int pos = 0;    // variable to store the servo position

WiFiClientSecure client;

UniversalTelegramBot bot(BOTtoken, client);
int Bot_mtbs = 10000; //wait time between checking Telegram for new messages
long Bot_lasttime;   //last time messages' scan has been done

// The bot will allow only accept commands from authenticatedUsers
// The bot will post status messages to the chat

// Use @myidbot to get these Telegram Ids

int numberAuthUsers = 2;
String authenticatedUsers[2] = {"987654321", "987665544"};
String chat = "-1123456789";

void setup()
{
  Serial.begin(115200);
  myservo.attach(SERVO_CONTROL_PIN);
  myservo.write(NEUTRAL_ANGLE);
  myservo.detach();

  pinMode(TOGGLE_BUTTON_PIN, INPUT);
  pinMode(BOOST_BUTTON_PIN, INPUT);


  attachInterrupt(TOGGLE_BUTTON_PIN, interuptToggleButton, RISING);
  attachInterrupt(BOOST_BUTTON_PIN, interuptBoostButton, RISING);

   // attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  IPAddress ip = WiFi.localIP();
  Serial.println(ip);

  if(bot.getMe()){
    Serial.print("Bot name: ");
    Serial.println(bot.userName);
  } else {
    Serial.println("getMeFailed");
  }

  bot.sendMessage(chat, "Starting up", "");
}



void turnImersionOff()
{
  if(myservo.attached() == false)
  {
     myservo.attach(SERVO_CONTROL_PIN);
  }
  for(pos = NEUTRAL_ANGLE; pos > OFF_ANGLE; pos -= STEPS)
  {
    myservo.write(pos);
    delay(DELAY);
  }
  imersionStatus = 0;
  myservo.write(NEUTRAL_ANGLE);
  myservo.detach();
}

void turnImersionOn()
{
  if(myservo.attached() == false)
  {
     myservo.attach(SERVO_CONTROL_PIN);
  }
  for(pos = NEUTRAL_ANGLE; pos < ON_ANGLE; pos += STEPS)
  {
    myservo.write(pos);
    delay(DELAY);
  }
  imersionStatus = 1;
  myservo.write(NEUTRAL_ANGLE);
  myservo.detach();

}

void handleNewMessages(int numNewMessages) {
  bool auth = false;
  for(int i=0; i<numNewMessages; i++) {
    for(int j=0; j<numberAuthUsers; j++ ) {
      if(bot.messages[i].from_id == authenticatedUsers[j] )
      {
        auth = true;
        j = numberAuthUsers;
      }
    }
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    if (auth)
    {
      if (text == "\/on") {
        Serial.println("got Telegram /on");
        turnImersionOn();
        bot.sendMessage(chat_id, "Imersion is now on", "");
      }
      if (text == "\/off") {
        Serial.println("got Telegram /off");
        turnImersionOff();
        bot.sendMessage(chat_id, "Imersion is now off", "");
      }
      if (text == "\/boost") {
        Serial.println("got Telegram /boost");
        turnOnBoost();
        bot.sendMessage(chat_id, "Imersion is now on with boost", "");
      }
      if (text == "\/status") {
        if(imersionStatus == 1){
          bot.sendMessage(chat_id, "Imersion is ON", "");
        } else if(imersionStatus == 0)
        {
          bot.sendMessage(chat_id, "Imersion is OFF", "");
        } else
        {
          bot.sendMessage(chat_id, "I'm not sure", "");
        }
      }
      if (text == "\/start") {
        StaticJsonBuffer<500> jsonBuffer;
        JsonObject& payload = jsonBuffer.createObject();
        payload["chat_id"] = chat_id;
        payload["text"] = "Welcome to Imersion bot";
        JsonObject& replyMarkup = payload.createNestedObject("reply_markup");

        StaticJsonBuffer<200> keyboardBuffer;
        char keyboardJson[] = "[[\"/on\", \"/off\"],[\"/boost\"],[\"/status\"]]";
        replyMarkup["keyboard"] = keyboardBuffer.parseArray(keyboardJson);
        replyMarkup["resize_keyboard"] = true;

        bot.sendPostMessage(payload);
      }
    }
    else {
      bot.sendMessage(chat_id, "You are not authorized", "");
    }
  }
}

void telegremMessageRead() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {

    int numNewMessages = bot.getUpdates(bot.last_message_recived + 1);
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_recived + 1);
    }
    Bot_lasttime = millis();
  }
}


void processToggleButton() {
  if ((millis() - lastToggleTime) > coolDownDelay) {
    if(toggleButtonPressed) {
      toggleButtonPressed = LOW;
      lastToggleTime = millis();
      if(imersionStatus == 1){
        Serial.println("toggle pressed off");
        turnImersionOff();
        bot.sendMessage(chat, "Immersion turned OFF by button", "");
      } else {
        Serial.println("toggle pressed on");
        turnImersionOn();
        bot.sendMessage(chat, "Immersion turned ON by button", "");
      }
    }

  }

  return;

}

void turnOnBoost() {
  boostStartTime = millis();
  boostActive = 1;
  Serial.println("Boost on");
  turnImersionOn();
}

void processBoostButton() {

  if (boostButtonPressed == HIGH) {
    boostButtonPressed = LOW;
    turnOnBoost();
    bot.sendMessage(chat, "Boost activated, Turned immersion on", "");
  }

  return;
}

void processBoost() {

  if (boostActive) {
    if (millis() > boostStartTime + BOOST_DELAY)
    {
      Serial.println("Boost off");
      // Boost expired
      turnImersionOff();
      boostActive = 0;
      bot.sendMessage(chat, "Boost finished, Turned immersion off", "");
    }
  }
}

void interuptToggleButton()
{
  Serial.println("interuptToggleButton");
  int togglePress = digitalRead(TOGGLE_BUTTON_PIN);
  if(togglePress == HIGH)
  {
    toggleButtonPressed = HIGH;
  }
  return;
}

void interuptBoostButton()
{
  Serial.println("interuptBoostButton");
  int boostPress = digitalRead(BOOST_BUTTON_PIN);
  if(boostPress == HIGH)
  {
    boostButtonPressed = HIGH;
  }
  return;
}

void loop() {
  telegremMessageRead();
  processToggleButton();
  processBoostButton();
  processBoost();
}
