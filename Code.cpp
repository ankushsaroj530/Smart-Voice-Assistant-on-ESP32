#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include "Audio.h"

// I2S Pins
#define I2S_DOUT 22
#define I2S_BCLK 26
#define I2S_LRC 25

const char* ssid = "Ankush";
const char* password = "123456789";

String GROQ_API_KEY = "Enter Your GROQ API Key";


WebServer server(80);
Audio audio;

bool speaking=false;


// -------------------
// WEBPAGE
// -------------------

String webpage = R"====(
<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">

<title>NEXA AI Assistant</title>

<style>

body{
margin:0;
font-family:Arial;
background:linear-gradient(135deg,#141e30,#243b55);
display:flex;
justify-content:center;
align-items:center;
height:100vh;
color:white;
}

#box{
width:420px;
background:rgba(255,255,255,0.1);
backdrop-filter:blur(20px);
padding:30px;
border-radius:20px;
text-align:center;
}

#face{
font-size:90px;
margin-bottom:10px;
}

button{
padding:12px 25px;
border:none;
border-radius:50px;
background:#00ffd5;
cursor:pointer;
font-weight:bold;
font-size:16px;
}

.answer{
margin-top:20px;
background:white;
color:#333;
padding:15px;
border-radius:10px;
min-height:40px;
}

</style>

</head>

<body>

<div id="box">

<div id="face">🙂</div>

<h2>NEXA AI Assistant</h2>

<button onclick="record()">🎤 Ask Question</button>

<div class="answer" id="answer">
Hello! I am Nexa created by Ankush. Press the microphone button and ask your question.
</div>

</div>

<script>

let face=document.getElementById("face")
let recognition

function record(){

const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition

recognition=new SpeechRecognition()

recognition.lang="en-US"

face.innerHTML="👂"

recognition.start()

recognition.onresult=function(event){

let q=event.results[0][0].transcript

document.getElementById("answer").innerHTML="You: "+q

sendQuestion(q)

}

}

function sendQuestion(q){

face.innerHTML="🤔"

fetch("/ask?q="+encodeURIComponent(q))

.then(res=>res.text())

.then(data=>{

face.innerHTML="🗣"

document.getElementById("answer").innerHTML="NEXA: "+data

setTimeout(()=>{

face.innerHTML="🙂"

},3000)

})

}

</script>

</body>
</html>
)====";


// -------------------
// SPEAK FUNCTION
// -------------------

void speakESP32(String text){

if(text.length()>180) text=text.substring(0,180);

text.replace("\n","");
text.replace(",","");
text.replace(".","");
text.replace("?","");
text.replace("!","");
text.replace(" ","%20");

String url="https://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&tl=en&q="+text;

audio.setVolume(60);

audio.connecttohost(url.c_str());

speaking=true;

}


// -------------------
// GROQ AI
// -------------------

void askGroq(String question){

HTTPClient http;

http.begin("https://api.groq.com/openai/v1/chat/completions");

http.addHeader("Content-Type","application/json");
http.addHeader("Authorization","Bearer "+GROQ_API_KEY);

StaticJsonDocument<4096> doc;

doc["model"]="llama-3.1-8b-instant";
doc["max_tokens"]=60;
doc["temperature"]=0.7;

JsonArray messages=doc.createNestedArray("messages");

JsonObject sys=messages.createNestedObject();
sys["role"]="system";
sys["content"]="You are NEXA, a friendly AI assistant created by Ankush. Reply short.";

JsonObject msg=messages.createNestedObject();
msg["role"]="user";
msg["content"]=question;

String body;
serializeJson(doc,body);

int httpCode=http.POST(body);

if(httpCode==200){

String response=http.getString();

StaticJsonDocument<4096> resDoc;
deserializeJson(resDoc,response);

String answer=resDoc["choices"][0]["message"]["content"].as<String>();

answer+=". Please ask your next question.";

speakESP32(answer);

server.send(200,"text/plain",answer);

}

else{

String error="Sorry I could not answer.";

speakESP32(error);

server.send(200,"text/plain",error);

}

http.end();

}


// -------------------
// SERVER
// -------------------

void handleRoot(){

server.send(200,"text/html",webpage);

}

void handleAsk(){

String q=server.arg("q");

askGroq(q);

}


// -------------------
// SETUP
// -------------------

void setup(){

Serial.begin(115200);

WiFi.begin(ssid,password);

while(WiFi.status()!=WL_CONNECTED){

delay(500);
Serial.print(".");

}

Serial.println("");
Serial.println("WiFi Connected");

audio.setPinout(I2S_BCLK,I2S_LRC,I2S_DOUT);

audio.setVolume(60);

server.on("/",handleRoot);
server.on("/ask",handleAsk);

server.begin();

speakESP32("Hello I am Nexa created by Ankush. Press the microphone button and ask your question.");

}


// -------------------
// LOOP
// -------------------

void loop(){

audio.loop();

server.handleClient();

if(speaking && !audio.isRunning())
speaking=false;

}
