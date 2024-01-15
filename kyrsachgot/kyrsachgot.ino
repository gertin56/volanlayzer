#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <arduinoFFT.h>

#define SAMPLES 256    // Количество семплов для анализа
#define SAMPLING_FREQ 22800  // Частота дискретизации

#define LOW_PASS 30 
#define GAIN 44100

int16_t samples[SAMPLES];
double real[SAMPLES];
double imaginary[SAMPLES];

//byte posOffset[16] = {2, 3, 4, 6, 8, 10, 12, 14, 16, 20, 25, 30, 35, 60, 80, 100};

arduinoFFT fft = arduinoFFT(real, imaginary, SAMPLES, SAMPLING_FREQ);

const char* ssid = "DIR-2150-1406";
const char* password = "22959423";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <style>
    body {
      min-width: 310px;
    	max-width: 800px;
    	height: 400px;
      margin: 0 auto;
    }
    h2 {
      font-family: Arial;
      font-size: 2.5rem;
      text-align: center;
    }
  </style>
</head>
<body>
  <h2>ESP VOl Analyzer</h2>
  <div id="audio" class="container"></div>
</body>
<script>
var chartT = new Highcharts.Chart({
  chart:{ type: 'column', renderTo : 'audio' },
  title: { text: 'Specture' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    column: {
      pointPadding: 0.2,
      borderWidth: 0
    }
  },
  xAxis: { 
    title: { text: 'Friquency' }
  },
  yAxis: {
    title: { text: 'Amplitude' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var siries = [];
      var arrayOfStrings = this.responseText;
      var data = arrayOfStrings.split(' ');
      for(var i = 0; i < 128; i++){
        var x = i* 89,
            y = parseFloat(data[i]);
        siries.push([x,y]);
      }
      chartT.series[0].setData(siries);
    }
  };
  xhttp.open("GET", "/audio", true);
  xhttp.send();
}, 100 ) ;

</script>
</html>
)rawliteral";

AsyncWebServer server(80);

String analyzeAudio() {
  String str = "";

  for (int i = 0; i < SAMPLES; i++) {
    real[i] = adc1_get_raw(ADC1_CHANNEL_6);
    imaginary[i] = 0;
  }

  // Применение FFT к буферу
  fft.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  fft.Compute(FFT_FORWARD);
  fft.ComplexToMagnitude();

  for (int pos = 0; pos < SAMPLES/2; pos++) {
    int posLevel = map(real[pos], LOW_PASS, GAIN, 0, 15);
    posLevel = constrain(posLevel, 0, 15);
    str += posLevel;
    str += " ";
  }
  return String(str);
}

void setup(){
  Serial.begin(115200);

  adc1_config_width(ADC_WIDTH_BIT_12);  // Настройка разрядности АЦП
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);  // Настройка канала АЦП

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });
  server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", analyzeAudio().c_str());
  });

  server.begin();
}
 
void loop(){
  
}