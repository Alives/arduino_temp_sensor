#define LED 2
#include <WiFiManager.h>

WiFiManager wifi_manager;

// Parameters are created dynamically in setupWiFi() AFTER config is loaded from SPIFFS
// This ensures the form fields show the current saved values, not empty strings
WiFiManagerParameter* param_version_info = nullptr;
WiFiManagerParameter* param_carbon_host = nullptr;
WiFiManagerParameter* param_ota_password = nullptr;
WiFiManagerParameter* param_sensor_name = nullptr;
bool wifi_params_initialized = false;

// ============================================================================
// STATIC BUFFERS - WiFiManager stores pointers, so all strings must persist!
// ============================================================================

// Hostname/AP name: "sensor_" (7) + sensor_name (max 32) + null (1) = 40 bytes
#define HOSTNAME_BUFFER_SIZE 40
static char sensor_hostname[HOSTNAME_BUFFER_SIZE];

// Page title buffer - must persist
#define TITLE_BUFFER_SIZE 40
static char page_title[TITLE_BUFFER_SIZE];

// Version info HTML - needs enough space for full table (~900 bytes to be safe)
#define VERSION_INFO_BUFFER_SIZE 1024
static char version_info_buffer[VERSION_INFO_BUFFER_SIZE];

// Custom menu HTML - shows on ALL pages (info, config, param, etc.)
#define MENU_HTML_SIZE 1024
static char custom_menu_html[MENU_HTML_SIZE];

// Parameter default values - must persist for WiFiManagerParameter
#define PARAM_VALUE_SIZE 42
static char param_carbon_host_value[PARAM_VALUE_SIZE];
static char param_ota_password_value[PARAM_VALUE_SIZE];
static char param_sensor_name_value[PARAM_VALUE_SIZE];

// ESP core version (returned as temporary String, need to cache it)
static char esp_core_version[20];


void connectWiFi() {
  Serial.println("WiFi Connecting...");
  setupWiFi();
}

bool getWiFiStatus() {
  return (WiFi.status() == WL_CONNECTED);
}

void handleWiFi() {
  digitalWrite(LED, HIGH);
  if (getWiFiStatus()) { return; }
  Serial.println("WiFi Reconnecting...");
  wifi_connect_attempts += 1;
  setupWiFi();
}

void handleHTTPServer() {
  wifi_manager.process();
}

// Helpers for sending key-value row start/end via chunked response without heap allocation
void sendRowStart(const __FlashStringHelper* label) {
  wifi_manager.server->sendContent(F("<dt>"));
  wifi_manager.server->sendContent(label);
  wifi_manager.server->sendContent(F("</dt><dd>"));
}

void sendRowEnd() {
  wifi_manager.server->sendContent(F("</dd>"));
}

void handleStatusPage() {
  updateEnvironment();

  // Chunked response - no large String buffer needed
  wifi_manager.server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  wifi_manager.server->send(200, F("text/html"), "");

  // Head - matches WiFiManager's built-in page styling (same CSS as /info)
  wifi_manager.server->sendContent(F(
    "<!DOCTYPE html><html lang='en'><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1,user-scalable=no'>"
    "<title>"
  ));
  wifi_manager.server->sendContent(sensor_name);
  wifi_manager.server->sendContent(F(
    "</title>"
    "<style>"
    ".c,body{text-align:center;font-family:verdana}"
    "div,input,select{padding:5px;font-size:1em;margin:5px 0;box-sizing:border-box}"
    "input,button,select,.msg{border-radius:.3rem;width:100%}"
    "button{cursor:pointer;border:0;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;margin:5px 0}"
    ".wrap{text-align:left;display:inline-block;min-width:260px;max-width:500px}"
    "a{color:#000;font-weight:700;text-decoration:none}"
    "a:hover{color:#1fa3ec;text-decoration:underline}"
    ".msg{padding:20px;margin:20px 0;border:1px solid #eee;border-left-width:5px;border-left-color:#777}"
    ".msg h4{margin-top:0;margin-bottom:5px}"
    ".msg.S{border-left-color:#5cb85c}.msg.S h4{color:#5cb85c}"
    ".msg.D{border-left-color:#dc3630}.msg.D h4{color:#dc3630}"
    "dt{font-weight:bold}"
    "dd{margin:0;padding:0 0 0.5em 0;min-height:12px}"
    "button.D{background-color:#dc3630}"
    "button:active{opacity:50%!important;cursor:wait}"
    ":disabled{opacity:0.5}"
    "</style></head><body><div class='wrap'>"
  ));

  // Connection status box (matches info page .msg pattern)
  wifi_manager.server->sendContent(F("<div class='msg S'><strong>Connected</strong> to "));
  wifi_manager.server->sendContent(WiFi.SSID());
  wifi_manager.server->sendContent(F("<br/><em><small>with IP "));
  wifi_manager.server->sendContent(WiFi.localIP().toString());
  wifi_manager.server->sendContent(F("</small></em></div>"));

  // Section: Sensor Readings
  wifi_manager.server->sendContent(F("<h3>"));
  wifi_manager.server->sendContent(sensor_name);
  wifi_manager.server->sendContent(F(" v"));
  wifi_manager.server->sendContent(VERSION);
  wifi_manager.server->sendContent(F("</h3><hr>"));

  char float_buf[16];
  char num_buf[16];

  if (bme_initialized) {
    wifi_manager.server->sendContent(F("<dl>"));

    sendRowStart(F("Temperature"));
    dtostrf(env.celcius, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" &deg;C / "));
    dtostrf(env.fahrenheit, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" &deg;F"));
    sendRowEnd();

    sendRowStart(F("Humidity"));
    dtostrf(env.humidity, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" %"));
    sendRowEnd();

    sendRowStart(F("Pressure"));
    dtostrf(env.pressure, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" hPa"));
    sendRowEnd();

    sendRowStart(F("Dew Point"));
    dtostrf(env.dewpointC, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" &deg;C / "));
    dtostrf(env.dewpointF, 1, 2, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" &deg;F"));
    sendRowEnd();

    sendRowStart(F("Altitude"));
    dtostrf(env.altitude, 1, 1, float_buf);
    wifi_manager.server->sendContent(float_buf);
    wifi_manager.server->sendContent(F(" m"));
    sendRowEnd();

    wifi_manager.server->sendContent(F("</dl>"));
  } else {
    wifi_manager.server->sendContent(F("<div class='msg D'><h4>BME280 Error</h4>Sensor not available</div>"));
  }

  // Section: Carbon / Graphite
  wifi_manager.server->sendContent(F("<h3>Carbon / Graphite</h3><hr><dl>"));

  sendRowStart(F("Carbon Host"));
  if (carbon_host.length() > 0) {
    wifi_manager.server->sendContent(carbon_host);
  } else {
    wifi_manager.server->sendContent(F("(not set)"));
  }
  sendRowEnd();

  sendRowStart(F("Carbon Port"));
  itoa(CARBON_PORT, num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  sendRowEnd();

  sendRowStart(F("Connect Attempts"));
  ltoa(client_connect_attempts, num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  sendRowEnd();

  sendRowStart(F("Metric Path"));
  wifi_manager.server->sendContent(F("temp_sensor."));
  wifi_manager.server->sendContent(sensor_name);
  wifi_manager.server->sendContent(F(".<metric>"));
  sendRowEnd();
  wifi_manager.server->sendContent(F("</dl>"));

  // Section: System & Network
  wifi_manager.server->sendContent(F("<h3>System &amp; Network</h3><hr><dl>"));

  sendRowStart(F("SSID"));
  wifi_manager.server->sendContent(WiFi.SSID());
  sendRowEnd();

  sendRowStart(F("RSSI"));
  itoa(WiFi.RSSI(), num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F(" dBm"));
  sendRowEnd();

  sendRowStart(F("IP Address"));
  wifi_manager.server->sendContent(WiFi.localIP().toString());
  sendRowEnd();

  sendRowStart(F("Free Heap"));
  ltoa(ESP.getFreeHeap(), num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F(" bytes"));
  sendRowEnd();

  sendRowStart(F("Heap Frag"));
  itoa(ESP.getHeapFragmentation(), num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F("%"));
  sendRowEnd();

  uint32_t secs = millis() / 1000;
  sendRowStart(F("Uptime"));
  ltoa(secs / 3600, num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F("h "));
  itoa((secs / 60) % 60, num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F("m "));
  itoa(secs % 60, num_buf, 10);
  wifi_manager.server->sendContent(num_buf);
  wifi_manager.server->sendContent(F("s"));
  sendRowEnd();

  sendRowStart(F("Chip ID"));
  wifi_manager.server->sendContent(F("0x"));
  ltoa(ESP.getChipId(), num_buf, 16);
  wifi_manager.server->sendContent(num_buf);
  sendRowEnd();

  sendRowStart(F("Firmware"));
  wifi_manager.server->sendContent(VERSION);
  sendRowEnd();

  sendRowStart(F("ESP Core"));
  wifi_manager.server->sendContent(esp_core_version);
  sendRowEnd();

  sendRowStart(F("SDK"));
  wifi_manager.server->sendContent(ESP.getSdkVersion());
  sendRowEnd();
  wifi_manager.server->sendContent(F("</dl>"));

  // Navigation buttons (full-width, matching info page button style)
  wifi_manager.server->sendContent(F(
    "<form action='/' method='get'><button>Refresh</button></form>"
    "<form action='/wifi' method='get'><button>Configure WiFi</button></form>"
    "<form action='/param' method='get'><button>Configure Parameters</button></form>"
    "<form action='/info' method='get'><button>System Info</button></form>"
    "<form action='/update' method='get'><button>Update</button></form>"
    "<br/>"
    "<form action='/restart' method='get' onsubmit='return confirm(\"Are you sure you want to restart?\")'><button class='D'>Restart</button></form>"
    "</div></body></html>"
  ));

  wifi_manager.server->sendContent(""); // End chunked response
}

void bindServerCallback() {
  if (wifi_manager.server) {
    wifi_manager.server->on("/", handleStatusPage);
    wifi_manager.server->on("/status", handleStatusPage);
  }
}

void setupWiFi() {
  // =========================================================================
  // STEP 1: Populate ALL static buffers FIRST, before any WiFiManager calls
  // =========================================================================
  
  // Build hostname/AP name into static buffer
  snprintf(sensor_hostname, HOSTNAME_BUFFER_SIZE, "sensor_%s", sensor_name.c_str());
  
  // Build page title into static buffer
  snprintf(page_title, TITLE_BUFFER_SIZE, "%s", sensor_name.c_str());
  
  // Cache ESP core version (returns temporary String)
  strncpy(esp_core_version, ESP.getCoreVersion().c_str(), sizeof(esp_core_version) - 1);
  esp_core_version[sizeof(esp_core_version) - 1] = '\0';
  
  // Copy parameter default values into static buffers
  strncpy(param_carbon_host_value, carbon_host.c_str(), PARAM_VALUE_SIZE - 1);
  param_carbon_host_value[PARAM_VALUE_SIZE - 1] = '\0';
  
  strncpy(param_ota_password_value, ota_password.c_str(), PARAM_VALUE_SIZE - 1);
  param_ota_password_value[PARAM_VALUE_SIZE - 1] = '\0';
  
  strncpy(param_sensor_name_value, sensor_name.c_str(), PARAM_VALUE_SIZE - 1);
  param_sensor_name_value[PARAM_VALUE_SIZE - 1] = '\0';
  
  // Build custom menu HTML with version table and status link
  // This renders on ALL WiFiManager pages (info, config, param, etc.)
  // No JS injection needed - the styled button is always visible
  snprintf(custom_menu_html, MENU_HTML_SIZE,
      "<div style='background:#f8fafc;padding:12px;margin:10px 0;border:1px solid #e2e8f0;border-radius:8px;box-shadow:0 1px 3px rgba(0,0,0,0.05);'>"
      "<div style='text-align:center;font-size:1.25em;font-weight:bold;color:#1e293b;margin-bottom:8px;'>%s</div>"
      "<table style='width:100%%;font-size:0.9em;color:#475569;margin-bottom:10px;'>"
      "<tr><td style='padding:2px 0;'>Version</td><td style='text-align:right;'><b>%s</b></td></tr>"
      "<tr><td style='padding:2px 0;'>ESP Core</td><td style='text-align:right;'>%s</td></tr>"
      "<tr><td style='padding:2px 0;'>SDK</td><td style='text-align:right;'>%s</td></tr>"
      "</table>"
      "<a href='/' style='display:block;text-align:center;background:#3b82f6;color:white;padding:8px 12px;border-radius:6px;text-decoration:none;font-weight:bold;font-size:0.9em;box-shadow:0 2px 4px rgba(59,130,246,0.2);transition:background 0.2s;'>View Dashboard</a>"
      "</div>",
      param_sensor_name_value,
      VERSION,
      esp_core_version,
      ESP.getSdkVersion());
  
  // Build simpler version info for param page (custom menu already has details)
  snprintf(version_info_buffer, VERSION_INFO_BUFFER_SIZE,
      "<hr><b>Firmware: %s v%s</b><hr>",
      param_sensor_name_value,
      VERSION);
  
  // Debug output
  Serial.print(F("Hostname buffer: ")); Serial.println(sensor_hostname);
  Serial.print(F("Title buffer: ")); Serial.println(page_title);
  
  // =========================================================================
  // STEP 2: Configure WiFiManager with static buffers
  // =========================================================================
  
  WiFi.hostname(sensor_hostname);
  Serial.println("MAC address: " + WiFi.macAddress());
  Serial.println("Connecting to SSID: " + WiFi.SSID());

  // Set title BEFORE any portal operations (use static buffer!)
  wifi_manager.setTitle(page_title);
  
  // Add custom HTML to menu (shows version + status link on all pages)
  wifi_manager.setCustomMenuHTML(custom_menu_html);
  
  // Explicitly set menu to include 'custom' entry - this ensures our
  // custom HTML (with the /status link) appears in the menu on all pages
  // including /info. Without this, WiFiManager may not render custom HTML.
  const char* menu[] = {"wifi", "info", "param", "custom", "sep", "restart", "exit"};
  wifi_manager.setMenu(menu, 7);
  
  wifi_manager.setConfigPortalTimeout(60);
  wifi_manager.setSaveParamsCallback(saveParams);
  wifi_manager.setWebServerCallback(bindServerCallback);

  // Create parameters only once
  if (!wifi_params_initialized) {
    // Create parameters with static buffer values
    param_version_info = new WiFiManagerParameter(version_info_buffer);
    param_carbon_host = new WiFiManagerParameter(
        "carbon_host", "Carbon Host", param_carbon_host_value, 41);
    param_ota_password = new WiFiManagerParameter(
        "ota_password", "OTA Password", param_ota_password_value, 41);
    param_sensor_name = new WiFiManagerParameter(
        "sensor_name", "Sensor Name", param_sensor_name_value, 41);
    
    wifi_manager.addParameter(param_version_info);
    wifi_manager.addParameter(param_carbon_host);
    wifi_manager.addParameter(param_ota_password);
    wifi_manager.addParameter(param_sensor_name);
    
    wifi_params_initialized = true;
    
    Serial.println(F("WiFi parameters initialized:"));
    Serial.print(F("  carbon_host: ")); Serial.println(param_carbon_host_value);
    Serial.print(F("  ota_password: ")); Serial.println(strlen(param_ota_password_value) > 0 ? "****" : "(empty)");
    Serial.print(F("  sensor_name: ")); Serial.println(param_sensor_name_value);
  }

  // =========================================================================
  // STEP 3: Connect to WiFi
  // =========================================================================
  
  analogWrite(LED, 10);
  // Open config portal if carbon_host is not configured
  if (carbon_host.length() == 0) {
    Serial.println(F("Carbon host not configured, starting config portal..."));
    wifi_manager.startConfigPortal(sensor_hostname, NULL);
  }
  if (!wifi_manager.autoConnect(sensor_hostname)) {
    Serial.println("Failed connecting to WiFi and timed out. Rebooting...");
    ESP.reset();
  }

  while (!getWiFiStatus()) {
    delay(30);
  }
  analogWrite(LED, 0);
  wifi_manager.stopConfigPortal();
  wifi_manager.setCaptivePortalEnable(false);
  wifi_manager.startWebPortal();
  WiFi.mode(WIFI_STA);
  Serial.print("Hostname: ");
  Serial.println(WiFi.hostname());
  Serial.print(F("Primary DNS Server: "));
  Serial.println(WiFi.dnsIP(0));
  Serial.print(F("Secondary DNS Server: "));
  Serial.println(WiFi.dnsIP(1));
  Serial.print(F("Gateway: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("Subnet Mask: "));
  Serial.println(WiFi.subnetMask());
}

void saveParams() {
  Serial.println(F("saveParams() called - checking for changes..."));
  
  if (!param_carbon_host || !param_ota_password || !param_sensor_name) {
    Serial.println(F("ERROR: Parameters not initialized!"));
    return;
  }
  
  String value;

  value = param_carbon_host->getValue();
  value.trim();
  if (value.length() > 0) {
    if (!carbon_host.equals(value)) {
      Serial.print(F("Updating carbon_host: "));
      Serial.print(carbon_host);
      Serial.print(F(" -> "));
      Serial.println(value);
      carbon_host = value;
      writeFile("/carbon_host", carbon_host);
    }
  } else {
    if (!carbon_host.equals(DEFAULT_CARBON_HOST)) {
      Serial.print(F("Updating carbon_host (unset -> default): "));
      Serial.print(carbon_host);
      Serial.print(F(" -> "));
      Serial.println(DEFAULT_CARBON_HOST);
      carbon_host = DEFAULT_CARBON_HOST;
      writeFile("/carbon_host", carbon_host);
    }
  }

  value = param_ota_password->getValue();
  value.trim();
  if (value.length() > 0) {
    if (!ota_password.equals(value)) {
      Serial.println(F("Updating ota_password"));
      ota_password = value;
      writeFile("/ota_password", ota_password);
    }
  } else {
    if (!ota_password.equals(DEFAULT_OTA_PASSWORD)) {
      Serial.println(F("Updating ota_password (unset -> default)"));
      ota_password = DEFAULT_OTA_PASSWORD;
      writeFile("/ota_password", ota_password);
    }
  }

  value = param_sensor_name->getValue();
  if (value.length() > 0 && !sensor_name.equals(value)) {
    Serial.print(F("Updating sensor_name: "));
    Serial.print(sensor_name);
    Serial.print(F(" -> "));
    Serial.println(value);
    sensor_name = value;
    writeFile("/sensor_name", sensor_name);
  } else if (value.length() == 0 && sensor_name.length() > 0) {
    Serial.println(F("Keeping existing sensor_name (form was empty)"));
  }
  
  Serial.println(F("saveParams() complete"));
}
