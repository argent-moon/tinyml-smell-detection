/*
Smell Detection: Blynk Integration
*/

#define BLYNK_TEMPLATE_ID "Template ID"
#define BLYNK_TEMPLATE_NAME "Template Name"
#define BLYNK_AUTH_TOKEN "Auth Token"
#define BLYNK_PRINT Serial

#include <rpcWiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleWioTerminal.h>
#include <Wire.h>
#include <Multichannel_Gas_GMXXX.h>
#include <smell_detection_inferencing.h>

// WiFi credentials
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "WiFi Name";
char pass[] = "WiFi Password";

// Blynk Virtual Pins
#define VPIN_DETECTED_SMELL  V0
#define VPIN_CONFIDENCE      V1
#define VPIN_NO2             V2
#define VPIN_ETHANOL         V3
#define VPIN_CO              V4
#define VPIN_VOC             V5

// Gas Sensor
GAS_GMXXX<TwoWire> gas;

BlynkTimer timer;
float features[40];
static int counter = 0;

// Actual 4 drinks
String classLabels[] = {"Black Tea", "Environment", "Mountain Dew", "Mburukuja Juice"};

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void print_inference_result(ei_impulse_result_t result);

void setup() {
  Serial.begin(115200);
  
  // Initialize Gas Sensor
  Wire.begin();
  gas.begin(Wire, 0x08);
  
  Serial.println("Warming up sensor (2 min)...");
  delay(120000); // 2 minute warm-up
  Serial.println("Sensor ready!");
  
  // Connect to Blynk
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Blynk.config(auth);
  Blynk.connect();
  
  // Update every 0.1 seconds
  timer.setInterval(100L, readSensorAndSendToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
}

void readSensorAndSendToBlynk() {
  // Read gas sensor
  float no2 = gas.measure_NO2();
  float ethanol = gas.measure_C2H5OH();
  float co = gas.measure_CO();
  float voc = gas.measure_VOC();
  
  // Send raw values to Blynk
  Blynk.virtualWrite(VPIN_NO2, no2);
  Blynk.virtualWrite(VPIN_ETHANOL, ethanol);
  Blynk.virtualWrite(VPIN_CO, co);
  Blynk.virtualWrite(VPIN_VOC, voc);
  
  // Print to Serial
  Serial.print("NO2: "); Serial.print(no2);
  Serial.print(" | ETH: "); Serial.print(ethanol);
  Serial.print(" | CO: "); Serial.print(co);
  Serial.print(" | VOC: "); Serial.println(voc);
  
  // Add to features
  features[counter++] = no2;
  features[counter++] = ethanol;
  features[counter++] = co;
  features[counter++] = voc;
  
  // Model Deployment
  if (counter == 40) {
    counter = 0;
    ei_printf("Edge Impulse standalone inferencing (Arduino)\n");

    if (sizeof(features) / sizeof(float) != EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        ei_printf("The size of your 'features' array is not correct. Expected %lu items, but had %lu\n",
            EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, sizeof(features) / sizeof(float));
        delay(1000);
        return;
    }

    ei_impulse_result_t result = { 0 };

    signal_t features_signal;
    features_signal.total_length = sizeof(features) / sizeof(features[0]);
    features_signal.get_data = &raw_feature_get_data;

    // Invoke the impulse
    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        ei_printf("ERR: Failed to run classifier (%d)\n", res);
        return;
    }

    ei_printf("run_classifier returned: %d\r\n", res);
    print_inference_result(result);
  }
}

void print_inference_result(ei_impulse_result_t result) {
  // Print timing
  ei_printf("Timing: DSP %d ms, inference %d ms\r\n",
    result.timing.dsp,
    result.timing.classification);

  // Print predictions
  ei_printf("Predictions:\r\n");
  int maxLabel = 0;
  float maxConfidence = result.classification[0].value;
  
  for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    ei_printf("  %s: ", ei_classifier_inferencing_categories[i]);
    ei_printf("%.5f\r\n", result.classification[i].value);

    if (result.classification[i].value > maxConfidence) {
      maxConfidence = result.classification[i].value;
      maxLabel = i;
    }
  }

  // Send to Blynk
  //Blynk.virtualWrite(VPIN_DETECTED_SMELL, classLabels[maxLabel]);
  Blynk.virtualWrite(VPIN_DETECTED_SMELL, ei_classifier_inferencing_categories[maxLabel]);

  Blynk.virtualWrite(VPIN_CONFIDENCE, maxConfidence * 100);
}
