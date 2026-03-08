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

String GROQ_API_KEY = "Paste Your API Key";

WebServer server(80);
Audio audio;
bool speaking = false;

// -------------------
// Webpage served by ESP32 (no browser TTS)
// -------------------
String webpage = R"====(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>AI Voice Assistant</title>
<style>
body {
  margin:0;
  font-family:Arial, sans-serif;
  background:linear-gradient(135deg,#667eea,#764ba2);
  color:white;
  display:flex;
  justify-content:center;
  align-items:center;
  height:100vh;
}
#chatBox {
  width:450px;
  background:rgba(255,255,255,0.15);
  backdrop-filter:blur(20px);
  padding:30px;
  border-radius:20px;
  text-align:center;
  box-shadow:0 8px 32px rgba(0,0,0,0.3);
}
#chatBox h2 {
  margin:0 0 20px;
  font-weight:600;
}
#chatBox button {
  padding:12px 24px;
  margin:10px;
  border:none;
  border-radius:50px;
  cursor:pointer;
  background:linear-gradient(135deg,#ff9a9e,#fad0c4);
  color:#333;
  font-weight:bold;
}
#chatBox .answer {
  margin-top:20px;
  background:white;
  color:#333;
  padding:16px;
  border-radius:10px;
  min-height:40px;
  font-size:16px;
}
</style>
</head>
<body>

<div id="chatBox">
  <h2>🎤 AI Voice Assistant</h2>
  <button onclick="record()">Ask Question</button>
  <div class="answer" id="answer">Hello! Please ask your question 😊</div>
</div>

<script>
// -------------------
// Voice Recognition
// -------------------
let recognition;
function record(){
  const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
  if(!SpeechRecognition){ alert("Speech Recognition not supported"); return; }
  recognition = new SpeechRecognition();
  recognition.lang="en-US";
  recognition.interimResults = false;
  recognition.maxAlternatives = 1;
  recognition.start();

  recognition.onresult = function(event){
    let question = event.results[0][0].transcript;
    document.getElementById("answer").innerHTML = "You: " + question;
    sendQuestion(question);
  };

  recognition.onerror = function(event){
    alert("Error: " + event.error);
  };
}

// -------------------
// Send question to ESP32
// -------------------
function sendQuestion(q){
  fetch("/ask?q="+encodeURIComponent(q))
  .then(res => res.text())
  .then(data => {
    document.getElementById("answer").innerHTML = "AI: " + data;
    // No browser speech synthesis here
  });
}
</script>

</body>
</html>
)====";

// -------------------
// ESP32 TTS using Google Translate
// -------------------
void speakESP32(String text) {
  if(text.length()>180) text = text.substring(0,180);
  text.replace("\n",""); text.replace(",",""); text.replace(".",""); 
  text.replace("?",""); text.replace("!","");
  text.replace(" ","%20");
  String url = "https://translate.google.com/translate_tts?ie=UTF-8&client=tw-ob&tl=en&q=" + text;

  audio.setVolume(40); // max volume (0-40)
  audio.connecttohost(url.c_str());
  speaking = true;
}

// -------------------
// Send text to Groq API
// -------------------
void askGroq(String question){
  if(question.length()==0) return;

  HTTPClient http;
  http.begin("https://api.groq.com/openai/v1/chat/completions");
  http.addHeader("Content-Type","application/json");
  http.addHeader("Authorization","Bearer "+GROQ_API_KEY);

  StaticJsonDocument<4096> doc;
  doc["model"] = "llama-3.3-70b-versatile";
  doc["max_tokens"] = 50;

  JsonArray messages = doc.createNestedArray("messages");
  JsonObject msg = messages.createNestedObject();
  msg["role"] = "user";
  msg["content"] = question;

  String body;
  serializeJson(doc, body);

  int httpCode = http.POST(body);
  if(httpCode == 200){
    String response = http.getString();
    StaticJsonDocument<4096> resDoc;
    DeserializationError err = deserializeJson(resDoc, response);
    if(!err){
      String answer = resDoc["choices"][0]["message"]["content"].as<String>();
      Serial.println("Answer: " + answer);
      answer += ". Please ask next question.";
      speakESP32(answer);
      server.send(200,"text/plain",answer);
    } else {
      Serial.println("JSON parse error");
      speakESP32("Sorry, I could not parse the answer. Please ask next question.");
      server.send(200,"text/plain","Error parsing response.");
    }
  } else {
    Serial.println("HTTP Error: " + String(httpCode));
    speakESP32("Sorry, I could not get an answer. Please ask next question.");
    server.send(200,"text/plain","Error contacting Groq API.");
  }
  http.end();
}

// -------------------
// Web Server Handlers
// -------------------
void handleRoot(){ server.send(200,"text/html",webpage); }
void handleAsk(){
  String question = server.arg("q");
  Serial.println("Question: " + question);
  askGroq(question);
}

// -------------------
// Setup
// -------------------
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(40); // max volume

  server.on("/", handleRoot);
  server.on("/ask", handleAsk);
  server.begin();

  speakESP32("Hello, I am ready. Please ask your question.");
}

// -------------------
// Main Loop
// -------------------
void loop() {
  audio.loop();
  server.handleClient();
  if(speaking && !audio.isRunning()) speaking=false;
}
