# ESP-WIFI-NEOPIXEL-CONTROL
NodeMCU script to control NeoPixel ws2812b led strips.
Control colors, brightness and some effects with HTTP or MQTT calls.
<br/><br/>
<strong>Video of Script in Action</strong><br/>https://www.youtube.com/watch?v=9A8iUpCb1MI
<br/><br/>
Example Calls below</strong>
The http server gets initialzed on port 5001, this can be changed in the code.
You should change the following variables to your needs:
- wifi_ssid
- wifi_password
- mqtt_server
- mqtt_user
- mqtt_password
- OTA_hostname
- OTA_password
- PixelCount





<br/><br/>
<h1>MQTT VERSION</h1>

<h2>Control LED effects</h2>
Send JSON commands to the MQTT SET topic to change the effect and brghtness:

Set animation FIRE:
<pre>{"animation":"fun"}</pre>

Set brightness (0-100):
<pre>{"brightness":20}</pre>

Set animation COLOR with predefined color values:
<pre>{"animation":"colorred"}
{"animation":"colorblue"}
{"animation":"colorgreen"}
{"animation":"colorwhite"}
{"animation":"colorblack"}</pre>

Set animation COLOR with RGB values:
<pre>{"animation":"color","color":{"r":200,"g":200,"b":10}}</pre>

Set all at once:
<pre>{"animation":"color","brightness":34,"color":{"r":200,"g":200,"b":10}}</pre>

Note: color values only get considered when animation=color is selected

<h2>Status of LED controller</h2>
During startup and in case an effect changes the controller sends a status message to the MQTT topic.
The status in a JSON message and looks like the following.
The "uptime" value are the milliseconds since the controller was started, "uptimeH" are the hours since it was started.
The "brightness" value is the percent value between 0 and 100, brightnessraw is the actual set value between 0 and 255
<pre>
{
  "uptime":143248,
  "uptimeH":0,
  "animation":"fun",
  "brightness":34,
  "brightnessraw":86,
  "color":{
    "r":9,
    "g":49,
    "b":9
  }
}
</pre>


<br/><br/>
<h1>Example OpenHAB2 Config</h1>

<br/><br/>
<h3>ITEMS</h3>
<pre>
String  Light_L_Digiledstrip_Anim       "Current Animation [%s]"        <light>  {mqtt="<[openhab2:home/ledcontroller1:state:JSONPATH($.animation)]"}
String  Light_L_Digiledstrip_Bright     "Brightness [%s]"               <dimmablelight>  {mqtt="<[openhab2:home/ledcontroller1:state:JSONPATH($.brightness)]"}
String  Light_L_Digiledstrip_Uptime     "Uptime [%s h]"                 <clock>  {mqtt="<[openhab2:home/ledcontroller1:state:JSONPATH($.uptimeH)]"}
Color   Light_L_Digiledstrip_Color      "Color"                         <colorwheel>

String  Light_L_Digiledstrip            "Movie Screen Animation"                <light> { mqtt=">[openhab2:home/ledcontroller1/set:command:*:{animation\\:${command}}]" }
</pre>

<br/><br/>
<h3>SITEMAP</h3>
<pre>
Frame label="Movie Screen Light" icon="light" {
  Selection item=Light_L_Digiledstrip mappings=[off="off", colorblue="Movie", beam="Beam", fun="Party", cylon="Cylon", pulse="Pulse", fire="Fire", aqua="Aqua"]
  Colorpicker item=Light_L_Digiledstrip_Color
  Slider item=Light_L_Digiledstrip_Bright
  Text item=Light_L_Digiledstrip_Uptime
}

</pre>

<br/><br/>
<h3>RULES</h3>
<pre>
import java.awt.Color

rule "LED controller Brightness"
when
  Item Light_L_Digiledstrip_Bright received command
then
  logInfo( "FILE", "RULE: LED controller Brightness triggered")
  //sendHttpGetRequest("http://192.168.0.116:5001/control?brightness=" + Light_L_Digiledstrip_Bright.state)
  publish("openhab2","home/ledcontroller1/set","{brightness:" + Light_L_Digiledstrip_Bright.state + "}")
end

rule "LED controller Color"
when
  Item Light_L_Digiledstrip_Color received command
then
  logInfo( "FILE", "RULE: LED controller Color triggered")
  var hsbValue = Light_L_Digiledstrip_Color.state as HSBType
  var Color color = Color::getHSBColor(hsbValue.hue.floatValue / 360, hsbValue.saturation.floatValue / 100, hsbValue.brightness.floatValue / 100)

  var String redValue   = String.format("%03d", ((color.red.floatValue / 2.55).intValue))
  var String greenValue = String.format("%03d", ((color.green.floatValue / 2.55).intValue))
  var String blueValue  = String.format("%03d", ((color.blue.floatValue / 2.55).intValue))
  logInfo("FILE", "RED: "+ redValue + " GREEN: "+ greenValue +  " BLUE: "+ blueValue + "")

  //sendHttpGetRequest("http://192.168.0.116:5001/control?animationid=color" + redValue + greenValue + blueValue)
  publish("openhab2","home/ledcontroller1/set","{animation:color,color:{r:" + redValue + ",g:" + greenValue + ",b:" + blueValue + "}}")
end
</pre>








<br/><br/>
<h1>WIFI VERSION (legacy)</h1>
<br/><br/><br/>
<strong>Start Effects</strong><br/>Effects were mainly taken from the standard NeoPixelBus example library and slightly changed

<pre>http://[ip]:5001/control?animationid=fun
http://[ip]:5001/control?animationid=beam
http://[ip]:5001/control?animationid=fire
http://[ip]:5001/control?animationid=aqua
http://[ip]:5001/control?animationid=pulse
http://[ip]:5001/control?animationid=cylon</pre>

<br/><br/><br/>
<strong>Set Predefined Colors</strong>

<pre>http://[ip]:5001/control?animationid=colorred
http://[ip]:5001/control?animationid=colorblue
http://[ip]:5001/control?animationid=colorgreen
http://[ip]:5001/control?animationid=colorblack
http://[ip]:5001/control?animationid=colorwhite</pre>


<br/><br/><br/>
<strong>Set Custom Colors</strong><br/>You can add an RGB color code (9 digits) at the end of the color command to set a custom color

<pre>http://[ip]:5001/control?animationid=color255255255</pre>

<br/><br/><br/>
<strong>Set Brightness</strong><br/>Value is a percent value between 0 and 100)

<pre>http://[ip]:5001/control?brightness=20</pre>



<br/><br/><br/>
<strong>Turn LED Strip off</strong>

<pre>http://[ip]:5001/control?animationid=off</pre>



<br/><br/><br/>
<strong>Get current status as JSON with</strong>

<pre>http://[ip]:5001/control?status</pre>
The returned message is the same as in the MQTT version




<br/><br/><br/>
<strong>Stefan Schmidhammer 2017</strong>

