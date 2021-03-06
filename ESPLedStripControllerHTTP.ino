/*
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP calls.
  _____                 
 |_   ____ _ _ _ _ __ _ 
   | |/ -_| '_| '_/ _` |
   |_|\___|_| |_| \__,_|
Stefan Schmidhammer 2017
GPL-3.0

Some animation effects are based upon the NeoPixelBus library Examples published by Michael Miller / Makuna at
https://github.com/Makuna/NeoPixelBus

Fire and Aqua effect based on Examples by John Wall
www.walltech.cc/neopixel-fire-effect/

Example Calls below.
The http server gets initialzed on port 5001, this can be changed in the code.
You should change the following variables to your needs:
- wifi_ssid
- wifi_password
- PixelCount
- PixelPin (note that the pin is fixed to the RX pin on an ESP board, this config will be ignored on ESP boards)


Start Effects
Effects were mainly taken from the standard NeoPixelBus example library and slightly changed
http://<ip>:5001/control?animationid=fun
http://<ip>:5001/control?animationid=beam
http://<ip>:5001/control?animationid=fire
http://<ip>:5001/control?animationid=aqua
http://<ip>:5001/control?animationid=pulse
http://<ip>:5001/control?animationid=cylon

Set Predefined Colors
http://<ip>:5001/control?animationid=colorred
http://<ip>:5001/control?animationid=colorblue
http://<ip>:5001/control?animationid=colorgreen
http://<ip>:5001/control?animationid=colorblack
http://<ip>:5001/control?animationid=colorwhite

Set Custom Colors
You can add an RGB color code (9 digits) at the end of the color command to set a custom color
http://<ip>:5001/control?animationid=color255255255

Set Brightness (value is a percent value between 0 and 100)
http://<ip>:5001/control?brightness=20

Turn LED Strip off
http://<ip>:5001/control?animationid=off

Get current status as JSON with
http://<ip>:5001/control?status



Stefan Schmidhammer 2017
*/

#include <ESP8266WiFi.h>
#include <NeoPixelAnimator.h>
#include <NeoPixelBrightnessBus.h>
#include <ctype.h>

//WiFi Config
const char* wifi_ssid     = "<wifi_ssid>";
const char* wifi_password = "<wifi_password";
WiFiServer server(5001);

const bool dbg = false;

//LED Strip Config
const uint16_t PixelCount = 150;
const uint16_t PixelPin   = 2;

NeoPixelBrightnessBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

struct MyAnimationState
{
    RgbColor StartingColor;
    RgbColor EndingColor;
    uint16_t IndexPixel; // which pixel this animation is effecting
};


NeoPixelAnimator animations(PixelCount);
MyAnimationState animationState[PixelCount];

uint16_t colorSaturation = 255;
RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);
uint16_t brightnessLevelPercent = 80;
uint16_t brightnessLevel = brightnessLevelPercent*2.55;

bool StopSignReceived           = false;
bool WaitForReset               = false;
unsigned long WaitForResetTimer = false;
bool StartAnimationOnce         = true;
uint16_t ResetAnimationDuration = 300;
String definedAnimation         = "";
String definedAnimationNext     = "fun";

String AnimTypes[] = {"fun","pulse","cylon","color","fire","aqua","beam","off"};
uint16_t AnimCount = 8;

float startmillis;


// ################################################################################################################
// ################################################################################################################
// FUN
// ################################################################################################################
void FunLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = HslColor(random(360) / 360.0f, 1.0f, luminance);
        animations.StartAnimation(pixel, 300, FunLightsAnim);
      }
    }
  }
}

void FunLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// FUN
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// PULSE
// ################################################################################################################
void PulseLights(float luminance) {
  if(StopSignReceived == false) {
    RgbColor target = HslColor(random(360) / 360.0f, 1.0f, luminance);
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = target;
        animations.StartAnimation(pixel, 2000, PulseLightsAnim);
      }
    }
  }
}

void PulseLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// PULSE
// ################################################################################################################
// ################################################################################################################





// ################################################################################################################
// ################################################################################################################
// CYLON
// ################################################################################################################
const RgbColor CylonEyeColor(HtmlColor(0x7f0000));
uint16_t CylonLastPixel = 0; // track the eye position
int8_t CylonMoveDir     = 1; // track the direction of movement
AnimEaseFunction moveEase =
//      NeoEase::Linear;
//      NeoEase::QuadraticInOut;
//      NeoEase::CubicInOut;
        NeoEase::QuarticInOut;
//      NeoEase::QuinticInOut;
//      NeoEase::SinusoidalInOut;
//      NeoEase::ExponentialInOut;
//      NeoEase::CircularInOut;
void CylonLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    animations.StartAnimation(0, 5, CylonFadeAnimUpdate);
    animations.StartAnimation(1, 3000, CylonMoveAnimUpdate);
    StartAnimationOnce = false;
  }
}

void CylonFadeAll(uint8_t darkenBy) {
  if(StopSignReceived == false) {
    RgbColor color;
    for (uint16_t indexPixel = 0; indexPixel < strip.PixelCount(); indexPixel++) {
        color = strip.GetPixelColor(indexPixel);
        color.Darken(darkenBy);
        strip.SetPixelColor(indexPixel, color);
    }
  }
}

void CylonFadeAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    if (param.state == AnimationState_Completed) {
        CylonFadeAll(10);
        animations.RestartAnimation(param.index);
    }
  }
}

void CylonMoveAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {

    // apply the movement animation curve
    float progress = moveEase(param.progress);

    // use the curved progress to calculate the pixel to effect
    uint16_t CylonNextPixel;
    if (CylonMoveDir > 0)
    {
        CylonNextPixel = progress * PixelCount;
    }
    else
    {
        CylonNextPixel = (1.0f - progress) * PixelCount;
    }

    // if progress moves fast enough, we may move more than
    // one pixel, so we update all between the calculated and
    // the last
    if (CylonLastPixel != CylonNextPixel)
    {
        for (uint16_t i = CylonLastPixel + CylonMoveDir; i != CylonNextPixel; i += CylonMoveDir)
        {
            strip.SetPixelColor(i, CylonEyeColor);
        }
    }
    strip.SetPixelColor(CylonNextPixel, CylonEyeColor);

    CylonLastPixel = CylonNextPixel;

    if (param.state == AnimationState_Completed)
    {
        // reverse direction of movement
        CylonMoveDir *= -1;

        // done, time to restart this position tracking animation/timer
        animations.RestartAnimation(param.index);
    }

  }
}
// ################################################################################################################
// CYLON
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// COLOR
// ################################################################################################################
RgbColor ColorOfStrip  = green;
String ColorOfStripStr = "";
void ColorLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = ColorOfStrip;
        animations.StartAnimation(pixel, ResetAnimationDuration, ColorLightsAnim);
      }
    }
    StartAnimationOnce = false;
  }
}

void ColorLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// COLOR
// ################################################################################################################
// ################################################################################################################



// ################################################################################################################
// ################################################################################################################
// FIRE
// ################################################################################################################

int fire_r = 255;
int fire_g = fire_r-180;
int fire_b = 40;
int fire_flicker;
int fire_r_fadein;
int fire_g_fadein;
int fire_b_fadein;
void FireLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        fire_flicker = random(0,150);
        fire_r_fadein = fire_r-fire_flicker;
        fire_g_fadein = fire_g-fire_flicker;
        fire_b_fadein = fire_b-fire_flicker;
        if(fire_g_fadein<0) fire_g_fadein=0;
        if(fire_r_fadein<0) fire_r_fadein=0;
        if(fire_b_fadein<0) fire_b_fadein=0;
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = RgbColor(fire_r_fadein, fire_g_fadein, fire_b_fadein);
        animations.StartAnimation(pixel, random(100,300), FireLightsAnim);
      }
    }
  }
}

void FireLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// FIRE
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// AQUA
// ################################################################################################################

int aqua_r = 10;
int aqua_g = aqua_r-180;
int aqua_b = 255;
int aqua_flicker;
int aqua_r_fadein;
int aqua_g_fadein;
int aqua_b_fadein;
void AquaLights(float luminance) {
  if(StopSignReceived == false) {
    for(int pixel=0; pixel < PixelCount; pixel++) {
      if( !animations.IsAnimationActive(pixel) ) {
        aqua_flicker = random(0,200);
        aqua_r_fadein = aqua_r-aqua_flicker;
        aqua_g_fadein = aqua_g-aqua_flicker;
        aqua_b_fadein = aqua_b-aqua_flicker;
        if(aqua_g_fadein<0) aqua_g_fadein=0;
        if(aqua_r_fadein<0) aqua_r_fadein=0;
        if(aqua_b_fadein<0) aqua_b_fadein=0;
        animationState[pixel].StartingColor = strip.GetPixelColor(pixel);
        animationState[pixel].EndingColor   = RgbColor(aqua_r_fadein, aqua_g_fadein, aqua_b_fadein);
        animations.StartAnimation(pixel, random(100,300), AquaLightsAnim);
      }
    }
  }
}

void AquaLightsAnim(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(param.index, updatedColor);
  }
}
// ################################################################################################################
// AQUA
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// BEAM
// ################################################################################################################
uint16_t BeamFrontPixel = 0;  // the front of the loop
RgbColor BeamFrontColor;  // the color at the front of the loop
const uint16_t BeamPixelFadeDuration = 300;
const uint16_t BeamNextPixelMoveDuration = 1000 / PixelCount;
void BeamLights(float luminance) {
  if(StopSignReceived == false && StartAnimationOnce == true) {
    animations.StartAnimation(0, BeamNextPixelMoveDuration, BeamLoopAnimUpdate);
    StartAnimationOnce = false;
  }
}

void BeamLoopAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    if (param.state == AnimationState_Completed) {
      animations.RestartAnimation(param.index);
      BeamFrontPixel = (BeamFrontPixel + 1) % PixelCount; // increment and wrap
      if (BeamFrontPixel == 0) {
          BeamFrontColor = HslColor(random(360) / 360.0f, 1.0f, 0.25f);
      }
      uint16_t indexAnim;
      if (animations.NextAvailableAnimation(&indexAnim, 1)) {
          animationState[indexAnim].StartingColor = BeamFrontColor;
          animationState[indexAnim].EndingColor   = RgbColor(0, 0, 0);
          animationState[indexAnim].IndexPixel    = BeamFrontPixel;
          animations.StartAnimation(indexAnim, BeamPixelFadeDuration, BeamFadeOutAnimUpdate);
      }
    }
  }
}

void BeamFadeOutAnimUpdate(const AnimationParam& param) {
  if(StopSignReceived == false) {
    RgbColor updatedColor = RgbColor::LinearBlend(animationState[param.index].StartingColor,animationState[param.index].EndingColor,param.progress);
    strip.SetPixelColor(animationState[param.index].IndexPixel, updatedColor);
  }
}
// ################################################################################################################
// BEAM
// ################################################################################################################
// ################################################################################################################




// ################################################################################################################
// ################################################################################################################
// STOP
// ################################################################################################################
void stopAllAnimations() {
  debugln("stop animation defined, running for " + String(ResetAnimationDuration) + "ms");
  for(int pixel=0; pixel < PixelCount; pixel++) {
    animations.StopAnimation(pixel);
    animations.StartAnimation(pixel, ResetAnimationDuration, stopAllAnimationsAnim);
  }
  StopSignReceived = false;
}

void stopAllAnimationsAnim(const AnimationParam& param) {
  strip.SetPixelColor(param.index, black);
}
// ################################################################################################################
// STOP
// ################################################################################################################
// ################################################################################################################


void loop() {

  if( definedAnimationNext != "" ) {
    definedAnimation     = definedAnimationNext;
    definedAnimationNext = "";
    StartAnimationOnce   = true;
  }

  if( StopSignReceived == true) {
    debugln("STOP ANIMATION COMMAND RECEIVED, PREPARE FOR RESET");
    stopAllAnimations();
    WaitForReset = true;
    WaitForResetTimer = millis();
    debugln("reset animation start");
  }

  if( WaitForReset && (WaitForResetTimer+ResetAnimationDuration+100) < millis() ) { //add 100ms for safety
    WaitForResetTimer = 0;
    WaitForReset      = false;
    debugln("reset animation end");
  }

  if( StopSignReceived == false && WaitForReset == false ) {

    if( definedAnimation == "fun" ) {
      FunLights(0.25f);
    } else if( definedAnimation == "pulse" ) {
      PulseLights(0.25f);
    } else if( definedAnimation == "cylon" ) {
      CylonLights(0.25f);
    } else if( definedAnimation == "beam" ) {
      BeamLights(0.25f);
    } else if( definedAnimation == "color" ) {
      ColorLights(0.25f);
    } else if( definedAnimation == "fire" ) {
      FireLights(0.25f);
    } else if( definedAnimation == "aqua" ) {
      AquaLights(0.25f);
    } else if( definedAnimation == "off" ) {
      ColorOfStripStr = "black";
      ColorOfStrip = black;
      ColorLights(0.25f);
    }

  }

  animations.UpdateAnimations();  
  strip.Show();

  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
 
  // Wait until the client sends some data
  debugln("new client");
  int timeoutstart = millis();
  bool isTimeout   = false;
  while(!client.available() && isTimeout == false){
    delay(1);
    //continue with animations while waiting
    animations.UpdateAnimations();  
    strip.Show();
    if( millis() > (timeoutstart+2000) ) {
      debugln("client timeout");
      isTimeout = true;
    }
  }

  if( !isTimeout ) {

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println(""); //  do not forget this one
    
    // Read the first line of the request
    String request = client.readStringUntil('\r');
  
    // trim to the raw message 
    if( request.indexOf(" HTTP") > 0 ) {
      request = request.substring( 0 , request.indexOf(" HTTP") );
    }
    
    debugln(request);
    client.flush();
  
    if (request.indexOf("/control?animationid=") != -1) {

      String animationType;
      animationType    = request.substring( request.indexOf("animationid=")+12 , request.indexOf(" HTTP") );
      ColorOfStripStr  = "";
      if(animationType.indexOf("color") == 0) {
        ColorOfStripStr    = animationType.substring( 5 , animationType.length() );
        animationType      = "color";
        debugln("Set new animation: " + animationType + " with color: " + ColorOfStripStr);
        if( ColorOfStripStr == "red" ) {
          ColorOfStrip = red;
        } else if( ColorOfStripStr == "blue" ) {
          ColorOfStrip = blue;
        } else if( ColorOfStripStr == "green" ) {
          ColorOfStrip = green;
        } else if( ColorOfStripStr == "black" ) {
          ColorOfStrip = black;
        } else if( ColorOfStripStr == "white" ) {
          ColorOfStrip = white;
        } else if( ColorOfStripStr.length() == 9 && isValidNumber(ColorOfStripStr) ) {
          ColorOfStrip = RgbColor( ColorOfStripStr.substring(0,3).toInt() , ColorOfStripStr.substring(3,6).toInt() , ColorOfStripStr.substring(6,9).toInt() );
        }
        
      } else {
        debugln("Set new animation: " + animationType);
      }
  
      if( !inArray(AnimTypes, AnimCount, animationType) ) {
        definedAnimationNext = "fire";
      }
  
      StopSignReceived     = true;
      definedAnimationNext = animationType;
      definedAnimation     = "";
      StartAnimationOnce   = true;

      client.println("{ \"code\":\"OK\", \"info\":\"Animation set "+definedAnimationNext + ColorOfStripStr+"\" }");
  
    } else if (request.indexOf("/control?status") != -1) {
      client.println("{ \"animation\":\"" + definedAnimation + ColorOfStripStr + "\", \"brightness\":" + String(brightnessLevelPercent) + ", \"brightnessraw\":" + String(brightnessLevel) + ", \"uptime\":" + String(millis()) + " }");
    } else if (request.indexOf("/control?brightness=") != -1) {
      String brightnessLevelStr;
      brightnessLevelStr    = request.substring( request.indexOf("brightness=")+11 , request.indexOf(" HTTP") );
      if( brightnessLevelStr.length() <= 3 && brightnessLevelStr.length() >= 1 && isValidNumber(brightnessLevelStr) ) {
        brightnessLevelPercent = brightnessLevelStr.toInt();
        brightnessLevel = brightnessLevelPercent*2.55;
        if( brightnessLevelPercent >= 0 && brightnessLevelPercent <= 100 ) {
          strip.SetBrightness( brightnessLevel );
          client.println("{ \"code\":\"OK\", \"info\":\"brightness set to " + String(brightnessLevelPercent) + "%\" }");
        } else {
          client.println("{ \"code\":\"FAIL\", \"info\":\"brightness was not set, invalid number range passed\" }");
        }
      } else {
        client.println("{ \"code\":\"FAIL\", \"info\":\"brightness was not set, invalid value passed\" }");
      }
    } else if (request.indexOf("/control") != -1) {

      //client.println(F("<!doctype html> <html itemscope=\"\" itemtype=\"http://schema.org/SearchResultsPage\" lang=\"de-AT\"> <head> <title>LED Controller</title> <style> body { background-color:orange; font-size:15px; font-family:arial; } .effects { width:250px; display:block; } .colors { width:250px; display:block; clear:both; padding-top:20px; } .colors input,button,span.label { float:left; width:58px; display:block; font-size:35px; margin:2px 10px; height:40px; border:1px solid orange; text-align: center; } a,button,.effects span { float:left; width:90px; display:block; border:1px solid black; padding:15px; margin:1px; background-color:grey; color:white; text-decoration: none; } button { height:45px; width:245px; font-size:15px; } </style><script>"));
      //client.println("function call(url) {");
      //client.println("  var xmlhttp = new XMLHttpRequest();");
      //client.println("  xmlhttp.open(\"GET\",url,true);");
      //client.println("  xmlhttp.send();");
      //client.println("}");
      //client.println(F("</script> </head> <body> <div class=\"effects\"> <span onclick=\"javascript:call(window.location.href+'?animationid=off');\">OFF</span> <span onclick=\"javascript:call(window.location.href+'?animationid=beam');\">BEAM</span> <span onclick=\"javascript:call(window.location.href+'?animationid=fire');\">FIRE</span> <span onclick=\"javascript:call(window.location.href+'?animationid=aqua');\">AQUA</span> <span onclick=\"javascript:call(window.location.href+'?animationid=cylon');\">CYLON</span> <span onclick=\"javascript:call(window.location.href+'?animationid=fun');\">FUN</span> <span onclick=\"javascript:call(window.location.href+'?animationid=pulse');\">PULSE</span> <span onclick=\"javascript:call(window.location.href+'?animationid=colorblue');\">MOVIE</span> </div> <div class=\"colors\"> <form action=\"\" method=\"GET\"> <span class=\"label\">R</span> <span class=\"label\">G</span> <span class=\"label\">B</span> <input type=\"text\" id=\"r\" value=\"170\" /> <input type=\"text\" id=\"g\" value=\"80\" /> <input type=\"text\" id=\"b\" value=\"10\" /> <button type=\"button\" onclick=\"javascript:call(window.location.href+'?animationid=color'+('00'+document.getElementById('r').value).slice(-3)+('00'+document.getElementById('g').value).slice(-3)+('00'+document.getElementById('b').value).slice(-3));\">SET COLOR</button> </form> </div> </body> </html>"));

    } else {
      client.println("{ \"code\":\"FAIL\", \"info\":\"Unknown Request received\" }");
    }

    debugln("Client disconnected\n");

  }
 
}







void setup() {

  if(dbg) {
    Serial.begin(115200);
    delay(4);
  }

  // Connect to WiFi network
  debugln("Connecting to ");
  debugln(wifi_ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    debug(".");
  }
  debugln("WiFi connected");

  // Start the server
  server.begin();
  debugln("Server started");

  // Print the IP address
  if(dbg) {
    Serial.println("Use this URL to connect: ");
    Serial.print("http://");
    Serial.print(WiFi.localIP());
    Serial.println("/");
  }


  strip.Begin();
  strip.Show();
  strip.SetBrightness(brightnessLevel);

  startmillis = millis();

}




// ################################################################################################################
// ################################################################################################################
// GLOBAL FUNCTIONS
// ################################################################################################################

boolean isValidNumber(String str) {
   boolean isNum=false;
   for(byte i=0;i<str.length();i++)
   {
       isNum = isDigit(str.charAt(i)) || str.charAt(i) == '+' || str.charAt(i) == '.' || str.charAt(i) == '-';
       if(!isNum) return false;
   }
   return isNum;
}

boolean inArray(String arr[], int count, String needle) {
  for(int a=0;a < count;a++) {
    if( arr[a] == needle ) {
      return true;
    }
  }
  return false;
}

void debug(String dbg) {
  if(dbg) {
    Serial.print(dbg);
  }
}
void debugln(String dbg) {
  if(dbg) {
    Serial.println(dbg);
  }
}

