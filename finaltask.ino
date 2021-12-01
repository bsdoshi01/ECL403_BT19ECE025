/*
 * Bhavesh Doshi
 * BT19ECE025
 * ECL403 - Final Exam Task
*/

//Including required libraries
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

//Initialise WiFi credentials
const char* ssid = "FTTH";
const char* password = "Home@Bsnl0";

//Initialise ThingSpeak Server credentials
const char* serverName = "https://api.thingspeak.com/update";
String apiKey = "RDX3HSYGS7520NYT";

//Initialise Telegram Bot Credentials
#define BOT_TOKEN "2122746346:AAELDmAKEUUXuGALUQqiSGxHiO0ty4KDrC0"
#define CHAT_ID "898181560"

//User interface LED
#define contactPin 23

//Set up Telegram Bot
WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

//Defining touch pins for input from 0-8
int touchPins[] = {4, 2, 15, 13, 12, 14, 27, 33, 32};

//Initialising global variables with values of account balance, notes availibility and login status
int accountBalance = 25000;
int numberNotesAvailable[] = {5, 10, 10};
int currencyValue[] = {2000, 1000, 500};
int loginStatus = 0;

//Variables to check for new messages
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;

//Fucntion to update ThingSpeak cloud values with the current values
void updateThingSpeak() {
  //Start http client and setup server
  HTTPClient http;
  http.begin(serverName);
  //Create the url with data
  String dataSent = "api_key=" + apiKey + "&field1=" + String(accountBalance) + "&field2=" + String(numberNotesAvailable[0]) + "&field3=" + String(numberNotesAvailable[1]) + "&field4=" + String(numberNotesAvailable[2]);
  //Send response
  int Response = http.POST(dataSent);
  //Print the response code on serial monitor
  Serial.print("Response of update ThingSpeak: ");
  Serial.println(Response);
}

//Generated a random pin and validates it with the user input
void pinValidation() {
  int lower = 0, upper = 8, generatedPin;
  //Generate a random 2 digit pin with digits 0-8
  generatedPin = (10 * ((rand() % (upper - lower + 1)) + lower)) + ((rand() % (upper - lower + 1)) + lower);
  String loginPinMessage = "Your PIN as a OTP is: " + String(generatedPin); //Send pin as message to use via Telegram
  bot.sendMessage(CHAT_ID, loginPinMessage, "");
  Serial.println(loginPinMessage);
  int enteredPin = userTouchInput(2); //Get pin from user via touch input for authentication
  Serial.print("User entered pin: ");
  Serial.println(enteredPin);
  loginStatus = ((enteredPin == generatedPin) ? 1 : 0); //Check if pin is correct
  if (loginStatus) {
    bot.sendMessage(CHAT_ID, "Pin entered is correct, Login Successful", ""); //Display pin status and login status
  } else {
    bot.sendMessage(CHAT_ID, "Pin entered is incorrect, Login Unsuccessful", "");
  }
}

//Returns the user input, argument is the length of user input
int userTouchInput(int numberDigits) {
  int userTouchInputData = 0; //Initialise input to 0
  int instTouchValue = 100;
  int avgTouchValue;
  int inputDigit; //Variable to store the input digit
  while (numberDigits--) {
    digitalWrite(contactPin, HIGH); //Turn LED on to indicate user can input
    int flag = 1;
    for (int i = 0; i <=8; ++i) {
      instTouchValue = touchRead(touchPins[i]); //Remove unwanted temperory buffered inputs
      delay(10);
    }
    while (flag) { //While input not recieved
      for (int i = 0; i <= 8 && flag; ++i) { //Run through all 9 pins
        delay(10);
        avgTouchValue = 0; //Average value var
        for (int j = 0; j < 5; ++j) { //Get 5 values and take average to eliminate undesired output
          instTouchValue = touchRead(touchPins[i]);
          avgTouchValue += instTouchValue;
        }
        avgTouchValue /= 5;
        if (avgTouchValue < 10) { //Check if user touch is there
          digitalWrite(contactPin, LOW); //Turn LED off to indicate input recieved
          //Display input with recorded mean value
          Serial.print(i);
          Serial.print(" - ");
          Serial.println(avgTouchValue);
          inputDigit = i; //Set input digit
          flag = 0; //Set flag to indicate input recieved
        }
      }
    }
    userTouchInputData = inputDigit + (10 * userTouchInputData); //Add the digit to the number 
    delay(2000); //Delay for next input
  }
  return userTouchInputData; //Return the user input
}

//Fuction to withdraw money if conditions are met and update the same
void moneyWithdrawal() {
  //Get the number of digits in the withdrawal amount
  bot.sendMessage(CHAT_ID, "Please enter the number of digits in the withdrawal amount through touch input", "");
  int numberDigitsWithdraw = userTouchInput(1);
  String numberDigitsMessage = "The number of digits entered is: " + String(numberDigitsWithdraw);
  bot.sendMessage(CHAT_ID, numberDigitsMessage, "");
  //Get the amount from the user
  bot.sendMessage(CHAT_ID, "Please enter the amount", "");
  int withdrawalAmount = userTouchInput(numberDigitsWithdraw);
  String withdrawAmountMessage = "The amount entered is: " + String(withdrawalAmount);
  bot.sendMessage(CHAT_ID, withdrawAmountMessage, "");
  if (withdrawalAmount <= accountBalance) { //Check if balance available
    bot.sendMessage(CHAT_ID, "Balance Available, processing transaction", "");
    if (withdrawalAmount % 500 == 0) { //If exact amount can be processed proceed
      int notesRequired[] = {0, 0, 0}; //Initialise requirement of notes to 0
      int tempAmount = withdrawalAmount; //Create a temp var
      for (int j = 0; j < 3; ++j) { //Find number of notes of each type required
        notesRequired[j] = ((tempAmount / currencyValue[j] <= numberNotesAvailable[j]) ? tempAmount / currencyValue[j] : numberNotesAvailable[j]);
        tempAmount -= (notesRequired[j] * currencyValue[j]);
      }
      if (tempAmount == 0) { //If successful, process further
        for (int j = 0; j < 3; ++j) {
          numberNotesAvailable[j] -= notesRequired[j];
        }
        accountBalance -= withdrawalAmount; //reduce balance and notes availibility
        //Update cloud and the user
        updateThingSpeak();
        String updateBalance = "Your new balance is: " + String(accountBalance);
        bot.sendMessage(CHAT_ID, updateBalance, "");
        Serial.println(updateBalance);
      } else {
        bot.sendMessage(CHAT_ID, "Something went wrong", ""); //Error message
      }
    } else { //Send message that the amount is not available as only specific notes are available
      bot.sendMessage(CHAT_ID, "Sorry, the transaction has been cancelled", "");
      bot.sendMessage(CHAT_ID, "Only denominations of Rs 500, Rs 1000 and Rs 2000 available. Please retry with different amount", "");
    }
  } else {
    bot.sendMessage(CHAT_ID, "Insufficient Balance", "");
  }
}

//Handles new messages in telegram and procedes as per request
void handleNewMessage(int numNewMessages) {
  Serial.println("handleNewMessage");
  Serial.println(String(numNewMessages));
  for (int i=0; i<numNewMessages; i++) { //For the number of new messages
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){ //Check if sent by authorised user
      bot.sendMessage(chat_id, "Unauthorized user", "");
      break;
    }
    String text = bot.messages[i].text;
    Serial.println(text);
    String from_name = bot.messages[i].from_name;
    if (text == "/start") { //Start message
      String welcome = "Welcome " + from_name + ".\n";
      welcome += "Available Commands: \n";
      welcome += "/login to get PIN as a OTP for login\n";
      welcome += "/logout to logout\n";
      welcome += "/balance to check the balance of your bank account\n";
      welcome += "/moneywithdraw to withdraw money\n";
      bot.sendMessage(chat_id, welcome, "");
    }
    if (text == "/login") { //Login, calls for pin otp validation
      if (loginStatus == 0) {
        pinValidation();
      } else {
        bot.sendMessage(CHAT_ID, "You are already logged in", "");
      }
    }
    if (text == "/moneywithdraw") { //Calls moneywithdrawal to withdraw money
      if (loginStatus) {
        moneyWithdrawal();
      } else {
        bot.sendMessage(CHAT_ID, "Please login first", "");
      }
    }
    if (text == "/balance") { //To check balance
      if (loginStatus) {
        String balanceMessage = "The balance in your account is: " + String(accountBalance);
        bot.sendMessage(CHAT_ID, balanceMessage, "");
        Serial.println(balanceMessage);
      } else {
        bot.sendMessage(CHAT_ID, "Please login first", "");
      }
    }
    if (text == "/logout") { //To end the session
      if (loginStatus) {
        loginStatus = 0; 
        bot.sendMessage(CHAT_ID, "You have been sucessfully logged out", "");
        Serial.println("Logout");
      } else {
        loginStatus = 0; 
        bot.sendMessage(CHAT_ID, "You are already logged out", "");
      }
    }
  }
}

//Initialisation of different pins
void initialisePins() {
  for (int i = 0; i <= 8; ++i) {
    pinMode(touchPins[i], INPUT); //Touch pins setup as input
  }
  pinMode(contactPin, OUTPUT); //User interface LED as output
  bot.sendMessage(CHAT_ID, "Welcome", ""); //Welcome message to indicate initialisation complete
  Serial.println("Welcome");
}

//Setup Function
void setup() {
  //Begin Serial Monitor
  Serial.begin(115200);
  delay(100);
  //Start WiFi in station mode
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected");
  secured_client.setInsecure();
  //Initialise Thing Speak with default values
  updateThingSpeak();
  //Initialise input output pins and display welcome message
  initialisePins();
}

//Loop Function
void loop() {
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1); //Check for new messages
    while(numNewMessages) {
      Serial.println("New Message Recieved");
      handleNewMessage(numNewMessages); //Handle new messages
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}
