#include <DHT.h>
#include <DHT_U.h>
#include <MKRGSM.h>
#include <RTCZero.h>
#include <SPIMemory.h>
#include "ThingSpeak.h"
#include <ArduinoJson.h>

const char PINNUMBER[] = " ";
const char GPRS_APN[] = "hologram";
const char GPRS_LOGIN[] = " ";
const char GPRS_PASSWORD[] = " ";

//DHT temperature information
#define DHTPIN 4
#define DHTTYPE DHT22
char jsonBuffer[6000] ="[";

//ThingSpeak IDs
unsigned long ThingSpeakChannelID = ;
const char * ThingSpeakWriteKey = "";
char TSserver[] = "api.thingspeak.com";

//Flash information
int timesResetLocation=1;
int forceDayCycle;
int tempStart=256001;
int weatherStart=4000000;
int problemStart=6000000;
const int FlashChipSelect = 5;
int differenceLocation=7900000;


//WeatherMap data
int requestedLines = 10;
char WMserver[] = "api.openweathermap.org"; 
String location = "lat=&lon=";
String appID = "";

//IFTTT data
char IFTTT_Event[] = "highTemp";
char IFserver[] = "maker.ifttt.com"; 
char IFTTT_Key[] = "";

//Input and Output
int buttonState = 0;
const int ledPin = 6; //MKR GSM 1400 on board led on 6
const int buttonPin = 7;

//Objects and Functions
GPRS gprs;
RTCZero rtc;
GSM gsmAccess;
GSMClient client;
DHT dht(DHTPIN,DHTTYPE);
SPIFlash flash(FlashChipSelect);

String readFlash(int add);
String getDT(int inputTime);
String print2digits(int number);
int writeFlash(int add,String str);
void httpRequest(char* jsonBuffer);
void updatesJson(char* jsonBuffer);
char *append_str(char *here, char *s);
int sendNotification(String dateTime);
char *append_ul(char *here, unsigned long u);
int getWeather(int* weatherDate, int* weatherTemp, bool forceReplace=0);
int checkHighTemp(int *weatherDate, int *weatherTemp,int* problemDate, int* problemTemp, int highTemp);


void setup() {
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  Serial.begin(9600);
  while(!Serial);
  
  Serial.println(F("All processes with Timings 04"));
  
  flash.begin(MB(2));
  int timesReset=flash.readLong(timesResetLocation);
  if(timesReset!=-1){
    connectInternet();
  }

  dht.begin();
  rtc.begin();
  if(timesReset!=-1){
    rtc.setEpoch(gsmAccess.getTime());
  }
  ThingSpeak.begin(client);

  if(timesReset==-1){
    resetAll();
  }

  forceDayCycle=flash.readLong(timesResetLocation+32);
  
  Serial.println(F("GSM initialized."));
  gsmAccess.shutdown();
  delay(2000);
  Serial.println(F("\nAwaiting Input.\n"));
  digitalWrite(ledPin, HIGH);
}

void loop() {
  buttonState = digitalRead(buttonPin);

  //Reset Button
  int i;
  if(buttonState == HIGH){
    for(i=0;i<10;i++){
      buttonState = digitalRead(buttonPin);
      if(buttonState == HIGH){
        if(i<5){
          Serial.print(F("Soft reset in: "));
          Serial.println(5-i);
          digitalWrite(ledPin, LOW);
          delay(500);
          digitalWrite(ledPin, HIGH);
          delay(500);
        }
        if(i==5){
          Serial.println(F("Release for soft reset"));
          digitalWrite(ledPin, LOW);
          delay(250);
          digitalWrite(ledPin, HIGH);
          delay(250);
          digitalWrite(ledPin, LOW);
          delay(250);
          digitalWrite(ledPin, HIGH);
          delay(250);
        }
        if(i>5){
          Serial.print(F("Hard reset in: "));
          Serial.println(10-i);
          digitalWrite(ledPin, LOW);
          delay(500);
          digitalWrite(ledPin, HIGH);
          delay(500);
        }
      }
      else{
        Serial.println(F("Canceled"));
        break;
      }
    }
    if(i>=5&&i<10){
      Serial.println("Daily Reset");
      forceDayCycle=100000;
    }
    if(i==10){
      Serial.println("Full Reset");
      resetAll();
    }
  }
  
  //Every 10 minutes take temperature, check if it is too high, and calculate the difference from pridicted if applicable 86400
  if(rtc.getEpoch()%600==0&&rtc.getEpoch()%86400!=0&&forceDayCycle<87000){
    Serial.println(F("Getting temperature"));

    //Store current time and temperature
    int i;
    for(i=0;i<300;i+=2){
      if(flash.readLong(tempStart+(32*i))<=1){
        flash.writeLong(tempStart+(32*i),rtc.getEpoch());
        dht.readTemperature(true);
        delay(3000);
        int temp = dht.readTemperature(true);
        Serial.print(F("Current temp: "));
        Serial.println(temp);
        flash.writeLong(tempStart+(32*(i+1)),temp);
        break;
      }
    }
    delay(3000);

    //Check for high temp
    float overheatingTemp=flash.readFloat(differenceLocation+64);
    if(dht.readTemperature(true)>=overheatingTemp){
      delay(3000);
      if(dht.readTemperature(true)>=overheatingTemp){
        String notification = "Your tunnel is currently overheating with a temperature of: ";
        notification+= String(dht.readTemperature(true));
        notification+= "F";
        connectInternet();
        sendNotification(notification);
        gsmAccess.shutdown();
      }
    }
    delay(3000);

    //Caluclate the difference if the current time has a weather pridiction within +/- 5 minutes
    int j=0;
    for(i=0;i<requestedLines;i++){
      int weatherDate = flash.readLong(weatherStart+(32*j));
      int weatherTemp = flash.readLong(weatherStart+(32*(j+1)));
      if((rtc.getEpoch()<=weatherDate+300)&&(rtc.getEpoch()>=weatherDate-300)){
        int difference = dht.readTemperature(true)-weatherTemp;
        float weight = flash.readFloat(differenceLocation+32);
        float newDifference= ((difference+(flash.readFloat(differenceLocation)*weight))/(weight+1));
        flash.eraseBlock32K(differenceLocation);
        flash.writeFloat(differenceLocation,newDifference);
        if(weight<100){
          flash.writeFloat(differenceLocation+32,(weight+1));
        }
        else{
          flash.writeFloat(differenceLocation+32,weight);
        }
        flash.writeFloat(differenceLocation+64,overheatingTemp);
        break;
      }
      j+=2;
    }
    forceDayCycle+=600;
    int timesReset=flash.readLong(timesResetLocation);
    flash.eraseBlock32K(timesResetLocation);
    flash.writeLong(timesResetLocation,timesReset);
    flash.writeLong(timesResetLocation+32,forceDayCycle);
    Serial.println(F("Finished getting temperature"));
  }

  //Send in the weather once a day or if has not run for a day
  if(rtc.getEpoch()%86400==0||forceDayCycle>=87000){
    Serial.println("Preforming daily upload and download");
    connectInternet();
    int i;
    updatesJson(jsonBuffer);
    httpRequest(jsonBuffer);
    for(i=0;i<5;i++){
      if((getWeather())==1){
        break;
      }
      else if((getWeather())==2){
        i=3;
      }
    }
    
    float difference=flash.readFloat(differenceLocation);
    float weight = flash.readFloat(differenceLocation+32);
    float overheatingTemp = ThingSpeak.readIntField(ThingSpeakChannelID,2,ThingSpeakWriteKey);
    flash.eraseBlock32K(differenceLocation);
    flash.writeFloat(differenceLocation,difference);
    flash.writeFloat(differenceLocation+32,weight);
    flash.writeFloat(differenceLocation+64,overheatingTemp);
    
    checkHighTemp(overheatingTemp-flash.readFloat(differenceLocation));
    int currentDate=rtc.getEpoch();
    int problemDates[50];
    for(i=0;i<50;i++){
      problemDates[i]=flash.readLong(problemStart+(32*i));
      if(problemDates[i]<=currentDate){
        problemDates[i]=-1;
      }
    }
    flash.eraseBlock32K(problemStart);
    for(i=0;i<50;i++){
      flash.writeLong(problemStart+(32*i),problemDates[i]);
    }
    int timesReset=flash.readLong(timesResetLocation);
    flash.eraseBlock32K(timesResetLocation);
    flash.writeLong(timesResetLocation,timesReset);
    flash.writeLong(timesResetLocation+32,0);
    forceDayCycle=0;
    gsmAccess.shutdown();
    delay(5000);
    Serial.println("Finished daily upload and download");
  }
}

void connectInternet(){
  boolean connected = false;
  while(!connected){
    if((gsmAccess.begin() == GSM_READY)&&(gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)){
      Serial.println(F("Connected"));
      connected = true;
    }
    else{
      Serial.println(F("Not connected"));
      int i;
      for(i=0;i<5;i++){
        digitalWrite(ledPin, HIGH); //LED will flash if the SIM can't connect
        delay(250);
        digitalWrite(ledPin, LOW);
        delay(250);
      }
    }
  }
}

// Updates the josnBuffer with data
void updatesJson(char* jsonBuffer){
  // JSON format for updates paramter in the API
  int i;
  for(i=0;i<300;i+=2){
    if(flash.readLong(tempStart+(32*i))>=1){
      strcat(jsonBuffer,"{\"created_at\":");
      unsigned long deltaT = flash.readLong(tempStart+(32*i));//rtc.getEpoch();
      size_t lengthT = String(deltaT).length();
      char temp[11];
      String(deltaT).toCharArray(temp,lengthT+1);
      strcat(jsonBuffer,temp);
      strcat(jsonBuffer,",");
      int fahr = flash.readLong(tempStart+(32*(i+1)));//dht.readTemperature(true);
      //Serial.print(F("Temp: "));
      //Serial.println(fahr);
      strcat(jsonBuffer, "\"field1\":");
      lengthT = String(fahr).length();
      String(fahr).toCharArray(temp,lengthT+1);
      strcat(jsonBuffer,temp);
      strcat(jsonBuffer,"},");
    }
    else{
      break;
    }
  }
  size_t len = strlen(jsonBuffer);
  jsonBuffer[len-1] = ']';
  Serial.println(jsonBuffer);
}

// Updates the ThingSpeakchannel with data
void httpRequest(char* jsonBuffer) {
  // JSON format for data buffer in the API
  char data[6000] = "{\"write_api_key\":\"";
  strcat(data,ThingSpeakWriteKey);
  strcat(data,"\",\"updates\":");
  strcat(data,jsonBuffer);
  strcat(data,"}");
  // Close any connection before sending a new request
  client.stop();
  String data_length = String(strlen(data)+1); //Compute the data buffer length
  Serial.println(data);
  // POST data to ThingSpeak
  if (client.connect(TSserver, 80)) {
    client.print("POST /channels/");
    client.print(ThingSpeakChannelID);
    client.println("/bulk_update.json HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (MKR_GSM_1400)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: "+data_length);
    client.println();
    client.println(data);
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); //Wait to receive the response
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Response code:"+resp); // Print the response code. 202 indicates that the TSserver has accepted the response
  jsonBuffer[0] = '['; //Reinitialize the jsonBuffer for next batch of data
  jsonBuffer[1] = '\0';
  flash.eraseBlock32K(tempStart);
}

int getWeather() { 
  Serial.println("\nStarting connection to WMserver..."); 
  // if you get a connection, report back via serial: 
  if (client.connect(WMserver, 80)) { 
    Serial.println("Connected to WM server"); 
    // Make a HTTP request: 
    client.print("GET /data/2.5/forecast?"); 
    client.print(location); 
    client.print("&appid="+appID); 
    client.print("&cnt="+String(requestedLines)); 
    client.println("&units=imperial"); 
    client.println("Host: api.openweathermap.org"); 
    client.println("Connection: close"); 
    client.println(); 
  } else { 
    Serial.println("Failed to connected to WM server"); 
    return 0;
  } 
  delay(1000); 
  String line = ""; 
  while (client.connected()) { 
    line = client.readStringUntil('\n');
    //String line="{\"cod\":\"200\",\"message\":0.013,\"cnt\":10,\"list\":[{\"dt\":1561572000,\"main\":{\"temp\":83.97,\"temp_min\":81.27,\"temp_max\":84.25,\"pressure\":1015.82,\"sea_level\":1015.82,\"grnd_level\":982.45,\"humidity\":46,\"temp_kf\":1.5},\"weather\":[{\"id\":803,\"main\":\"Clouds\",\"description\":\"broken clouds\",\"icon\":\"04d\"}],\"clouds\":{\"all\":52},\"wind\":{\"speed\":7.18,\"deg\":238.342},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-26 18:00:00\"},{\"dt\":1561582800,\"main\":{\"temp\":85.41,\"temp_min\":83.39,\"temp_max\":85.41,\"pressure\":1014.94,\"sea_level\":1014.94,\"grnd_level\":981.47,\"humidity\":46,\"temp_kf\":1.12},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01d\"}],\"clouds\":{\"all\":4},\"wind\":{\"speed\":6.51,\"deg\":226.226},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-26 21:00:00\"},{\"dt\":1561593600,\"main\":{\"temp\":82.04,\"temp_min\":80.69,\"temp_max\":82.04,\"pressure\":1015.04,\"sea_level\":1015.04,\"grnd_level\":981.9,\"humidity\":59,\"temp_kf\":0.75},\"weather\":[{\"id\":802,\"main\":\"Clouds\",\"description\":\"scattered clouds\",\"icon\":\"03n\"}],\"clouds\":{\"all\":25},\"wind\":{\"speed\":6.55,\"deg\":223.879},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2019-06-27 00:00:00\"},{\"dt\":1561604400,\"main\":{\"temp\":74.79,\"temp_min\":74.11,\"temp_max\":74.79,\"pressure\":1016.27,\"sea_level\":1016.27,\"grnd_level\":982.72,\"humidity\":66,\"temp_kf\":0.37},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10n\"}],\"clouds\":{\"all\":17},\"wind\":{\"speed\":4.41,\"deg\":154.902},\"rain\":{\"3h\":0.125},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2019-06-27 03:00:00\"},{\"dt\":1561615200,\"main\":{\"temp\":72.09,\"temp_min\":72.09,\"temp_max\":78.09,\"pressure\":1016.15,\"sea_level\":1016.15,\"grnd_level\":982.6,\"humidity\":80,\"temp_kf\":0},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10n\"}],\"clouds\":{\"all\":26},\"wind\":{\"speed\":5.23,\"deg\":152.427},\"rain\":{\"3h\":0.063},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2019-06-27 06:00:00\"},{\"dt\":1561626000,\"main\":{\"temp\":70.05,\"temp_min\":70.05,\"temp_max\":70.05,\"pressure\":1016.06,\"sea_level\":1016.06,\"grnd_level\":982.52,\"humidity\":89,\"temp_kf\":0},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}],\"clouds\":{\"all\":0},\"wind\":{\"speed\":3.78,\"deg\":128.697},\"sys\":{\"pod\":\"n\"},\"dt_txt\":\"2019-06-27 09:00:00\"},{\"dt\":1561636800,\"main\":{\"temp\":71.38,\"temp_min\":71.38,\"temp_max\":71.38,\"pressure\":1015.01,\"sea_level\":1015.01,\"grnd_level\":982.04,\"humidity\":89,\"temp_kf\":0},\"weather\":[{\"id\":801,\"main\":\"Clouds\",\"description\":\"few clouds\",\"icon\":\"02d\"}],\"clouds\":{\"all\":16},\"wind\":{\"speed\":6.62,\"deg\":126.909},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-27 12:00:00\"},{\"dt\":1561647600,\"main\":{\"temp\":78.2,\"temp_min\":78.2,\"temp_max\":78.2,\"pressure\":1013.82,\"sea_level\":1013.82,\"grnd_level\":981.52,\"humidity\":80,\"temp_kf\":0},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10d\"}],\"clouds\":{\"all\":53},\"wind\":{\"speed\":10.36,\"deg\":174.227},\"rain\":{\"3h\":1.5},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-27 15:00:00\"},{\"dt\":1561658400,\"main\":{\"temp\":74.98,\"temp_min\":74.98,\"temp_max\":74.98,\"pressure\":1013.71,\"sea_level\":1013.71,\"grnd_level\":979.67,\"humidity\":60,\"temp_kf\":0},\"weather\":[{\"id\":501,\"main\":\"Rain\",\"description\":\"moderate rain\",\"icon\":\"10d\"}],\"clouds\":{\"all\":77},\"wind\":{\"speed\":6.44,\"deg\":191.447},\"rain\":{\"3h\":9.5},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-27 18:00:00\"},{\"dt\":1561669200,\"main\":{\"temp\":82.77,\"temp_min\":82.77,\"temp_max\":82.77,\"pressure\":1013.49,\"sea_level\":1013.49,\"grnd_level\":980.05,\"humidity\":66,\"temp_kf\":0},\"weather\":[{\"id\":500,\"main\":\"Rain\",\"description\":\"light rain\",\"icon\":\"10d\"}],\"clouds\":{\"all\":100},\"wind\":{\"speed\":8.3,\"deg\":214.285},\"rain\":{\"3h\":0.938},\"sys\":{\"pod\":\"d\"},\"dt_txt\":\"2019-06-27 21:00:00\"}],\"city\":{\"id\":5023752,\"name\":\"Dakota County\",\"coord\":{\"lat\":44.6666,\"lon\":-93.0669},\"country\":\"US\",\"population\":398552,\"timezone\":-18000}}";
    Serial.println(line); 
    Serial.print("Size of line: ");
    Serial.println(line.length());
    Serial.println("parsingValues");
    DynamicJsonDocument root(7000);
    auto error = deserializeJson(root,line);
    
    if (error){
      Serial.print(F("deserializeJson() failed with code "));
      Serial.println(error.c_str());
      return 2;
    }

    //Put the weather date and max temp into storage
    flash.eraseBlock32K(weatherStart);
    int i,j=0;
    for(i=0;i<requestedLines;i++){
      int weatherDate=root["list"][i]["dt"];
      int weatherTemp=root["list"][i]["main"]["temp_max"];
      flash.writeLong((weatherStart+(32*j)),weatherDate);
      Serial.println(weatherDate);
      flash.writeLong((weatherStart+(32*(j+1))),weatherTemp);
      Serial.println(weatherTemp);
      j+=2;
    }
  } 
  return 1;
}

void checkHighTemp(int highTemp){
  int i,j=0,k,currentDate,currentTemp;
  int problemDate[requestedLines];
  memset(problemDate,0,sizeof(problemDate));
  bool notWarned=0;
  String notification;
  for(i=0;i<requestedLines;i++){
    currentDate=flash.readLong(weatherStart+(32*j));
    currentTemp=flash.readLong(weatherStart+(32*(j+1)));
    if(currentTemp>=highTemp){
      problemDate[i]=currentDate;
    }
    j+=2;
  }
  for(i=0;i<requestedLines;i++){
    for(j=0;j<50;j++){
      if(problemDate[i]==flash.readLong(problemStart+(32*j))){
        problemDate[i]=0;
        break;
      }
    }
  }
  for(i=0;i<requestedLines;i++){
    if(problemDate[i]!=0){
      notWarned+=1;
    }
  }
  if(notWarned){
    if(notWarned==1){
      notification+="Your tunnel is predicted to overheat at times: ";
    }
    else{
      notification+="Your tunnel is predicted to overheat at time: ";
    }
    for(i=0;i<requestedLines;i++){
      if(problemDate[i]!=0){
        notification+=getDT(problemDate[i]);
        break;
      }
    }
    for(j=i;j<requestedLines;j++){
      if(problemDate[j]!=0){
        notification+=" and ";
        notification+=getDT(problemDate[j]);
      }
    }
    for(i=0;i<5;i++){
      if(sendNotification(notification)){
        break;
      }
    }
  }
}

int sendNotification(String notification) {

    // connect to the Maker event server
    if(client.connect(IFserver, 80)){
      Serial.println("Connected to IFTTT server");
    }
    else{
      Serial.println("Failed to connected to IFTTT server");
      return 0;
    }

    // construct the POST request
    char post_rqst[256+notification.length()];    // hand-calculated to be big enough

    char *p = post_rqst;
    p = append_str(p, "POST /trigger/");
    p = append_str(p, IFTTT_Event);
    p = append_str(p, "/with/key/");
    p = append_str(p, IFTTT_Key);
    p = append_str(p, " HTTP/1.1\r\n");
    p = append_str(p, "Host: maker.ifttt.com\r\n");
    p = append_str(p, "Content-Type: application/json\r\n");
    p = append_str(p, "Content-Length: ");

    // we need to remember where the content length will go, which is:
    char *content_length_here = p;

    // it's always two digits, so reserve space for them (the NN)
    p = append_str(p, "NN\r\n");

    // end of headers
    p = append_str(p, "\r\n");

    // construct the JSON; remember where we started so we will know len
    char *json_start = p;

    // As described - this example reports a pin, uptime, and "via#tuinbot"
    p = append_str(p, "{\"value1\":\"");
    
    size_t lengthT = notification.length();
    char temp[lengthT*2];
    notification.toCharArray(temp,lengthT+1);

    p = append_str(p, temp);
    //p = append_str(p, "\",\"value2\":\"");
    //p = append_ul(p, millis()/1000);
    //p = append_str(p, "\",\"value3\":\"");
    //p = append_str(p, "via #GarageDoor");
    p = append_str(p, "\"}");

    // go back and fill in the JSON length
    // we just know this is at most 2 digits (and need to fill in both)
    int i = strlen(json_start);
    if(i>=100){
      content_length_here[0] = '0' + ((i/100)%10);
      content_length_here[1] = '0' + ((i/10)%10);
      content_length_here[2] = '0' + (i%10);
    }
    else{
      content_length_here[0] = '0' + (i/10);
      content_length_here[1] = '0' + (i%10);
    }

    // finally we are ready to send the POST to the server!
    Serial.println(post_rqst);
    client.print(post_rqst);
    client.stop();
    return 1;
}

char *append_str(char *here, char *s) {
    while (*here++ = *s++)
  ;
    return here-1;
}

char *append_ul(char *here, unsigned long u) {
    char buf[20];       // we "just know" this is big enough

    return append_str(here, ultoa(u, buf, 10));
}

String print2digits(int number) {
  String result;
  if (number < 10) {
    result="0"; // print a 0 before if the number is < than 10
    result+=String(number);
  }
  else{
    result=String(number);
  }
  return result;
}

String getDT(int inputTime){
  unsigned long currentTime = rtc.getEpoch();
  rtc.setEpoch(inputTime-18000);
  String dt_txt;
  dt_txt+=print2digits(rtc.getMonth());
  dt_txt+="/";
  dt_txt+=print2digits(rtc.getDay());
  dt_txt+="/";
  dt_txt+=print2digits(rtc.getYear());
  dt_txt+=" ";

  dt_txt+=print2digits(rtc.getHours());
  dt_txt+=":";
  dt_txt+=print2digits(rtc.getMinutes());
  dt_txt+=":";
  dt_txt+=print2digits(rtc.getSeconds());
  rtc.setEpoch(currentTime);
  return dt_txt;
}

void resetAll(){
  Serial.println(F("Reseting all"));
  
  int timesReset=flash.readLong(timesResetLocation);
  flash.eraseChip();
  int i;
  for(i=0;i<10;i++){
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }

  gsmAccess.shutdown();
  boolean connected = false;
  connectInternet();
  rtc.setEpoch(gsmAccess.getTime());

  flash.writeFloat(differenceLocation,0);
  flash.writeFloat(differenceLocation+32,0);
  flash.writeFloat(differenceLocation+64,ThingSpeak.readIntField(ThingSpeakChannelID,2,ThingSpeakWriteKey));

  for(i=0;i<5;i++){
    if((getWeather())==1){
      break;
    }
    else if((getWeather())==2){
      i=3;
      delay(1000);
    }
    else{
      delay(1000);
    }
  }
  checkHighTemp(flash.readFloat(differenceLocation+64));
  Serial.println("Finished checking temp");
  
  forceDayCycle=0;
  flash.writeLong(timesResetLocation,timesReset+1);
  flash.writeLong(timesResetLocation+32,forceDayCycle);
  gsmAccess.shutdown();
  Serial.print(F("Finished, number of resets: "));
  Serial.println(flash.readLong(timesResetLocation));
  digitalWrite(ledPin, HIGH);
}
